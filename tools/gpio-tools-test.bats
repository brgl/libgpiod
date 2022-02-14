#!/usr/bin/env bats
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

# Simple test harness for the gpio-tools.

# Where output from coprocesses is stored
COPROC_OUTPUT=$BATS_TMPDIR/gpio-tools-test-output
# Save the PID of coprocess - otherwise we won't be able to wait for it
# once it exits as the COPROC_PID will be cleared.
COPROC_SAVED_PID=""

GPIOSIM_CHIPS=""
GPIOSIM_CONFIGFS="/sys/kernel/config/gpio-sim/"
GPIOSIM_SYSFS="/sys/devices/platform/"

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

random_name() {
	cat /proc/sys/kernel/random/uuid
}

gpiosim_chip() {
	local VAR=$1
	local NAME=$(random_name)
	local DEVPATH=$GPIOSIM_CONFIGFS/$NAME
	local BANKPATH=$DEVPATH/$NAME

	mkdir -p $BANKPATH

	for ARG in $*
	do
		local KEY=$(echo $ARG | cut -d"=" -f1)
		local VAL=$(echo $ARG | cut -d"=" -f2)

		if [ "$KEY" = "num_lines" ]
		then
			echo $VAL > $BANKPATH/num_lines
		elif [ "$KEY" = "line_name" ]
		then
			local OFFSET=$(echo $VAL | cut -d":" -f1)
			local LINENAME=$(echo $VAL | cut -d":" -f2)
			local LINEPATH=$BANKPATH/line$OFFSET

			mkdir -p $LINEPATH
			echo $LINENAME > $LINEPATH/name
		fi
	done

	echo 1 > $DEVPATH/live

	GPIOSIM_CHIPS="$VAR:$NAME $GPIOSIM_CHIPS"
}

gpiosim_chip_map_name() {
	local VAR=$1

	for CHIP in $GPIOSIM_CHIPS
	do
		KEY=$(echo $CHIP | cut -d":" -f1)
		VAL=$(echo $CHIP | cut -d":" -f2)

		if [ "$KEY" = "$VAR" ]
		then
			echo $VAL
		fi
	done
}

gpiosim_chip_name() {
	local VAR=$1
	local NAME=$(gpiosim_chip_map_name $VAR)

	cat $GPIOSIM_CONFIGFS/$NAME/$NAME/chip_name
}

gpiosim_dev_name() {
	local VAR=$1
	local NAME=$(gpiosim_chip_map_name $VAR)

	cat $GPIOSIM_CONFIGFS/$NAME/dev_name
}

gpiosim_set_pull() {
	local VAR=$1
	local OFFSET=$2
	local PULL=$3
	local DEVNAME=$(gpiosim_dev_name $VAR)
	local CHIPNAME=$(gpiosim_chip_name $VAR)

	echo $PULL > $GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/pull
}

gpiosim_check_value() {
	local VAR=$1
	local OFFSET=$2
	local EXPECTED=$3
	local DEVNAME=$(gpiosim_dev_name $VAR)
	local CHIPNAME=$(gpiosim_chip_name $VAR)

	VAL=$(cat $GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/value)
	if [ "$VAL" = "$EXPECTED" ]
	then
		return 0
	fi

	return 1
}

