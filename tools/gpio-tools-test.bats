#!/usr/bin/env bats
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

# Simple test harness for the gpio-tools.

# Where output from coprocesses is stored
COPROC_OUTPUT=$BATS_TMPDIR/gpio-tools-test-output
# Save the PID of coprocess - otherwise we won't be able to wait for it
# once it exits as the COPROC_PID will be cleared.
COPROC_SAVED_PID=""

# Run the command in $* and return 0 if the command failed. The way we do it
# here is a workaround for the way bats handles failing processes.
assert_fail() {
	$* || return 0
	return 1
}

# Check if the string in $2 matches against the pattern in $1.
regex_matches() {
	local PATTERN=$1
	local STRING=$2

	[[ $STRING =~ $PATTERN ]]
}

# Iterate over all lines in the output of the last command invoked with bats'
# 'run' or the coproc helper and check if at least one is equal to $1.
output_contains_line() {
	local LINE=$1

	for line in "${lines[@]}"
	do
		test "$line" = "$LINE" && return 0
	done

	return 1
}

# Same as above but match against the regex pattern in $1.
output_regex_match() {
	local PATTERN=$1

	for line in "${lines[@]}"
	do
		regex_matches "$PATTERN" "$line" && return 0
	done

	return 1
}

# Probe the gpio-mockup kernel module. The routine expects a list of chip
# sizes and optionally the 'named-lines' flag.
gpio_mockup_probe() {
	local CMDLINE="gpio_mockup_ranges="

	for ARG in $*
	do
		if [ $ARG = "named-lines" ]
		then
			CMDLINE="gpio_mockup_named_lines $CMDLINE"
			continue
		fi

		regex_matches "[0-9]+" "$ARG"

		CMDLINE="$CMDLINE-1,$ARG,"
	done

	CMDLINE=${CMDLINE%?}

	modprobe gpio-mockup $CMDLINE
	udevadm settle
}

gpio_mockup_remove() {
	if [ -d /sys/module/gpio_mockup ]
	then
		rmmod gpio-mockup
	fi
}

gpio_mockup_chip_name() {
	local CHIPNUM=$1

	test -d /sys/devices/platform/gpio-mockup.$CHIPNUM/ || return 1
	echo $(ls /sys/devices/platform/gpio-mockup.$CHIPNUM/ | grep gpiochip)
}

gpio_mockup_set_pull() {
	local CHIPNUM=$1
	local LINE=$2
	local PULL=$3

	FILE="/sys/kernel/debug/gpio-mockup/$(gpio_mockup_chip_name $CHIPNUM)/$LINE"
	test -f $FILE || return 1
	echo "$PULL" > $FILE
}

gpio_mockup_check_value() {
	local CHIPNUM=$1
	local LINE=$2
	local EXPECTED=$3

	FILE="/sys/kernel/debug/gpio-mockup/$(gpio_mockup_chip_name $CHIPNUM)/$LINE"
	test -f $FILE || return 1

	VAL=$(cat $FILE)
	test $VAL -eq $EXPECTED || return 1
}

run_tool() {
	# Executables to test are expected to be in the same directory as the
	# testing script.
	run timeout 10s $BATS_TEST_DIRNAME/"$@"
}

coproc_run_tool() {
	rm -f $BR_PROC_OUTPUT
	coproc timeout 10s $BATS_TEST_DIRNAME/"$@" > $COPROC_OUTPUT 2> $COPROC_OUTPUT
	COPROC_SAVED_PID=$COPROC_PID
	# FIXME We're giving the background process some time to get up, but really this
	# should be more reliable...
	sleep 0.2
}

coproc_tool_stdin_write() {
	echo $* >&${COPROC[1]}
}

coproc_tool_kill() {
	SIGNUM=$1

	kill $SIGNUM $COPROC_SAVED_PID
}

coproc_tool_wait() {
	status="0"
	# A workaround for the way bats handles command failures.
	wait $COPROC_SAVED_PID || export status=$?
	test "$status" -ne 0 || export status="0"
	output=$(cat $COPROC_OUTPUT)
	local ORIG_IFS="$IFS"
	IFS=$'\n' lines=($output)
	IFS="$ORIG_IFS"
	rm -f $COPROC_OUTPUT
}

setup() {
	gpio_mockup_remove
}

teardown() {
	if [ -n "$BG_PROC_PID" ]
	then
		kill -9 $BG_PROC_PID
		run wait $BG_PROC_PID
		BG_PROC_PID=""
	fi

	gpio_mockup_remove
}

#
# gpiodetect test cases
#