gpiosim_cleanup() {
	for CHIP in $GPIOSIM_CHIPS
	do
		local NAME=$(echo $CHIP | cut -d":" -f2)

		local DEVPATH=$GPIOSIM_CONFIGFS/$NAME
		local BANKPATH=$DEVPATH/$NAME

		echo 0 > $DEVPATH/live

		ls $BANKPATH/line* 2> /dev/null
		if [ "$?" = "0" ]
		then
			for LINE in $(find $BANKPATH/ | egrep "line[0-9]+$")
			do
				test -e $LINE/hog && rmdir $LINE/hog
				rmdir $LINE
			done
		fi

		rmdir $BANKPATH
		rmdir $DEVPATH
	done

	GPIOSIM_CHIPS=""
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

teardown() {
	if [ -n "$BG_PROC_PID" ]
	then
		kill -9 $BG_PROC_PID
		run wait $BG_PROC_PID
		BG_PROC_PID=""
	fi

	gpiosim_cleanup
}

#
# gpiodetect test cases
#

@test "gpiodetect: list chips" {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8
	gpiosim_chip sim2 num_lines=16

	run_tool gpiodetect

	test "$status" -eq 0
	output_contains_line "$(gpiosim_chip_name sim0) [$(gpiosim_dev_name sim0)-node0] (4 lines)"
	output_contains_line "$(gpiosim_chip_name sim1) [$(gpiosim_dev_name sim1)-node0] (8 lines)"
	output_contains_line "$(gpiosim_chip_name sim2) [$(gpiosim_dev_name sim2)-node0] (16 lines)"
}

@test "gpiodetect: invalid args" {
	run_tool gpiodetect unimplemented-arg
	test "$status" -eq 1
}

#
# gpioinfo test cases
#

@test "gpioinfo: dump all chips" {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8

	run_tool gpioinfo

	test "$status" -eq 0
	output_contains_line "$(gpiosim_chip_name sim0) - 4 lines:"
	output_contains_line "$(gpiosim_chip_name sim1) - 8 lines:"

	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+input\\s+active-high"
}

@test "gpioinfo: dump all chips with one line exported" {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8

	coproc_run_tool gpioset --mode=signal --active-low "$(gpiosim_chip_name sim1)" 7=1

	run_tool gpioinfo

	test "$status" -eq 0
	output_contains_line "$(gpiosim_chip_name sim0) - 4 lines:"
	output_contains_line "$(gpiosim_chip_name sim1) - 8 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+\\\"gpioset\\\"\\s+output\\s+active-low"

	coproc_tool_kill
	coproc_tool_wait
}

@test "gpioinfo: dump one chip" {
	gpiosim_chip sim0 num_lines=8
	gpiosim_chip sim1 num_lines=4

	run_tool gpioinfo "$(gpiosim_chip_name sim1)"

	test "$status" -eq 0
	assert_fail output_contains_line "$(gpiosim_chip_name sim0) - 8 lines:"
	output_contains_line "$(gpiosim_chip_name sim1) - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+unused\\s+input\\s+active-high"
	assert_fail output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+input\\s+active-high"
}

@test "gpioinfo: dump all but one chip" {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=4
	gpiosim_chip sim2 num_lines=8
	gpiosim_chip sim3 num_lines=4

	run_tool gpioinfo "$(gpiosim_chip_name sim0)" \
			"$(gpiosim_chip_name sim1)" "$(gpiosim_chip_name sim3)"

	test "$status" -eq 0
	output_contains_line "$(gpiosim_chip_name sim0) - 4 lines:"
	output_contains_line "$(gpiosim_chip_name sim1) - 4 lines:"
	assert_fail output_contains_line "$(gpiosim_chip_name sim2) - 8 lines:"
	output_contains_line "$(gpiosim_chip_name sim3) - 4 lines:"
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
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=3:bar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	run_tool gpiofind foobar

	test "$status" -eq "0"
	test "$output" = "$(gpiosim_chip_name sim1) 7"
}

@test "gpiofind: line not found" {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8
	gpiosim_chip sim2 num_lines=16

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
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	run_tool gpioget "$(gpiosim_chip_name sim0)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "0 0 1 1 0 1 0 1"
}

@test "gpioget: read all lines (active-low)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	run_tool gpioget --active-low "$(gpiosim_chip_name sim0)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "1 1 0 0 1 0 1 0"
}

@test "gpioget: read all lines (pull-up)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	run_tool gpioget --bias=pull-up "$(gpiosim_chip_name sim0)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "1 1 1 1 1 1 1 1"
}

@test "gpioget: read all lines (pull-down)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	run_tool gpioget --bias=pull-down "$(gpiosim_chip_name sim0)" 0 1 2 3 4 5 6 7

	test "$status" -eq "0"
	test "$output" = "0 0 0 0 0 0 0 0"
}

@test "gpioget: read some lines" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 1 pull-up
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 6 pull-up

	run_tool gpioget "$(gpiosim_chip_name sim0)" 0 1 4 6

	test "$status" -eq "0"
	test "$output" = "0 1 1 1"
}

@test "gpioget: no arguments" {
	run_tool gpioget

	test "$status" -eq "1"
	output_regex_match ".*gpiochip must be specified"
}

@test "gpioget: no lines specified" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioget "$(gpiosim_chip_name sim0)"

	test "$status" -eq "1"
	output_regex_match ".*at least one GPIO line offset must be specified"
}

@test "gpioget: too many lines specified" {
	gpiosim_chip sim0 num_lines=4

	run_tool gpioget "$(gpiosim_chip_name sim0)" 0 1 2 3 4

	test "$status" -eq "1"
	output_regex_match ".*unable to retrieve GPIO lines from chip"
}

@test "gpioget: same line twice" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioget "$(gpiosim_chip_name sim0)" 0 0

	test "$status" -eq "1"
	output_regex_match ".*unable to request lines.*"
}