@test "gpiodetect: list chips" {
	gpio_mockup_probe 4 8 16

	run_tool gpiodetect

	test "$status" -eq 0
	output_contains_line "$(gpio_mockup_chip_name 0) [gpio-mockup-A] (4 lines)"
	output_contains_line "$(gpio_mockup_chip_name 1) [gpio-mockup-B] (8 lines)"
	output_contains_line "$(gpio_mockup_chip_name 2) [gpio-mockup-C] (16 lines)"
}

@test "gpiodetect: invalid args" {
	run_tool gpiodetect unimplemented-arg
	test "$status" -eq 1
}

#
# gpioinfo test cases
#

@test "gpioinfo: dump all chips" {
	gpio_mockup_probe 4 8

	run_tool gpioinfo

	test "$status" -eq 0
	output_contains_line "$(gpio_mockup_chip_name 0) - 4 lines:"
	output_contains_line "$(gpio_mockup_chip_name 1) - 8 lines:"

	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+input\\s+active-high"
}

@test "gpioinfo: dump all chips with one line exported" {
	gpio_mockup_probe 4 8

	coproc_run_tool gpioset --mode=signal --active-low "$(gpio_mockup_chip_name 1)" 7=1

	run_tool gpioinfo

	test "$status" -eq 0
	output_contains_line "$(gpio_mockup_chip_name 0) - 4 lines:"
	output_contains_line "$(gpio_mockup_chip_name 1) - 8 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+\\\"gpioset\\\"\\s+output\\s+active-low"

	coproc_tool_kill
	coproc_tool_wait
}

@test "gpioinfo: dump one chip" {
	gpio_mockup_probe 8 4

	run_tool gpioinfo "$(gpio_mockup_chip_name 1)"

	test "$status" -eq 0
	assert_fail output_contains_line "$(gpio_mockup_chip_name 0) - 8 lines:"
	output_contains_line "$(gpio_mockup_chip_name 1) - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	assert_fail output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+input\\s+active-high"
}

@test "gpioinfo: dump all but one chip" {
	gpio_mockup_probe 4 4 8 4

	run_tool gpioinfo "$(gpio_mockup_chip_name 0)" \
			"$(gpio_mockup_chip_name 1)" "$(gpio_mockup_chip_name 3)"

	test "$status" -eq 0
	output_contains_line "$(gpio_mockup_chip_name 0) - 4 lines:"
	output_contains_line "$(gpio_mockup_chip_name 1) - 4 lines:"
	assert_fail output_contains_line "$(gpio_mockup_chip_name 2) - 8 lines:"
	output_contains_line "$(gpio_mockup_chip_name 3) - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	assert_fail output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+input\\s+active-high"
}

@test "gpioinfo: inexistent chip" {
	run_tool gpioinfo "inexistent"

	test "$status" -eq 1
}

#
# gpiofind test cases
#

@test "gpiofind: line found" {
	gpio_mockup_probe named-lines 4 8 16

	run_tool gpiofind gpio-mockup-B-7

	test "$status" -eq "0"
	test "$output" = "$(gpio_mockup_chip_name 1) 7"
}

@test "gpiofind: line not found" {
	gpio_mockup_probe named-lines 4 8 16

	run_tool gpiofind nonexistent-line

	test "$status" -eq "1"
}

@test "gpiofind: invalid args" {
	run_tool gpiodetect unimplemented-arg
	test "$status" -eq 1
}

#
# gpioget test cases
#

@test "gpioget: read all lines" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 1 2 1
	gpio_mockup_set_pull 1 3 1
	gpio_mockup_set_pull 1 5 1
	gpio_mockup_set_pull 1 7 1

	run_tool gpioget "$(gpio_mockup_chip_name 1)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "0 0 1 1 0 1 0 1"
}

@test "gpioget: read all lines (active-low)" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 1 2 1
	gpio_mockup_set_pull 1 3 1
	gpio_mockup_set_pull 1 5 1
	gpio_mockup_set_pull 1 7 1

	run_tool gpioget --active-low "$(gpio_mockup_chip_name 1)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "1 1 0 0 1 0 1 0"
}

@test "gpioget: read all lines (pull-up)" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 1 2 1
	gpio_mockup_set_pull 1 3 1
	gpio_mockup_set_pull 1 5 1
	gpio_mockup_set_pull 1 7 1

	run_tool gpioget --bias=pull-up "$(gpio_mockup_chip_name 1)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "1 1 1 1 1 1 1 1"
}

@test "gpioget: read all lines (pull-down)" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 1 2 1
	gpio_mockup_set_pull 1 3 1
	gpio_mockup_set_pull 1 5 1
	gpio_mockup_set_pull 1 7 1

	run_tool gpioget --bias=pull-down "$(gpio_mockup_chip_name 1)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "0 0 0 0 0 0 0 0"
}

@test "gpioget: read some lines" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 1 1 1
	gpio_mockup_set_pull 1 4 1
	gpio_mockup_set_pull 1 6 1

	run_tool gpioget "$(gpio_mockup_chip_name 1)" 0 1 4 6

	test "$status" -eq "0"
	test "$output" = "0 1 1 1"
}

@test "gpioget: no arguments" {
	run_tool gpioget

	test "$status" -eq "1"
	output_regex_match ".*gpiochip must be specified"
}

@test "gpioget: no lines specified" {
	gpio_mockup_probe 8 8 8

	run_tool gpioget "$(gpio_mockup_chip_name 1)"

	test "$status" -eq "1"
	output_regex_match ".*at least one GPIO line offset must be specified"
}

@test "gpioget: too many lines specified" {
	gpio_mockup_probe 4

	run_tool gpioget "$(gpio_mockup_chip_name 1)" 0 1 2 3 4

	test "$status" -eq "1"
	output_regex_match ".*unable to retrieve GPIO lines from chip"
}

@test "gpioget: same line twice" {
	gpio_mockup_probe 8 8 8

	run_tool gpioget "$(gpio_mockup_chip_name 1)" 0 0

	test "$status" -eq "1"
	output_regex_match ".*unable to request lines.*"
}

@test "gpioget: invalid bias" {
	gpio_mockup_probe 8 8 8

	run_tool gpioget --bias=bad "$(gpio_mockup_chip_name 1)" 0 1

	test "$status" -eq "1"
	output_regex_match ".*invalid bias.*"
}

#
# gpioset test cases
#

@test "gpioset: set lines and wait for SIGTERM" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpioset --mode=signal "$(gpio_mockup_chip_name 2)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpio_mockup_check_value 2 0 0
	gpio_mockup_check_value 2 1 0
	gpio_mockup_check_value 2 2 1
	gpio_mockup_check_value 2 3 1
	gpio_mockup_check_value 2 4 1
	gpio_mockup_check_value 2 5 1
	gpio_mockup_check_value 2 6 0
	gpio_mockup_check_value 2 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (active-low)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpioset --active-low --mode=signal "$(gpio_mockup_chip_name 2)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpio_mockup_check_value 2 0 1
	gpio_mockup_check_value 2 1 1
	gpio_mockup_check_value 2 2 0
	gpio_mockup_check_value 2 3 0
	gpio_mockup_check_value 2 4 0
	gpio_mockup_check_value 2 5 0
	gpio_mockup_check_value 2 6 1
	gpio_mockup_check_value 2 7 0

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (push-pull)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpioset --drive=push-pull --mode=signal "$(gpio_mockup_chip_name 2)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpio_mockup_check_value 2 0 0
	gpio_mockup_check_value 2 1 0
	gpio_mockup_check_value 2 2 1
	gpio_mockup_check_value 2 3 1
	gpio_mockup_check_value 2 4 1
	gpio_mockup_check_value 2 5 1
	gpio_mockup_check_value 2 6 0
	gpio_mockup_check_value 2 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (open-drain)" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 2 2 1
	gpio_mockup_set_pull 2 3 1
	gpio_mockup_set_pull 2 5 1
	gpio_mockup_set_pull 2 7 1

	coproc_run_tool gpioset --drive=open-drain --mode=signal "$(gpio_mockup_chip_name 2)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpio_mockup_check_value 2 0 0
	gpio_mockup_check_value 2 1 0
	gpio_mockup_check_value 2 2 1
	gpio_mockup_check_value 2 3 1
	gpio_mockup_check_value 2 4 0
	gpio_mockup_check_value 2 5 1
	gpio_mockup_check_value 2 6 0
	gpio_mockup_check_value 2 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (open-source)" {
	gpio_mockup_probe 8 8 8

	gpio_mockup_set_pull 2 2 1
	gpio_mockup_set_pull 2 3 1
	gpio_mockup_set_pull 2 5 1
	gpio_mockup_set_pull 2 7 1

	coproc_run_tool gpioset --drive=open-source --mode=signal "$(gpio_mockup_chip_name 2)" \
					0=0 1=0 2=1 3=0 4=1 5=1 6=0 7=1

	gpio_mockup_check_value 2 0 0
	gpio_mockup_check_value 2 1 0
	gpio_mockup_check_value 2 2 1
	gpio_mockup_check_value 2 3 1
	gpio_mockup_check_value 2 4 1
	gpio_mockup_check_value 2 5 1
	gpio_mockup_check_value 2 6 0
	gpio_mockup_check_value 2 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set some lines and wait for ENTER" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpioset --mode=wait "$(gpio_mockup_chip_name 2)" \
					1=0 2=1 5=1 6=0 7=1

	gpio_mockup_check_value 2 1 0
	gpio_mockup_check_value 2 2 1
	gpio_mockup_check_value 2 5 1
	gpio_mockup_check_value 2 6 0
	gpio_mockup_check_value 2 7 1

	coproc_tool_stdin_write ""
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set some lines and wait for SIGINT" {
	gpio_mockup_probe 8

	coproc_run_tool gpioset --mode=signal "$(gpio_mockup_chip_name 0)" 0=1

	gpio_mockup_check_value 0 0 1

	coproc_tool_kill -SIGINT
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set some lines and wait with --mode=time" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpioset --mode=time --sec=1 --usec=200000 \
				"$(gpio_mockup_chip_name 1)" 0=1 5=0 7=1

	gpio_mockup_check_value 1 0 1
	gpio_mockup_check_value 1 5 0
	gpio_mockup_check_value 1 7 1

	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: no arguments" {
	run_tool gpioset

	test "$status" -eq "1"
	output_regex_match ".*gpiochip must be specified"
}