@test "gpioget: invalid bias" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioget --bias=bad "$(gpiosim_chip_name sim0)" 0 1

	test "$status" -eq "1"
	output_regex_match ".*invalid bias.*"
}

#
# gpioset test cases
#

@test "gpioset: set lines and wait for SIGTERM" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpioset --mode=signal "$(gpiosim_chip_name sim0)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (active-low)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpioset --active-low --mode=signal "$(gpiosim_chip_name sim0)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 1
	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 2 0
	gpiosim_check_value sim0 3 0
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 5 0
	gpiosim_check_value sim0 6 1
	gpiosim_check_value sim0 7 0

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (push-pull)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpioset --drive=push-pull --mode=signal "$(gpiosim_chip_name sim0)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (open-drain)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	coproc_run_tool gpioset --drive=open-drain --mode=signal "$(gpiosim_chip_name sim0)" \
					0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set lines and wait for SIGTERM (open-source)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	coproc_run_tool gpioset --drive=open-source --mode=signal "$(gpiosim_chip_name sim0)" \
					0=0 1=0 2=1 3=0 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set some lines and wait for ENTER" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpioset --mode=wait "$(gpiosim_chip_name sim0)" \
					1=0 2=1 5=1 6=0 7=1

	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1

	coproc_tool_stdin_write ""
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set some lines and wait for SIGINT" {
	gpiosim_chip sim0 num_lines=4

	coproc_run_tool gpioset --mode=signal "$(gpiosim_chip_name sim0)" 0=1

	gpiosim_check_value sim0 0 1

	coproc_tool_kill -SIGINT
	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: set some lines and wait with --mode=time" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpioset --mode=time --sec=1 --usec=200000 \
				"$(gpiosim_chip_name sim0)" 0=1 5=0 7=1

	gpiosim_check_value sim0 0 1
	gpiosim_check_value sim0 5 0
	gpiosim_check_value sim0 7 1

	coproc_tool_wait

	test "$status" -eq "0"
}

@test "gpioset: no arguments" {
	run_tool gpioset

	test "$status" -eq "1"
	output_regex_match ".*gpiochip must be specified"
}

@test "gpioset: no lines specified" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset "$(gpiosim_chip_name sim1)"

	test "$status" -eq "1"
	output_regex_match ".*at least one GPIO line offset to value mapping must be specified"
}

@test "gpioset: too many lines specified" {
	gpiosim_chip sim0 num_lines=4

	run_tool gpioset "$(gpiosim_chip_name sim0)" 0=1 1=1 2=1 3=1 4=1 5=1

	test "$status" -eq "1"
	output_regex_match ".*unable to retrieve GPIO lines from chip"
}

@test "gpioset: use --sec without --mode=time" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --mode=exit --sec=1 "$(gpiosim_chip_name sim0)" 0=1

	test "$status" -eq "1"
	output_regex_match ".*can't specify wait time in this mode"
}

@test "gpioset: use --usec without --mode=time" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --mode=exit --usec=1 "$(gpiosim_chip_name sim1)" 0=1

	test "$status" -eq "1"
	output_regex_match ".*can't specify wait time in this mode"
}

@test "gpioset: default mode" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset "$(gpiosim_chip_name sim0)" 0=1

	test "$status" -eq "0"
}

@test "gpioset: invalid mapping" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset "$(gpiosim_chip_name sim1)" 0=c

	test "$status" -eq "1"
	output_regex_match ".*invalid offset<->value mapping"
}

@test "gpioset: invalid value" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset "$(gpiosim_chip_name sim1)" 0=3

	test "$status" -eq "1"
	output_regex_match ".*value must be 0 or 1"
}

@test "gpioset: invalid offset" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset "$(gpiosim_chip_name sim1)" 4000000000=0

	test "$status" -eq "1"
	output_regex_match ".*invalid offset"
}

@test "gpioset: invalid bias" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --bias=bad "$(gpiosim_chip_name sim1)" 0=1 1=1

	test "$status" -eq "1"
	output_regex_match ".*invalid bias.*"
}

@test "gpioset: invalid drive" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --drive=bad "$(gpiosim_chip_name sim1)" 0=1 1=1

	test "$status" -eq "1"
	output_regex_match ".*invalid drive.*"
}

@test "gpioset: daemonize in invalid mode" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --background "$(gpiosim_chip_name sim1)" 0=1

	test "$status" -eq "1"
	output_regex_match ".*can't daemonize in this mode"
}

@test "gpioset: same line twice" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset "$(gpiosim_chip_name sim0)" 0=1 0=1

	test "$status" -eq "1"
	output_regex_match ".*unable to request lines.*"
}