@test "gpioset: no lines specified" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset "$(gpio_mockup_chip_name 1)"

	test "$status" -eq "1"
	output_regex_match ".*at least one GPIO line offset to value mapping must be specified"
}

@test "gpioset: too many lines specified" {
	gpio_mockup_probe 4

	run_tool gpioset "$(gpio_mockup_chip_name 1)" 0=1 1=1 2=1 3=1 4=1 5=1

	test "$status" -eq "1"
	output_regex_match ".*unable to retrieve GPIO lines from chip"
}

@test "gpioset: use --sec without --mode=time" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset --mode=exit --sec=1 "$(gpio_mockup_chip_name 1)" 0=1

	test "$status" -eq "1"
	output_regex_match ".*can't specify wait time in this mode"
}

@test "gpioset: use --usec without --mode=time" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset --mode=exit --usec=1 "$(gpio_mockup_chip_name 1)" 0=1

	test "$status" -eq "1"
	output_regex_match ".*can't specify wait time in this mode"
}

@test "gpioset: invalid mapping" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset "$(gpio_mockup_chip_name 1)" 0=c

	test "$status" -eq "1"
	output_regex_match ".*invalid offset<->value mapping"
}

@test "gpioset: invalid value" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset "$(gpio_mockup_chip_name 1)" 0=3

	test "$status" -eq "1"
	output_regex_match ".*value must be 0 or 1"
}

@test "gpioset: invalid offset" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset "$(gpio_mockup_chip_name 1)" 4000000000=0

	test "$status" -eq "1"
	output_regex_match ".*invalid offset"
}

@test "gpioset: invalid bias" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset --bias=bad "$(gpio_mockup_chip_name 1)" 0=1 1=1

	test "$status" -eq "1"
	output_regex_match ".*invalid bias.*"
}

@test "gpioset: invalid drive" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset --drive=bad "$(gpio_mockup_chip_name 1)" 0=1 1=1

	test "$status" -eq "1"
	output_regex_match ".*invalid drive.*"
}

@test "gpioset: daemonize in invalid mode" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset --background "$(gpio_mockup_chip_name 1)" 0=1

	test "$status" -eq "1"
	output_regex_match ".*can't daemonize in this mode"
}

@test "gpioset: same line twice" {
	gpio_mockup_probe 8 8 8

	run_tool gpioset "$(gpio_mockup_chip_name 1)" 0=1 0=1

	test "$status" -eq "1"
	output_regex_match ".*unable to request lines.*"
}

#
# gpiomon test cases
#