#
# gpiomon test cases
#

@test "gpiomon: single rising edge event" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --rising-edge "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+RISING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single falling edge event" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --falling-edge "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+FALLING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single falling edge event (pull-up)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 4 pull-down

	coproc_run_tool gpiomon --bias=pull-up "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-down
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+FALLING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single rising edge event (pull-down)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 4 pull-up

	coproc_run_tool gpiomon --bias=pull-down "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+RISING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single rising edge event (active-low)" {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 4 pull-up

	coproc_run_tool gpiomon --rising-edge --active-low "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-down
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event:\\s+RISING\\s+EDGE\\s+offset:\\s+4\\s+timestamp:\\s+\[\s*[0-9]+\.[0-9]+\]"
}

@test "gpiomon: single rising edge event (silent mode)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --rising-edge --silent "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test -z "$output"
}

@test "gpiomon: four alternating events" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --num-events=4 "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2
	gpiosim_set_pull sim0 4 pull-down
	sleep 0.2
	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2
	gpiosim_set_pull sim0 4 pull-down
	sleep 0.2

	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match \
"event\\:\\s+FALLING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[\s*[0-9]+\\.[0-9]+\\]"
	output_regex_match \
"event\\:\\s+RISING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[\s*[0-9]+\\.[0-9]+\\]"
}

@test "gpiomon: exit after SIGINT" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "$(gpiosim_chip_name sim0)" 4

	coproc_tool_kill -SIGINT
	coproc_tool_wait

	test "$status" -eq "0"
	test -z "$output"
}

@test "gpiomon: exit after SIGTERM" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "$(gpiosim_chip_name sim0)" 4

	coproc_tool_kill -SIGTERM
	coproc_tool_wait

	test "$status" -eq "0"
	test -z "$output"
}

@test "gpiomon: both event flags" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --falling-edge --rising-edge "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2
	gpiosim_set_pull sim0 4 pull-down
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
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --format=%o "$(gpiosim_chip_name sim0)" 1 2 3 4 5

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "${lines[0]}" = "2"
	test "${lines[1]}" = "3"
	test "${lines[2]}" = "4"
}

@test "gpiomon: watch multiple lines (lines in mixed-up order)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon --format=%o "$(gpiosim_chip_name sim0)" 5 2 7 1 6

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 1 pull-up
	gpiosim_set_pull sim0 6 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "${lines[0]}" = "2"
	test "${lines[1]}" = "1"
	test "${lines[2]}" = "6"
}

@test "gpiomon: same line twice" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpiomon "$(gpiosim_chip_name sim0)" 0 0

	test "$status" -eq "1"
	output_regex_match ".*unable to request GPIO lines for events"
}

@test "gpiomon: no arguments" {
	run_tool gpiomon

	test "$status" -eq "1"
	output_regex_match ".*gpiochip must be specified"
}

@test "gpiomon: line not specified" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpiomon "$(gpiosim_chip_name sim0)"

	test "$status" -eq "1"
	output_regex_match ".*GPIO line offset must be specified"
}

@test "gpiomon: line out of range" {
	gpiosim_chip sim0 num_lines=4

	run_tool gpiomon "$(gpiosim_chip_name sim0)" 5

	test "$status" -eq "1"
	output_regex_match ".*unable to retrieve GPIO lines from chip"
}

@test "gpiomon: invalid bias" {
	gpiosim_chip sim0 num_lines=8

	run_tool gpiomon --bias=bad "$(gpiosim_chip_name sim0)" 0 1

	test "$status" -eq "1"
	output_regex_match ".*invalid bias.*"
}

@test "gpiomon: custom format (event type + offset)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%e %o" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "1 4"
}

@test "gpiomon: custom format (event type + offset joined)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%e%o" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "14"
}

@test "gpiomon: custom format (timestamp)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%e %o %s.%n" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	output_regex_match "1 4 [0-9]+\\.[0-9]+"
}

@test "gpiomon: custom format (double percent sign)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%%" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%"
}

@test "gpiomon: custom format (double percent sign + event type specifier)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%%e" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%e"
}

@test "gpiomon: custom format (single percent sign)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%"
}

@test "gpiomon: custom format (single percent sign between other characters)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=foo % bar" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "foo % bar"
}

@test "gpiomon: custom format (unknown specifier)" {
	gpiosim_chip sim0 num_lines=8

	coproc_run_tool gpiomon "--format=%x" "$(gpiosim_chip_name sim0)" 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.2

	coproc_tool_kill
	coproc_tool_wait

	test "$status" -eq "0"
	test "$output" = "%x"
}