@test "gpiomon: single rising edge event" {
	gpio_mockup_probe 8 8

	coproc_run_tool gpiomon --rising-edge "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+RISING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single falling edge event" {
	gpio_mockup_probe 8 8

	coproc_run_tool gpiomon --falling-edge "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	gpio_mockup_set_pull 1 4 0
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+FALLING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single falling edge event (pull-up)" {
	gpio_mockup_probe 8 8

	gpio_mockup_set_pull 1 4 0

	coproc_run_tool gpiomon --bias=pull-up "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 0
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+FALLING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single rising edge event (pull-down)" {
	gpio_mockup_probe 8 8

	gpio_mockup_set_pull 1 4 1

	coproc_run_tool gpiomon --bias=pull-down "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+RISING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single rising edge event (active-low)" {
	gpio_mockup_probe 8 8

	gpio_mockup_set_pull 1 4 1

	coproc_run_tool gpiomon --rising-edge --active-low "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 0
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+RISING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single rising edge event (silent mode)" {
	gpio_mockup_probe 8 8

	coproc_run_tool gpiomon --rising-edge --silent "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test -z "$output"
}

@test "gpiomon: four alternating events" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon --num-events=4 "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2
	gpio_mockup_set_pull 1 4 0
	sleep 0.2
	gpio_mockup_set_pull 1 4 1
	sleep 0.2
	gpio_mockup_set_pull 1 4 0
	sleep 0.2

	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event\\:\\s+FALLING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[\s*[0-9]+\\.[0-9]+\\]"
	output_regex_match \
"event\\:\\s+RISING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[\s*[0-9]+\\.[0-9]+\\]"
}

@test "gpiomon: exit after SIGINT" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "$(gpio_mockup_chip_name 1)" 4

	coproc_tool_kill -SIGINT
	coproc_tool_wait

	test "$status" -eq "0"
	test -z "$output"
}

@test "gpiomon: exit after SIGTERM" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "$(gpio_mockup_chip_name 1)" 4

	coproc_tool_kill -SIGTERM
	coproc_tool_wait

	test "$status" -eq "0"
	test -z "$output"
}

@test "gpiomon: both event flags" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon --falling-edge --rising-edge "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2
	gpio_mockup_set_pull 1 4 0
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event\\:\\s+FALLING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[\s*[0-9]+\\.[0-9]+\\]"
	output_regex_match \
"event\\:\\s+RISING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[\s*[0-9]+\\.[0-9]+\\]"
}

@test "gpiomon: watch multiple lines" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon --format=%o "$(gpio_mockup_chip_name 1)" 1 2 3 4 5

	gpio_mockup_set_pull 1 2 1
	gpio_mockup_set_pull 1 3 1
	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "${lines[0]}" = "2"
	test "${lines[1]}" = "3"
	test "${lines[2]}" = "4"
}

@test "gpiomon: watch multiple lines (lines in mixed-up order)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon --format=%o "$(gpio_mockup_chip_name 1)" 5 2 7 1 6

	gpio_mockup_set_pull 1 2 1
	gpio_mockup_set_pull 1 1 1
	gpio_mockup_set_pull 1 6 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "${lines[0]}" = "2"
	test "${lines[1]}" = "1"
	test "${lines[2]}" = "6"
}

@test "gpiomon: same line twice" {
	gpio_mockup_probe 8 8 8

	run_tool gpiomon "$(gpio_mockup_chip_name 1)" 0 0

	test "$status" -eq "1"
	output_regex_match ".*unable to request GPIO lines for events"
}

@test "gpiomon: no arguments" {
	run_tool gpiomon

	test "$status" -eq "1"
	output_regex_match ".*gpiochip must be specified"
}

@test "gpiomon: line not specified" {
	gpio_mockup_probe 8 8 8

	run_tool gpiomon "$(gpio_mockup_chip_name 1)"

	test "$status" -eq "1"
	output_regex_match ".*GPIO line offset must be specified"
}

@test "gpiomon: line out of range" {
	gpio_mockup_probe 4

	run_tool gpiomon "$(gpio_mockup_chip_name 0)" 5

	test "$status" -eq "1"
	output_regex_match ".*unable to retrieve GPIO lines from chip"
}

@test "gpiomon: invalid bias" {
	gpio_mockup_probe 8 8 8

	run_tool gpiomon --bias=bad "$(gpio_mockup_chip_name 1)" 0 1

	test "$status" -eq "1"
	output_regex_match ".*invalid bias.*"
}

@test "gpiomon: custom format (event type + offset)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%e %o" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "1 4"
}

@test "gpiomon: custom format (event type + offset joined)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%e%o" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "14"
}

@test "gpiomon: custom format (timestamp)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%e %o %s.%n" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match "1 4 [0-9]+\\.[0-9]+"
}

@test "gpiomon: custom format (double percent sign)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%%" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%"
}

@test "gpiomon: custom format (double percent sign + event type specifier)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%%e" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%e"
}

@test "gpiomon: custom format (single percent sign)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%"
}

@test "gpiomon: custom format (single percent sign between other characters)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=foo % bar" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "foo % bar"
}

@test "gpiomon: custom format (unknown specifier)" {
	gpio_mockup_probe 8 8 8

	coproc_run_tool gpiomon "--format=%x" "$(gpio_mockup_chip_name 1)" 4

	gpio_mockup_set_pull 1 4 1
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%x"
}
