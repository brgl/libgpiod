#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
# SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>
# SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

# Simple test harness for the gpio-tools.

# Where output from the dut is stored (must be used together
# with SHUNIT_TMPDIR).
DUT_OUTPUT=gpio-tools-test-output

# Save the PID of coprocess - otherwise we won't be able to wait for it
# once it exits as the COPROC_PID will be cleared.
DUT_PID=""

SOURCE_DIR="$(dirname ${BASH_SOURCE[0]})"

# mappings from local name to system chip name, path, dev name
declare -A GPIOSIM_CHIP_NAME
declare -A GPIOSIM_CHIP_PATH
declare -A GPIOSIM_DEV_NAME
GPIOSIM_CONFIGFS="/sys/kernel/config/gpio-sim"
GPIOSIM_SYSFS="/sys/devices/platform/"
GPIOSIM_APP_NAME="gpio-tools-test"

MIN_KERNEL_VERSION="5.17.4"
MIN_SHUNIT_VERSION="2.1.8"

# Run the command in $* and fail the test if the command succeeds.
assert_fail() {
	$* || return 0
	fail " '$*': command did not fail as expected"
}

# Check if the string in $2 matches against the pattern in $1.
regex_matches() {
	[[ $2 =~ $1 ]]
	assertEquals " '$2' did not match '$1':" "0" "$?"
}

output_contains_line() {
	assertContains "$1" "$output"
}

output_is() {
	assertEquals " output:" "$1" "$output"
}

num_lines_is() {
	[ "$1" -eq "0" ] || [ -z "$output" ] && return 0
	local NUM_LINES=$(echo "$output" | wc -l)
	assertEquals " number of lines:" "$1" "$NUM_LINES"
}

status_is() {
	assertEquals " status:" "$1" "$status"
}

# Same as above but match against the regex pattern in $1.
output_regex_match() {
	[[ "$output" =~ $1 ]]
	assertEquals "0" "$?"
}

gpiosim_chip() {
	local VAR=$1
	local NAME=${GPIOSIM_APP_NAME}-$$-${VAR}
	local DEVPATH=$GPIOSIM_CONFIGFS/$NAME
	local BANKPATH=$DEVPATH/bank0

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

	local chip_name=$(<$BANKPATH/chip_name)
	GPIOSIM_CHIP_NAME[$1]=$chip_name
	GPIOSIM_CHIP_PATH[$1]="/dev/$chip_name"
	GPIOSIM_DEV_NAME[$1]=$(<$DEVPATH/dev_name)
}

gpiosim_chip_number() {
	local NAME=${GPIOSIM_CHIP_NAME[$1]}
	echo ${NAME#"gpiochip"}
}

gpiosim_chip_symlink() {
	GPIOSIM_CHIP_LINK="$2/${GPIOSIM_APP_NAME}-$$-lnk"
	ln -s ${GPIOSIM_CHIP_PATH[$1]} "$GPIOSIM_CHIP_LINK"
}

gpiosim_chip_symlink_cleanup() {
	if [ -n "$GPIOSIM_CHIP_LINK" ]
	then
		rm "$GPIOSIM_CHIP_LINK"
	fi
	unset GPIOSIM_CHIP_LINK
}

gpiosim_set_pull() {
	local OFFSET=$2
	local PULL=$3
	local DEVNAME=${GPIOSIM_DEV_NAME[$1]}
	local CHIPNAME=${GPIOSIM_CHIP_NAME[$1]}

	echo $PULL > $GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/pull
}

gpiosim_check_value() {
	local OFFSET=$2
	local EXPECTED=$3
	local DEVNAME=${GPIOSIM_DEV_NAME[$1]}
	local CHIPNAME=${GPIOSIM_CHIP_NAME[$1]}

	VAL=$(<$GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/value)
	[ "$VAL" = "$EXPECTED" ]
}

gpiosim_wait_value() {
	local OFFSET=$2
	local EXPECTED=$3
	local DEVNAME=${GPIOSIM_DEV_NAME[$1]}
	local CHIPNAME=${GPIOSIM_CHIP_NAME[$1]}
	local PORT=$GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/value

	for i in {1..30}; do
		[ "$(<$PORT)" = "$EXPECTED" ] && return
		sleep 0.01
	done
	return 1
}

gpiosim_cleanup() {
	for CHIP in ${!GPIOSIM_CHIP_NAME[@]}
	do
		local NAME=${GPIOSIM_APP_NAME}-$$-$CHIP

		local DEVPATH=$GPIOSIM_CONFIGFS/$NAME
		local BANKPATH=$DEVPATH/bank0

		echo 0 > $DEVPATH/live

		ls $BANKPATH/line* > /dev/null 2>&1
		if [ "$?" = "0" ]
		then
			for LINE in $(find $BANKPATH/ | grep -E "line[0-9]+$")
			do
				test -e $LINE/hog && rmdir $LINE/hog
				rmdir $LINE
			done
		fi

		rmdir $BANKPATH
		rmdir $DEVPATH
	done

	gpiosim_chip_symlink_cleanup

	GPIOSIM_CHIP_NAME=()
	GPIOSIM_CHIP_PATH=()
	GPIOSIM_DEV_NAME=()
}

run_tool() {
	# Executables to test are expected to be in the same directory as the
	# testing script.
	output=$(timeout 10s $SOURCE_DIR/"$@" 2>&1)
	status=$?
}

dut_run() {
	coproc timeout 10s $SOURCE_DIR/"$@" 2>&1
	DUT_PID=$COPROC_PID
	read -t1 -n1 -u ${COPROC[0]} DUT_FIRST_CHAR
}

dut_run_redirect() {
	coproc timeout 10s $SOURCE_DIR/"$@" > $SHUNIT_TMPDIR/$DUT_OUTPUT 2>&1
	DUT_PID=$COPROC_PID
	# give the process time to spin up
	# FIXME - find a better solution
	sleep 0.2
}

dut_read_redirect() {
	output=$(<$SHUNIT_TMPDIR/$DUT_OUTPUT)
        local ORIG_IFS="$IFS"
        IFS=$'\n' lines=($output)
        IFS="$ORIG_IFS"
}

dut_read() {
	local LINE
	lines=()
	while read -t 0.2 -u ${COPROC[0]} LINE;
	do
		if [ -n "$DUT_FIRST_CHAR" ]
		then
			LINE=${DUT_FIRST_CHAR}${LINE}
			unset DUT_FIRST_CHAR
		fi
		lines+=("$LINE")
	done
	output="${lines[@]}"
}

dut_readable() {
	read -t 0 -u ${COPROC[0]} LINE
}

dut_flush() {
	local JUNK
	lines=()
	output=
	unset DUT_FIRST_CHAR
	while read -t 0 -u ${COPROC[0]} JUNK;
	do
		read -t 0.1 -u ${COPROC[0]} JUNK || true
	done
}

# check the next line of output matches the regex
dut_regex_match() {
	PATTERN=$1

	read -t 0.2 -u ${COPROC[0]} LINE || (echo Timeout && false)
	if [ -n "$DUT_FIRST_CHAR" ]
	then
		LINE=${DUT_FIRST_CHAR}${LINE}
		unset DUT_FIRST_CHAR
	fi
	[[ $LINE =~ $PATTERN ]]
	assertEquals "0" "$?"
}

dut_write() {
	echo $* >&${COPROC[1]}
}

dut_kill() {
	SIGNUM=$1

	kill $SIGNUM $DUT_PID
}

dut_wait() {
	wait $DUT_PID
	export status=$?
	unset DUT_PID
}

dut_cleanup() {
        if [ -n "$DUT_PID" ]
        then
		kill -SIGTERM $DUT_PID 2> /dev/null
		wait $DUT_PID || false
        fi
        rm -f $SHUNIT_TMPDIR/$DUT_OUTPUT
}

tearDown() {
	dut_cleanup
	gpiosim_cleanup
}

request_release_line() {
	$SOURCE_DIR/gpioget -c $* >/dev/null
}

#
# gpiodetect test cases
#

test_gpiodetect_all_chips() {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8
	gpiosim_chip sim2 num_lines=16

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}
	local sim2=${GPIOSIM_CHIP_NAME[sim2]}
	local sim0dev=${GPIOSIM_DEV_NAME[sim0]}
	local sim1dev=${GPIOSIM_DEV_NAME[sim1]}
	local sim2dev=${GPIOSIM_DEV_NAME[sim2]}

	run_tool gpiodetect

	output_contains_line "$sim0 [${sim0dev}-node0] (4 lines)"
	output_contains_line "$sim1 [${sim1dev}-node0] (8 lines)"
	output_contains_line "$sim2 [${sim2dev}-node0] (16 lines)"
	status_is 0

	# ignoring symlinks
	local initial_output=$output
	gpiosim_chip_symlink sim1 /dev

	run_tool gpiodetect

	output_is "$initial_output"
	status_is 0
}

test_gpiodetect_a_chip() {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8
	gpiosim_chip sim2 num_lines=16

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}
	local sim2=${GPIOSIM_CHIP_NAME[sim2]}
	local sim0dev=${GPIOSIM_DEV_NAME[sim0]}
	local sim1dev=${GPIOSIM_DEV_NAME[sim1]}
	local sim2dev=${GPIOSIM_DEV_NAME[sim2]}

	# by name
	run_tool gpiodetect $sim0

	output_contains_line "$sim0 [${sim0dev}-node0] (4 lines)"
	num_lines_is 1
	status_is 0

	# by path
	run_tool gpiodetect ${GPIOSIM_CHIP_PATH[sim1]}

	output_contains_line "$sim1 [${sim1dev}-node0] (8 lines)"
	num_lines_is 1
	status_is 0

	# by number
	run_tool gpiodetect $(gpiosim_chip_number sim2)

	output_contains_line "$sim2 [${sim2dev}-node0] (16 lines)"
	num_lines_is 1
	status_is 0

	# by symlink
	gpiosim_chip_symlink sim2 .
	run_tool gpiodetect $GPIOSIM_CHIP_LINK

	output_contains_line "$sim2 [${sim2dev}-node0] (16 lines)"
	num_lines_is 1
	status_is 0
}

test_gpiodetect_multiple_chips() {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8
	gpiosim_chip sim2 num_lines=16

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}
	local sim2=${GPIOSIM_CHIP_NAME[sim2]}
	local sim0dev=${GPIOSIM_DEV_NAME[sim0]}
	local sim1dev=${GPIOSIM_DEV_NAME[sim1]}
	local sim2dev=${GPIOSIM_DEV_NAME[sim2]}

	run_tool gpiodetect $sim0 $sim1 $sim2

	output_contains_line "$sim0 [${sim0dev}-node0] (4 lines)"
	output_contains_line "$sim1 [${sim1dev}-node0] (8 lines)"
	output_contains_line "$sim2 [${sim2dev}-node0] (16 lines)"
	num_lines_is 3
	status_is 0
}

test_gpiodetect_with_nonexistent_chip() {
	run_tool gpiodetect nonexistent-chip

	status_is 1
	output_regex_match \
".*cannot find GPIO chip character device 'nonexistent-chip'"
}

#
# gpioinfo test cases
#

test_gpioinfo_all_chips() {
	gpiosim_chip sim0 num_lines=4
	gpiosim_chip sim1 num_lines=8

	run_tool gpioinfo

	output_contains_line "${GPIOSIM_CHIP_NAME[sim0]} - 4 lines:"
	output_contains_line "${GPIOSIM_CHIP_NAME[sim1]} - 8 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+7:\\s+unnamed\\s+input"
	status_is 0

	# ignoring symlinks
	local initial_output=$output
	gpiosim_chip_symlink sim1 /dev

	run_tool gpioinfo

	output_is "$initial_output"
	status_is 0
}

test_gpioinfo_all_chips_with_some_used_lines() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=3:baz line_name=4:xyz

	dut_run gpioset --banner --active-low foo=1 baz=0

	run_tool gpioinfo

	output_contains_line "${GPIOSIM_CHIP_NAME[sim0]} - 4 lines:"
	output_contains_line "${GPIOSIM_CHIP_NAME[sim1]} - 8 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match \
"\\s+line\\s+1:\\s+\"foo\"\\s+output active-low consumer=\"gpioset\""
	output_regex_match \
"\\s+line\\s+3:\\s+\"baz\"\\s+output active-low consumer=\"gpioset\""
	status_is 0
}

test_gpioinfo_a_chip() {
	gpiosim_chip sim0 num_lines=8
	gpiosim_chip sim1 num_lines=4

	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	# by name
	run_tool gpioinfo --chip $sim1

	output_contains_line "$sim1 - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+1:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+2:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+3:\\s+unnamed\\s+input"
	num_lines_is 5
	status_is 0

	# by path
	run_tool gpioinfo --chip $sim1

	output_contains_line "$sim1 - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+1:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+2:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+3:\\s+unnamed\\s+input"
	num_lines_is 5
	status_is 0

	# by number
	run_tool gpioinfo --chip $sim1

	output_contains_line "$sim1 - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+1:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+2:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+3:\\s+unnamed\\s+input"
	num_lines_is 5
	status_is 0

	# by symlink
	gpiosim_chip_symlink sim1 .
	run_tool gpioinfo --chip $GPIOSIM_CHIP_LINK

	output_contains_line "$sim1 - 4 lines:"
	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+1:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+2:\\s+unnamed\\s+input"
	output_regex_match "\\s+line\\s+3:\\s+unnamed\\s+input"
	num_lines_is 5
	status_is 0
}

test_gpioinfo_a_line() {
	gpiosim_chip sim0 num_lines=8 line_name=5:bar
	gpiosim_chip sim1 num_lines=4 line_name=2:bar

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	# by offset
	run_tool gpioinfo --chip $sim1 2

	output_regex_match "$sim1 2\\s+\"bar\"\\s+input"
	num_lines_is 1
	status_is 0

	# by name
	run_tool gpioinfo bar

	output_regex_match "$sim0 5\\s+\"bar\"\\s+input"
	num_lines_is 1
	status_is 0

	# by chip and name
	run_tool gpioinfo --chip $sim1 2

	output_regex_match "$sim1 2\\s+\"bar\"\\s+input"
	num_lines_is 1
	status_is 0

	# unquoted
	run_tool gpioinfo --unquoted --chip $sim1 2

	output_regex_match "$sim1 2\\s+bar\\s+input"
	num_lines_is 1
	status_is 0

}

test_gpioinfo_first_matching_named_line() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioinfo foobar

	output_regex_match "$sim0 3\\s+\"foobar\"\\s+input"
	num_lines_is 1
	status_is 0
}

test_gpioinfo_multiple_lines() {
	gpiosim_chip sim0 num_lines=8 line_name=5:bar
	gpiosim_chip sim1 num_lines=4 line_name=2:baz

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	# by offset
	run_tool gpioinfo --chip $sim1 1 2

	output_regex_match "$sim1 1\\s+unnamed\\s+input"
	output_regex_match "$sim1 2\\s+\"baz\"\\s+input"
	num_lines_is 2
	status_is 0

	# by name
	run_tool gpioinfo bar baz

	output_regex_match "$sim0 5\\s+\"bar\"\\s+input"
	output_regex_match "$sim1 2\\s+\"baz\"\\s+input"
	num_lines_is 2
	status_is 0

	# by name and offset
	run_tool gpioinfo --chip $sim0 bar 3

	output_regex_match "$sim0 5\\s+\"bar\"\\s+input"
	output_regex_match "$sim0 3\\s+unnamed\\s+input"
	num_lines_is 2
	status_is 0
}

test_gpioinfo_line_attribute_menagerie() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo
	gpiosim_chip sim1 num_lines=8 line_name=3:baz

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	dut_run gpioset --banner --active-low --bias=pull-up --drive=open-source foo=1 baz=0

	run_tool gpioinfo foo baz

	output_regex_match \
"$sim0 1\\s+\"foo\"\\s+output active-low drive=open-source bias=pull-up consumer=\"gpioset\""
	output_regex_match \
"$sim1 3\\s+\"baz\"\\s+output active-low drive=open-source bias=pull-up consumer=\"gpioset\""
	num_lines_is 2
	status_is 0

	dut_kill
	dut_wait

	dut_run gpioset --banner --bias=pull-down --drive=open-drain foo=1 baz=0

	run_tool gpioinfo foo baz

	output_regex_match \
"$sim0 1\\s+\"foo\"\\s+output drive=open-drain bias=pull-down consumer=\"gpioset\""
	output_regex_match \
"$sim1 3\\s+\"baz\"\\s+output drive=open-drain bias=pull-down consumer=\"gpioset\""
	num_lines_is 2
	status_is 0

	dut_kill
	dut_wait

	dut_run gpiomon --banner --bias=disabled --utc -p 10ms foo baz

	run_tool gpioinfo foo baz

	output_regex_match \
"$sim0 1\\s+\"foo\"\\s+input bias=disabled edges=both event-clock=realtime debounce-period=10ms consumer=\"gpiomon\""
	output_regex_match \
"$sim1 3\\s+\"baz\"\\s+input bias=disabled edges=both event-clock=realtime debounce-period=10ms consumer=\"gpiomon\""
	num_lines_is 2
	status_is 0

	dut_kill
	dut_wait

	dut_run gpiomon --banner --edges=rising --localtime foo baz

	run_tool gpioinfo foo baz

	output_regex_match \
"$sim0 1\\s+\"foo\"\\s+input edges=rising event-clock=realtime consumer=\"gpiomon\""
	output_regex_match \
"$sim1 3\\s+\"baz\"\\s+input edges=rising event-clock=realtime consumer=\"gpiomon\""
	num_lines_is 2
	status_is 0

	dut_kill
	dut_wait

	dut_run gpiomon --banner --edges=falling foo baz

	run_tool gpioinfo foo baz

	output_regex_match \
"$sim0 1\\s+\"foo\"\\s+input edges=falling consumer=\"gpiomon\""
	output_regex_match \
"$sim1 3\\s+\"baz\"\\s+input edges=falling consumer=\"gpiomon\""
	num_lines_is 2
	status_is 0
}

test_gpioinfo_with_same_line_twice() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# by offset
	run_tool gpioinfo --chip $sim0 1 1

	output_regex_match "$sim0 1\\s+\"foo\"\\s+input"
	output_regex_match ".*lines '1' and '1' are the same line"
	num_lines_is 2
	status_is 1

	# by name
	run_tool gpioinfo foo foo

	output_regex_match "$sim0 1\\s+\"foo\"\\s+input"
	output_regex_match ".*lines 'foo' and 'foo' are the same line"
	num_lines_is 2
	status_is 1

	# by name and offset
	run_tool gpioinfo --chip $sim0 foo 1

	output_regex_match "$sim0 1\\s+\"foo\"\\s+input"
	output_regex_match ".*lines 'foo' and '1' are the same line"
	num_lines_is 2
	status_is 1
}

test_gpioinfo_all_lines_matching_name() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	run_tool gpioinfo --strict foobar

	output_regex_match "$sim0 3\\s+\"foobar\"\\s+input"
	output_regex_match "$sim1 2\\s+\"foobar\"\\s+input"
	output_regex_match "$sim1 7\\s+\"foobar\"\\s+input"
	num_lines_is 3
	status_is 1
}

test_gpioinfo_with_lines_strictly_by_name() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# first by offset (to show offsets match first)
	run_tool gpioinfo --chip $sim0 1 6

	output_regex_match "$sim0 1\\s+\"6\"\\s+input"
	output_regex_match "$sim0 6\\s+\"1\"\\s+input"
	num_lines_is 2
	status_is 0

	# then strictly by name
	run_tool gpioinfo --by-name --chip $sim0 1

	output_regex_match "$sim0 6\\s+\"1\"\\s+input"
	num_lines_is 1
	status_is 0
}

test_gpioinfo_with_nonexistent_chip() {
	run_tool gpioinfo --chip nonexistent-chip

	output_regex_match \
".*cannot find GPIO chip character device 'nonexistent-chip'"
	status_is 1
}

test_gpioinfo_with_nonexistent_line() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioinfo nonexistent-line

	output_regex_match ".*cannot find line 'nonexistent-line'"
	status_is 1

	run_tool gpioinfo --chip ${GPIOSIM_CHIP_NAME[sim0]} nonexistent-line

	output_regex_match ".*cannot find line 'nonexistent-line'"
	status_is 1
}

test_gpioinfo_with_offset_out_of_range() {
	gpiosim_chip sim0 num_lines=4

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioinfo --chip $sim0 0 1 2 3 4 5

	output_regex_match "$sim0 0\\s+unnamed\\s+input"
	output_regex_match "$sim0 1\\s+unnamed\\s+input"
	output_regex_match "$sim0 2\\s+unnamed\\s+input"
	output_regex_match "$sim0 3\\s+unnamed\\s+input"
	output_regex_match ".*offset 4 is out of range on chip '$sim0'"
	output_regex_match ".*offset 5 is out of range on chip '$sim0'"
	num_lines_is 6
	status_is 1
}

#
# gpioget test cases
#

test_gpioget_by_name() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	gpiosim_set_pull sim0 1 pull-up

	run_tool gpioget foo

	output_is "\"foo\"=active"
	status_is 0

	run_tool gpioget --unquoted foo

	output_is "foo=active"
	status_is 0
}

test_gpioget_by_offset() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 1 pull-up

	run_tool gpioget --chip ${GPIOSIM_CHIP_NAME[sim0]} 1

	output_is "\"1\"=active"
	status_is 0

	run_tool gpioget --unquoted --chip ${GPIOSIM_CHIP_NAME[sim0]} 1

	output_is "1=active"
	status_is 0
}

test_gpioget_by_symlink() {
	gpiosim_chip sim0 num_lines=8
	gpiosim_chip_symlink sim0 .

	gpiosim_set_pull sim0 1 pull-up

	run_tool gpioget --chip $GPIOSIM_CHIP_LINK 1

	output_is "\"1\"=active"
	status_is 0
}

test_gpioget_by_chip_and_name() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo
	gpiosim_chip sim1 num_lines=8 line_name=3:foo

	gpiosim_set_pull sim1 3 pull-up

	run_tool gpioget --chip ${GPIOSIM_CHIP_NAME[sim1]} foo

	output_is "\"foo\"=active"
	status_is 0

	run_tool gpioget --unquoted --chip ${GPIOSIM_CHIP_NAME[sim1]} foo

	output_is "foo=active"
	status_is 0
}

test_gpioget_first_matching_named_line() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar line_name=7:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz
	gpiosim_chip sim2 num_lines=16

	gpiosim_set_pull sim0 3 pull-up

	run_tool gpioget foobar

	output_is "\"foobar\"=active"
	status_is 0
}

test_gpioget_multiple_lines() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	run_tool gpioget --unquoted --chip ${GPIOSIM_CHIP_NAME[sim0]} 0 1 2 3 4 5 6 7

	output_is \
"0=inactive 1=inactive 2=active 3=active 4=inactive 5=active 6=inactive 7=active"
	status_is 0
}

test_gpioget_multiple_lines_by_name_and_offset() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=6:bar
	gpiosim_chip sim1 num_lines=8 line_name=1:baz line_name=3:bar
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 1 pull-up
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 6 pull-up

	run_tool gpioget --chip $sim0 0 foo 4 bar

	output_is "\"0\"=inactive \"foo\"=active \"4\"=active \"bar\"=active"
	status_is 0
}

test_gpioget_multiple_lines_across_multiple_chips() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=4:xyz

	gpiosim_set_pull sim0 1 pull-up
	gpiosim_set_pull sim1 4 pull-up

	run_tool gpioget baz bar foo xyz

	output_is "\"baz\"=inactive \"bar\"=inactive \"foo\"=active \"xyz\"=active"
	status_is 0
}

test_gpioget_with_numeric_values() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioget --numeric --chip $sim0 0 1 2 3 4 5 6 7

	output_is "0 0 1 1 0 1 0 1"
	status_is 0
}

test_gpioget_with_active_low() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioget --active-low --unquoted --chip $sim0 0 1 2 3 4 5 6 7

	output_is \
"0=active 1=active 2=inactive 3=inactive 4=active 5=inactive 6=active 7=inactive"
	status_is 0
}

test_gpioget_with_consumer() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=3:baz line_name=4:xyz

	dut_run gpionotify --banner -F "%l %E %C" foo baz

	run_tool gpioget --consumer gpio-tools-tests foo baz
	status_is 0

	dut_read
	output_regex_match "foo requested gpio-tools-tests"
	output_regex_match "baz requested gpio-tools-tests"
}

test_gpioget_with_pull_up() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioget --bias=pull-up --unquoted --chip $sim0 0 1 2 3 4 5 6 7

	output_is \
"0=active 1=active 2=active 3=active 4=active 5=active 6=active 7=active"
	status_is 0
}

test_gpioget_with_pull_down() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioget --bias=pull-down --unquoted --chip $sim0 0 1 2 3 4 5 6 7

	output_is \
"0=inactive 1=inactive 2=inactive 3=inactive 4=inactive 5=inactive 6=inactive 7=inactive"
	status_is 0
}

test_gpioget_with_direction_as_is() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# flip to output
	run_tool gpioset -t0 foo=1

	status_is 0

	run_tool gpioinfo foo
	output_regex_match "$sim0 1\\s+\"foo\"\\s+output"
	status_is 0

	run_tool gpioget --as-is foo
	# note gpio-sim reverts line to its pull when released
	output_is "\"foo\"=inactive"
	status_is 0

	run_tool gpioinfo foo
	output_regex_match "$sim0 1\\s+\"foo\"\\s+output"
	status_is 0

	# whereas the default behaviour forces to input
	run_tool gpioget foo
	# note gpio-sim reverts line to its pull when released
	# (defaults to pull-down)
	output_is "\"foo\"=inactive"
	status_is 0

	run_tool gpioinfo foo
	output_regex_match "$sim0 1\\s+\"foo\"\\s+input"
	status_is 0
}

test_gpioget_with_hold_period() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	# only test parsing - testing the hold-period itself is tricky
	run_tool gpioget --hold-period=100ms foo
	output_is "\"foo\"=inactive"
	status_is 0
}

test_gpioget_with_strict_named_line_check() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	run_tool gpioget --strict foobar

	output_regex_match ".*line 'foobar' is not unique"
	status_is 1
}

test_gpioget_with_lines_by_offset() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	gpiosim_set_pull sim0 1 pull-up
	gpiosim_set_pull sim0 6 pull-down

	run_tool gpioget --chip ${GPIOSIM_CHIP_NAME[sim0]} 1 6

	output_is "\"1\"=active \"6\"=inactive"
	status_is 0

	run_tool gpioget --unquoted --chip ${GPIOSIM_CHIP_NAME[sim0]} 1 6

	output_is "1=active 6=inactive"
	status_is 0
}

test_gpioget_with_lines_strictly_by_name() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	gpiosim_set_pull sim0 1 pull-up
	gpiosim_set_pull sim0 6 pull-down

	run_tool gpioget --by-name --chip ${GPIOSIM_CHIP_NAME[sim0]} 1 6

	output_is "\"1\"=inactive \"6\"=active"
	status_is 0

	run_tool gpioget --by-name --unquoted --chip ${GPIOSIM_CHIP_NAME[sim0]} 1 6

	output_is "1=inactive 6=active"
	status_is 0
}

test_gpioget_with_no_arguments() {
	run_tool gpioget

	output_regex_match ".*at least one GPIO line must be specified"
	status_is 1
}

test_gpioget_with_chip_but_no_line_specified() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioget --chip ${GPIOSIM_CHIP_NAME[sim0]}

	output_regex_match ".*at least one GPIO line must be specified"
	status_is 1
}

test_gpioget_with_offset_out_of_range() {
	gpiosim_chip sim0 num_lines=4
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioget --chip $sim0 0 1 2 3 4 5

	output_regex_match ".*offset 4 is out of range on chip '$sim0'"
	output_regex_match ".*offset 5 is out of range on chip '$sim0'"
	status_is 1
}

test_gpioget_with_nonexistent_line() {
	run_tool gpioget nonexistent-line

	output_regex_match ".*cannot find line 'nonexistent-line'"
	status_is 1
}

test_gpioget_with_same_line_twice() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# by offset
	run_tool gpioget --chip $sim0 0 0

	output_regex_match ".*lines '0' and '0' are the same line"
	status_is 1

	# by name
	run_tool gpioget foo foo

	output_regex_match ".*lines 'foo' and 'foo' are the same line"
	status_is 1

	# by chip and name
	run_tool gpioget --chip $sim0 foo foo

	output_regex_match ".*lines 'foo' and 'foo' are the same line"
	status_is 1

	# by name and offset
	run_tool gpioget --chip $sim0 foo 1

	output_regex_match ".*lines 'foo' and '1' are the same line"
	status_is 1

	# by offset and name
	run_tool gpioget --chip $sim0 1 foo

	output_regex_match ".*lines '1' and 'foo' are the same line"
	status_is 1
}

test_gpioget_with_invalid_bias() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioget --bias=bad --chip ${GPIOSIM_CHIP_NAME[sim0]} 0 1

	output_regex_match ".*invalid bias.*"
	status_is 1
}

test_gpioget_with_invalid_hold_period() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioget --hold-period=bad --chip ${GPIOSIM_CHIP_NAME[sim0]} 0

	output_regex_match ".*invalid period.*"
	status_is 1
}

#
# gpioset test cases
#

test_gpioset_by_name() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	dut_run gpioset --banner foo=1

	gpiosim_check_value sim0 1 1
}

test_gpioset_by_offset() {
	gpiosim_chip sim0 num_lines=8

	dut_run gpioset --banner --chip ${GPIOSIM_CHIP_NAME[sim0]} 1=1

	gpiosim_check_value sim0 1 1
}

test_gpioset_by_symlink() {
	gpiosim_chip sim0 num_lines=8
	gpiosim_chip_symlink sim0 .

	dut_run gpioset --banner --chip $GPIOSIM_CHIP_LINK 1=1

	gpiosim_check_value sim0 1 1
}

test_gpioset_by_chip_and_name() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo
	gpiosim_chip sim1 num_lines=8 line_name=3:foo

	dut_run gpioset --banner --chip ${GPIOSIM_CHIP_NAME[sim1]} foo=1

	gpiosim_check_value sim1 3 1
}

test_gpioset_first_matching_named_line() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	dut_run gpioset --banner foobar=1

	gpiosim_check_value sim0 3 1
}

test_gpioset_multiple_lines() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --banner --chip $sim0 0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_multiple_lines_by_name_and_offset() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar

	dut_run gpioset --banner --chip ${GPIOSIM_CHIP_NAME[sim0]} 0=1 foo=1 bar=1 3=1

	gpiosim_check_value sim0 0 1
	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
}


test_gpioset_multiple_lines_across_multiple_chips() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=4:xyz

	dut_run gpioset --banner foo=1 bar=1 baz=1 xyz=1

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim1 0 1
	gpiosim_check_value sim1 4 1
}

test_gpioset_with_active_low() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --banner --active-low -c $sim0 \
		0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 1
	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 2 0
	gpiosim_check_value sim0 3 0
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 5 0
	gpiosim_check_value sim0 6 1
	gpiosim_check_value sim0 7 0
}

test_gpioset_with_consumer() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=3:baz line_name=4:xyz

	dut_run gpioset --banner --consumer gpio-tools-tests foo=1 baz=0

	run_tool gpioinfo

	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match \
"\\s+line\\s+1:\\s+\"foo\"\\s+output consumer=\"gpio-tools-tests\""
	output_regex_match \
"\\s+line\\s+3:\\s+\"baz\"\\s+output consumer=\"gpio-tools-tests\""
	status_is 0
}

test_gpioset_with_push_pull() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --banner --drive=push-pull --chip $sim0 \
		0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_with_open_drain() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	dut_run gpioset --banner --drive=open-drain --chip $sim0 \
		0=0 1=0 2=1 3=1 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_with_open_source() {
	gpiosim_chip sim0 num_lines=8

	gpiosim_set_pull sim0 2 pull-up
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 7 pull-up

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --banner --drive=open-source --chip $sim0 \
		0=0 1=0 2=1 3=0 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_with_pull_up() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --banner --bias=pull-up --drive=open-drain \
		--chip $sim0 0=0 1=0 2=1 3=0 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_with_pull_down() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --banner --bias=pull-down --drive=open-source \
		--chip $sim0 0=0 1=0 2=1 3=0 4=1 5=1 6=0 7=1

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_with_value_variants() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 0 pull-up
	gpiosim_set_pull sim0 1 pull-down
	gpiosim_set_pull sim0 2 pull-down
	gpiosim_set_pull sim0 3 pull-up
	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 5 pull-up
	gpiosim_set_pull sim0 6 pull-up
	gpiosim_set_pull sim0 7 pull-down

	dut_run gpioset --banner --chip $sim0 0=0 1=1 2=active \
		3=inactive 4=on 5=off 6=false 7=true

	gpiosim_check_value sim0 0 0
	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 3 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 5 0
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1
}

test_gpioset_with_hold_period() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 5 pull-up

	dut_run gpioset --banner --hold-period=1200ms -t0 --chip $sim0 0=1 5=0 7=1

	gpiosim_check_value sim0 0 1
	gpiosim_check_value sim0 5 0
	gpiosim_check_value sim0 7 1

	dut_wait

	status_is 0
}

test_gpioset_interactive_exit() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpioset --interactive --chip $sim0 1=0 2=1 5=1 6=0 7=1

	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 2 1
	gpiosim_check_value sim0 5 1
	gpiosim_check_value sim0 6 0
	gpiosim_check_value sim0 7 1

	dut_write "exit"
	dut_wait

	status_is 0
}

test_gpioset_interactive_help() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	dut_run gpioset --interactive foo=1 bar=0 baz=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	dut_write "help"

	dut_read
	output_regex_match "COMMANDS:.*"
	output_regex_match ".*get \[line\]\.\.\..*"
	output_regex_match ".*set <line=value>\.\.\..*"
	output_regex_match ".*toggle \[line\]\.\.\..*"
	output_regex_match ".*sleep <period>.*"
}

test_gpioset_interactive_get() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	dut_run gpioset --interactive foo=1 bar=0 baz=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	dut_write "get"

	dut_read
	output_regex_match "\"foo\"=active \"bar\"=inactive \"baz\"=inactive"

	dut_write "get bar"

	dut_read
	output_regex_match "\"bar\"=inactive"
}

test_gpioset_interactive_get_unquoted() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	dut_run gpioset --interactive --unquoted foo=1 bar=0 baz=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	dut_write "get"

	dut_read
	output_regex_match "foo=active bar=inactive baz=inactive"

	dut_write "get bar"

	dut_read
	output_regex_match "bar=inactive"
}

test_gpioset_interactive_set() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	dut_run gpioset --interactive foo=1 bar=0 baz=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	dut_write "set bar=active"

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 7 0
	dut_write "get"
	dut_read
	output_regex_match "\"foo\"=active \"bar\"=active \"baz\"=inactive"
}

test_gpioset_interactive_toggle() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 7 pull-up

	dut_run gpioset -i foo=1 bar=0 baz=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	dut_write "toggle"

	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 7 1
	dut_write "get"
	dut_read
	output_regex_match "\"foo\"=inactive\\s+\"bar\"=active\\s+\"baz\"=active\\s*"

	dut_write "toggle baz"

	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 7 0
	dut_write "get"
	dut_read
	output_regex_match "\"foo\"=inactive\\s+\"bar\"=active\\s+\"baz\"=inactive\\s*"
}

test_gpioset_interactive_sleep() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	dut_run gpioset --interactive foo=1 bar=0 baz=0

	dut_write "sleep 500ms"
	dut_flush

	assert_fail dut_readable

	sleep 1

	# prompt, but not a full line...
	dut_readable
}

test_gpioset_toggle_continuous() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 7 pull-up

	dut_run gpioset --banner --toggle 100ms foo=1 bar=0 baz=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	gpiosim_wait_value sim0 1 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 7 1


	gpiosim_wait_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 0

	gpiosim_wait_value sim0 1 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 7 1
}

test_gpioset_toggle_terminated() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	gpiosim_set_pull sim0 4 pull-up

	# hold-period to allow test to sample before gpioset exits
	dut_run gpioset --banner --toggle 1s,0 -p 600ms foo=1 bar=0 baz=1

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 1

	sleep 1

	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 4 1
	gpiosim_check_value sim0 7 0

	dut_wait

	status_is 0

	# using --toggle 0 to exit
	# hold-period to allow test to sample before gpioset exits
	dut_run gpioset --banner -t0 -p 600ms foo=1 bar=0 baz=1

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 4 0
	gpiosim_check_value sim0 7 1

	dut_wait

	status_is 0
}

test_gpioset_with_invalid_toggle_period() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo line_name=4:bar \
				      line_name=7:baz

	run_tool gpioset --toggle 1ns foo=1 bar=0 baz=0

	output_regex_match ".*invalid period.*"
	status_is 1
}

test_gpioset_with_strict_named_line_check() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	run_tool gpioset --strict foobar=active

	output_regex_match ".*line 'foobar' is not unique"
	status_is 1
}

test_gpioset_with_lines_by_offset() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	gpiosim_set_pull sim0 1 pull-down
	gpiosim_set_pull sim0 6 pull-up

	dut_run gpioset --banner --chip ${GPIOSIM_CHIP_NAME[sim0]} 6=1 1=0

	gpiosim_check_value sim0 1 0
	gpiosim_check_value sim0 6 1
}

test_gpioset_with_lines_strictly_by_name() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	gpiosim_set_pull sim0 1 pull-down
	gpiosim_set_pull sim0 6 pull-up

	dut_run gpioset --banner --by-name --chip ${GPIOSIM_CHIP_NAME[sim0]} 6=1 1=0

	gpiosim_check_value sim0 1 1
	gpiosim_check_value sim0 6 0
}

test_gpioset_interactive_after_SIGINT() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	dut_run gpioset -i foo=1

	dut_kill -SIGINT
	dut_wait

	status_is 130
}

test_gpioset_interactive_after_SIGTERM() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	dut_run gpioset -i foo=1

	dut_kill -SIGTERM
	dut_wait

	status_is 143
}

test_gpioset_with_no_arguments() {
	run_tool gpioset

	status_is 1
	output_regex_match ".*at least one GPIO line value must be specified"
}

test_gpioset_with_chip_but_no_line_specified() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --chip ${GPIOSIM_CHIP_NAME[sim0]}

	output_regex_match ".*at least one GPIO line value must be specified"
	status_is 1
}

test_gpioset_with_offset_out_of_range() {
	gpiosim_chip sim0 num_lines=4

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioset --chip $sim0 0=1 1=1 2=1 3=1 4=1 5=1

	output_regex_match ".*offset 4 is out of range on chip '$sim0'"
	output_regex_match ".*offset 5 is out of range on chip '$sim0'"
	status_is 1
}

test_gpioset_with_invalid_hold_period() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioset --hold-period=bad --chip $sim0 0=1

	output_regex_match ".*invalid period.*"
	status_is 1
}

test_gpioset_with_invalid_value() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# by name
	run_tool gpioset --chip $sim0 0=c

	output_regex_match ".*invalid line value.*"
	status_is 1

	# by value
	run_tool gpioset --chip $sim0 0=3

	output_regex_match ".*invalid line value.*"
	status_is 1
}

test_gpioset_with_invalid_offset() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --chip ${GPIOSIM_CHIP_NAME[sim0]} 4000000000=0

	output_regex_match ".*cannot find line '4000000000'"
	status_is 1
}

test_gpioset_with_invalid_bias() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --bias=bad --chip ${GPIOSIM_CHIP_NAME[sim0]} 0=1 1=1

	output_regex_match ".*invalid bias.*"
	status_is 1
}

test_gpioset_with_invalid_drive() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpioset --drive=bad --chip ${GPIOSIM_CHIP_NAME[sim0]} 0=1 1=1

	output_regex_match ".*invalid drive.*"
	status_is 1
}

test_gpioset_with_interactive_and_toggle() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpioset --interactive --toggle 1s --chip $sim0 0=1

	output_regex_match ".*can't combine interactive with toggle"
	status_is 1
}

test_gpioset_with_nonexistent_line() {
	run_tool gpioset nonexistent-line=0

	output_regex_match ".*cannot find line 'nonexistent-line'"
	status_is 1
}

test_gpioset_with_same_line_twice() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# by offset
	run_tool gpioset --chip $sim0 0=1 0=1

	output_regex_match ".*lines '0' and '0' are the same line"
	status_is 1

	# by name
	run_tool gpioset --chip $sim0 foo=1 foo=1

	output_regex_match ".*lines 'foo' and 'foo' are the same line"
	status_is 1

	# by name and offset
	run_tool gpioset --chip $sim0 foo=1 1=1

	output_regex_match ".*lines 'foo' and '1' are the same line"
	status_is 1

	# by offset and name
	run_tool gpioset --chip $sim0 1=1 foo=1

	output_regex_match ".*lines '1' and 'foo' are the same line"
	status_is 1
}

#
# gpiomon test cases
#

test_gpiomon_by_name() {
	gpiosim_chip sim0 num_lines=8 line_name=4:foo

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --edges=rising foo
	dut_flush

	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+\"foo\""
	assert_fail dut_readable
}

test_gpiomon_by_offset() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --edges=rising --chip $sim0 4
	dut_regex_match "Monitoring line .*"

	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4"
	assert_fail dut_readable
}

test_gpiomon_by_symlink() {
	gpiosim_chip sim0 num_lines=8
	gpiosim_chip_symlink sim0 .

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --edges=rising --chip $GPIOSIM_CHIP_LINK 4
	dut_regex_match "Monitoring line .*"

	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0\\s+4"
	assert_fail dut_readable
}


test_gpiomon_by_chip_and_name() {
	gpiosim_chip sim0 num_lines=8 line_name=0:foo
	gpiosim_chip sim1 num_lines=8 line_name=2:foo

	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	gpiosim_set_pull sim1 0 pull-up

	dut_run gpiomon --banner --edges=rising --chip $sim1 foo
	dut_regex_match "Monitoring line .*"

	gpiosim_set_pull sim1 2 pull-down
	gpiosim_set_pull sim1 2 pull-up
	gpiosim_set_pull sim1 2 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim1 2 \"foo\""
	assert_fail dut_readable
}

test_gpiomon_first_matching_named_line() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	dut_run gpiomon --banner foobar
	dut_regex_match "Monitoring line .*"

	gpiosim_set_pull sim0 3 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+\"foobar\""
	assert_fail dut_readable
}

test_gpiomon_rising_edge() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --edges=rising --chip $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4"
	assert_fail dut_readable
}

test_gpiomon_falling_edge() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-down

	dut_run gpiomon --banner --edges=falling --chip $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 4"
	assert_fail dut_readable
}

test_gpiomon_both_edges() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner --edges=both --chip $sim0 4
	dut_regex_match "Monitoring line .*"

	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4"

	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 4"
}

test_gpiomon_with_pull_up() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-down

	dut_run gpiomon --banner --bias=pull-up --chip $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 4"

	assert_fail dut_readable
}

test_gpiomon_with_pull_down() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --bias=pull-down --chip $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up

	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4"

	assert_fail dut_readable
}

test_gpiomon_with_active_low() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --active-low --chip $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4"

	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 4"

	assert_fail dut_readable
}

test_gpiomon_with_consumer() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=3:baz line_name=4:xyz

	dut_run gpiomon --banner --consumer gpio-tools-tests foo baz

	run_tool gpioinfo

	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match \
"\\s+line\\s+1:\\s+\"foo\"\\s+input edges=both consumer=\"gpio-tools-tests\""
	output_regex_match \
"\\s+line\\s+3:\\s+\"baz\"\\s+input edges=both consumer=\"gpio-tools-tests\""
	status_is 0
}

test_gpiomon_with_quiet_mode() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner --edges=rising --quiet --chip $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	assert_fail dut_readable
}

test_gpiomon_with_unquoted() {
	gpiosim_chip sim0 num_lines=8 line_name=4:foo

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpiomon --banner --unquoted --edges=rising foo
	dut_flush

	gpiosim_set_pull sim0 4 pull-down
	gpiosim_set_pull sim0 4 pull-up
	gpiosim_set_pull sim0 4 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+foo"
	assert_fail dut_readable
}

test_gpiomon_with_num_events() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# redirect, as gpiomon exits after 4 events
	dut_run_redirect gpiomon --num-events=4 --chip $sim0 4

	gpiosim_set_pull sim0 4 pull-up
	sleep 0.01
	gpiosim_set_pull sim0 4 pull-down
	sleep 0.01
	gpiosim_set_pull sim0 4 pull-up
	sleep 0.01
	gpiosim_set_pull sim0 4 pull-down
	sleep 0.01

	dut_wait
	status_is 0
	dut_read_redirect

	regex_matches "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4" "${lines[0]}"
	regex_matches "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 4" "${lines[1]}"
	regex_matches "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 4" "${lines[2]}"
	regex_matches "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 4" "${lines[3]}"
	num_lines_is 4
}

test_gpiomon_with_debounce_period() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=3:baz line_name=4:xyz

	dut_run gpiomon --banner --debounce-period 123us foo baz

	run_tool gpioinfo

	output_regex_match "\\s+line\\s+0:\\s+unnamed\\s+input"
	output_regex_match \
"\\s+line\\s+1:\\s+\"foo\"\\s+input edges=both debounce-period=123us"
	output_regex_match \
"\\s+line\\s+3:\\s+\"baz\"\\s+input edges=both debounce-period=123us"
	status_is 0
}

test_gpiomon_with_idle_timeout() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# redirect, as gpiomon exits
	dut_run_redirect gpiomon --idle-timeout 10ms --chip $sim0 4

	dut_wait
	status_is 0
	dut_read_redirect
	num_lines_is 0
}

test_gpiomon_multiple_lines() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner --format=%o --chip $sim0 1 3 2 5 4
	dut_regex_match "Monitoring lines .*"

	gpiosim_set_pull sim0 2 pull-up
	dut_regex_match "2"
	gpiosim_set_pull sim0 3 pull-up
	dut_regex_match "3"
	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match "4"

	assert_fail dut_readable
}

test_gpiomon_multiple_lines_by_name_and_offset() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner --format=%o --chip $sim0 foo bar 3
	dut_regex_match "Monitoring lines .*"

	gpiosim_set_pull sim0 2 pull-up
	dut_regex_match "2"
	gpiosim_set_pull sim0 3 pull-up
	dut_regex_match "3"
	gpiosim_set_pull sim0 1 pull-up
	dut_regex_match "1"

	assert_fail dut_readable
}

test_gpiomon_multiple_lines_across_multiple_chips() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=4:xyz

	dut_run gpiomon --banner --format=%l foo bar baz
	dut_regex_match "Monitoring lines .*"

	gpiosim_set_pull sim0 2 pull-up
	dut_regex_match "bar"
	gpiosim_set_pull sim1 0 pull-up
	dut_regex_match "baz"
	gpiosim_set_pull sim0 1 pull-up
	dut_regex_match "foo"

	assert_fail dut_readable
}

test_gpiomon_exit_after_SIGINT() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner --chip $sim0 4
	dut_regex_match "Monitoring line .*"

	dut_kill -SIGINT
	dut_wait

	status_is 130
}

test_gpiomon_exit_after_SIGTERM() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner --chip $sim0 4
	dut_regex_match "Monitoring line .*"

	dut_kill -SIGTERM
	dut_wait

	status_is 143
}

test_gpiomon_with_nonexistent_line() {
	run_tool gpiomon nonexistent-line

	status_is 1
	output_regex_match ".*cannot find line 'nonexistent-line'"
}

test_gpiomon_with_same_line_twice() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# by offset
	run_tool gpiomon --chip $sim0 0 0

	output_regex_match ".*lines '0' and '0' are the same line"
	status_is 1

	# by name
	run_tool gpiomon foo foo

	output_regex_match ".*lines 'foo' and 'foo' are the same line"
	status_is 1

	# by name and offset
	run_tool gpiomon --chip $sim0 1 foo

	output_regex_match ".*lines '1' and 'foo' are the same line"
	status_is 1
}

test_gpiomon_with_strict_named_line_check() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	run_tool gpiomon --strict foobar

	output_regex_match ".*line 'foobar' is not unique"
	status_is 1
}
test_gpiomon_with_lines_by_offset() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 1 pull-up

	dut_run gpiomon --banner --chip $sim0 6 1
	dut_flush

	gpiosim_set_pull sim0 1 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 1"

	gpiosim_set_pull sim0 1 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 1"

	gpiosim_set_pull sim0 6 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 6"

	gpiosim_set_pull sim0 6 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 6"

	assert_fail dut_readable
}

test_gpiomon_with_lines_strictly_by_name() {
	# not suggesting this setup makes sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:42 line_name=6:13

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 1 pull-up

	dut_run gpiomon --banner --by-name --chip $sim0 42 13
	dut_flush

	gpiosim_set_pull sim0 1 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 1"

	gpiosim_set_pull sim0 1 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 1"

	gpiosim_set_pull sim0 6 pull-up
	dut_regex_match "[0-9]+\.[0-9]+\\s+rising\\s+$sim0 6"

	gpiosim_set_pull sim0 6 pull-down
	dut_regex_match "[0-9]+\.[0-9]+\\s+falling\\s+$sim0 6"

	assert_fail dut_readable
}

test_gpiomon_with_no_arguments() {
	run_tool gpiomon

	output_regex_match ".*at least one GPIO line must be specified"
	status_is 1
}

test_gpiomon_with_no_line_specified() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpiomon --chip ${GPIOSIM_CHIP_NAME[sim0]}

	output_regex_match ".*at least one GPIO line must be specified"
	status_is 1
}

test_gpiomon_with_offset_out_of_range() {
	gpiosim_chip sim0 num_lines=4

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpiomon --chip $sim0 5

	output_regex_match ".*offset 5 is out of range on chip '$sim0'"
	status_is 1
}

test_gpiomon_with_invalid_bias() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpiomon --bias=bad -c $sim0 0 1

	output_regex_match ".*invalid bias.*"
	status_is 1
}

test_gpiomon_with_invalid_debounce_period() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpiomon --debounce-period bad -c $sim0 0 1

	output_regex_match ".*invalid period: bad"
	status_is 1
}

test_gpiomon_with_invalid_idle_timeout() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpiomon --idle-timeout bad -c $sim0 0 1

	output_regex_match ".*invalid period: bad"
	status_is 1
}

test_gpiomon_with_custom_format_event_type_offset() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%e %o" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "1 4"
}

test_gpiomon_with_custom_format_event_type_offset_joined() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%e%o" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "14"
}

test_gpiomon_with_custom_format_edge_chip_and_line() {
	gpiosim_chip sim0 num_lines=8 line_name=4:baz

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%e %o %E %c %l" -c $sim0 baz
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match "1 4 rising $sim0 baz"
}

test_gpiomon_with_custom_format_seconds_timestamp() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%e %o %S" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match "1 4 [0-9]+\\.[0-9]+"
}

test_gpiomon_with_custom_format_UTC_timestamp() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%U %e %o " --event-clock=realtime \
		-c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match \
"[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]\\.[0-9]+Z 1 4"
}

test_gpiomon_with_custom_format_localtime_timestamp() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%L %e %o" --event-clock=realtime \
		-c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_regex_match \
"[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]\\.[0-9]+ 1 4"
}

test_gpiomon_with_custom_format_double_percent_sign() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=start%%end" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "start%end"
}

test_gpiomon_with_custom_format_double_percent_sign_event_type_specifier() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%%e" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "%e"
}

test_gpiomon_with_custom_format_single_percent_sign() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "%"
}

test_gpiomon_with_custom_format_single_percent_sign_between_other_characters() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=foo % bar" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "foo % bar"
}

test_gpiomon_with_custom_format_unknown_specifier() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpiomon --banner "--format=%x" -c $sim0 4
	dut_flush

	gpiosim_set_pull sim0 4 pull-up
	dut_read
	output_is "%x"
}

#
# gpionotify test cases
#

test_gpionotify_by_name() {
	gpiosim_chip sim0 num_lines=8 line_name=4:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner foo
	dut_regex_match "Watching line .*"

	request_release_line $sim0 4

	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+\"foo\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+\"foo\""
	# tools currently have no way to generate a reconfig event
}

test_gpionotify_by_offset() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --chip $sim0 4
	dut_regex_match "Watching line .*"

	request_release_line $sim0 4
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 4"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 4"

	assert_fail dut_readable
}

test_gpionotify_by_symlink() {
	gpiosim_chip sim0 num_lines=8
	gpiosim_chip_symlink sim0 .

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --chip $GPIOSIM_CHIP_LINK 4
	dut_regex_match "Watching line .*"

	request_release_line $sim0 4
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0\\s+4"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0\\s+4"

	assert_fail dut_readable
}

test_gpionotify_by_chip_and_name() {
	gpiosim_chip sim0 num_lines=8 line_name=4:foo
	gpiosim_chip sim1 num_lines=8 line_name=2:foo

	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	dut_run gpionotify --banner --chip $sim1 foo
	dut_regex_match "Watching line .*"

	request_release_line $sim1 2
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim1 2 \"foo\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim1 2 \"foo\""

	assert_fail dut_readable
}

test_gpionotify_first_matching_named_line() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	dut_run gpionotify --banner foobar
	dut_regex_match "Watching line .*"

	request_release_line ${GPIOSIM_CHIP_NAME[sim0]} 3
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+\"foobar\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+\"foobar\""

	assert_fail dut_readable
}

test_gpionotify_with_requested() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-up

	dut_run gpionotify --banner --event=requested --chip $sim0 4
	dut_flush

	request_release_line ${GPIOSIM_CHIP_NAME[sim0]} 4
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 4"
	assert_fail dut_readable
}

test_gpionotify_with_released() {
	gpiosim_chip sim0 num_lines=8
	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	gpiosim_set_pull sim0 4 pull-down

	dut_run gpionotify --banner --event=released --chip $sim0 4
	dut_flush

	request_release_line ${GPIOSIM_CHIP_NAME[sim0]} 4
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 4"
	assert_fail dut_readable
}

test_gpionotify_with_quiet_mode() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --quiet --chip $sim0 4
	dut_flush

	request_release_line ${GPIOSIM_CHIP_NAME[sim0]} 4
	assert_fail dut_readable
}

test_gpionotify_with_unquoted() {
	gpiosim_chip sim0 num_lines=8 line_name=4:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --unquoted foo
	dut_regex_match "Watching line .*"

	request_release_line $sim0 4

	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+foo"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+foo"
}

test_gpionotify_with_num_events() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# redirect, as gpionotify exits after 4 events
	dut_run_redirect gpionotify --num-events=4 --chip $sim0 3 4


	request_release_line ${GPIOSIM_CHIP_NAME[sim0]} 4
	request_release_line ${GPIOSIM_CHIP_NAME[sim0]} 3

	dut_wait
	status_is 0
	dut_read_redirect

	regex_matches "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 4" "${lines[0]}"
	regex_matches "[0-9]+\.[0-9]+\\s+released\\s+$sim0 4" "${lines[1]}"
	regex_matches "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 3" "${lines[2]}"
	regex_matches "[0-9]+\.[0-9]+\\s+released\\s+$sim0 3" "${lines[3]}"
	num_lines_is 4
}

test_gpionotify_with_idle_timeout() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# redirect, as gpionotify exits
	dut_run_redirect gpionotify --idle-timeout 10ms --chip $sim0 3 4

	dut_wait
	status_is 0
	dut_read_redirect

	num_lines_is 0
}

test_gpionotify_multiple_lines() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --chip $sim0 1 2 3 4 5
	dut_regex_match "Watching lines .*"

	request_release_line $sim0 2
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 2"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 2"

	request_release_line $sim0 3
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 3"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 3"

	request_release_line $sim0 4
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 4"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 4"

	assert_fail dut_readable
}

test_gpionotify_multiple_lines_by_name_and_offset() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --chip $sim0 bar foo 3
	dut_regex_match "Watching lines .*"

	request_release_line $sim0 2
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 2\\s+\"bar\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 2\\s+\"bar\""

	request_release_line $sim0 1
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 1\\s+\"foo\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 1\\s+\"foo\""

	request_release_line $sim0 3
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 3"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 3"

	assert_fail dut_readable
}

test_gpionotify_multiple_lines_across_multiple_chips() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=4:xyz

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}
	local sim1=${GPIOSIM_CHIP_NAME[sim1]}

	dut_run gpionotify --banner baz bar foo xyz
	dut_regex_match "Watching lines .*"

	request_release_line $sim0 2
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+\"bar\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+\"bar\""

	request_release_line $sim0 1
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+\"foo\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+\"foo\""

	request_release_line $sim1 4
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+\"xyz\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+\"xyz\""

	request_release_line $sim1 0
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+\"baz\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+\"baz\""

	assert_fail dut_readable
}

test_gpionotify_exit_after_SIGINT() {
	gpiosim_chip sim0 num_lines=8

	dut_run gpionotify --banner --chip ${GPIOSIM_CHIP_NAME[sim0]} 4
	dut_regex_match "Watching line .*"

	dut_kill -SIGINT
	dut_wait

	status_is 130
}

test_gpionotify_exit_after_SIGTERM() {
	gpiosim_chip sim0 num_lines=8

	dut_run gpionotify --banner --chip ${GPIOSIM_CHIP_NAME[sim0]} 4
	dut_regex_match "Watching line .*"

	dut_kill -SIGTERM
	dut_wait

	status_is 143
}

test_gpionotify_with_nonexistent_line() {
	run_tool gpionotify nonexistent-line

	status_is 1
	output_regex_match ".*cannot find line 'nonexistent-line'"
}

test_gpionotify_with_same_line_twice() {
	gpiosim_chip sim0 num_lines=8 line_name=1:foo

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	# by offset
	run_tool gpionotify --chip $sim0 0 0

	output_regex_match ".*lines '0' and '0' are the same line"
	num_lines_is 1
	status_is 1

	# by name
	run_tool gpionotify foo foo

	output_regex_match ".*lines 'foo' and 'foo' are the same line"
	num_lines_is 1
	status_is 1

	# by name and offset
	run_tool gpionotify --chip $sim0 1 foo

	output_regex_match ".*lines '1' and 'foo' are the same line"
	num_lines_is 1
	status_is 1
}

test_gpionotify_with_strict_named_line_check() {
	gpiosim_chip sim0 num_lines=4 line_name=1:foo line_name=2:bar \
				      line_name=3:foobar
	gpiosim_chip sim1 num_lines=8 line_name=0:baz line_name=2:foobar \
				      line_name=4:xyz line_name=7:foobar
	gpiosim_chip sim2 num_lines=16

	run_tool gpionotify --strict foobar

	output_regex_match ".*line 'foobar' is not unique"
	status_is 1
}

test_gpionotify_with_lines_by_offset() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --chip $sim0 1
	dut_flush

	request_release_line $sim0 1
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 1"
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 1"

	request_release_line $sim0 6

	assert_fail dut_readable
}

test_gpionotify_with_lines_strictly_by_name() {
	# not suggesting this setup makes any sense
	# - just test that we can deal with it
	gpiosim_chip sim0 num_lines=8 line_name=1:6 line_name=6:1

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --by-name --chip $sim0 1
	dut_flush

	request_release_line $sim0 6
	dut_regex_match "[0-9]+\.[0-9]+\\s+requested\\s+$sim0 6 \"1\""
	dut_regex_match "[0-9]+\.[0-9]+\\s+released\\s+$sim0 6 \"1\""

	request_release_line $sim0 1
	assert_fail dut_readable
}

test_gpionotify_with_no_arguments() {
	run_tool gpionotify

	output_regex_match ".*at least one GPIO line must be specified"
	status_is 1
}

test_gpionotify_with_no_line_specified() {
	gpiosim_chip sim0 num_lines=8

	run_tool gpionotify --chip ${GPIOSIM_CHIP_NAME[sim0]}

	output_regex_match ".*at least one GPIO line must be specified"
	status_is 1
}

test_gpionotify_with_offset_out_of_range() {
	gpiosim_chip sim0 num_lines=4

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpionotify --chip $sim0 5

	output_regex_match ".*offset 5 is out of range on chip '$sim0'"
	status_is 1
}

test_gpionotify_with_invalid_idle_timeout() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	run_tool gpionotify --idle-timeout bad -c $sim0 0 1

	output_regex_match ".*invalid period: bad"
	status_is 1
}

test_gpionotify_with_custom_format_event_type_offset() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=%e %o" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "1 4"
}

test_gpionotify_with_custom_format_event_type_offset_joined() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=%e%o" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "14"
}

test_gpionotify_with_custom_format_event_chip_and_line() {
	gpiosim_chip sim0 num_lines=8 line_name=4:baz

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=released \
		"--format=%e %o %E %c %l" -c $sim0 baz
	dut_flush

	request_release_line $sim0 4
	dut_regex_match "2 4 released $sim0 baz"
}

test_gpionotify_with_custom_format_seconds_timestamp() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=%e %o %S" \
		-c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_regex_match "1 4 [0-9]+\\.[0-9]+"
}

test_gpionotify_with_custom_format_UTC_timestamp() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=released \
		"--format=%U %e %o" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_regex_match \
"[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]\\.[0-9]+Z 2 4"
}

test_gpionotify_with_custom_format_localtime_timestamp() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=released \
		"--format=%L %e %o" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_regex_match \
"[0-9][0-9][0-9][0-9]-[0-1][0-9]-[0-3][0-9]T[0-2][0-9]:[0-5][0-9]:[0-5][0-9]\\.[0-9]+ 2 4"
}

test_gpionotify_with_custom_format_double_percent_sign() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=start%%end" \
		-c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "start%end"
}

test_gpionotify_with_custom_format_double_percent_sign_event_type_specifier() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=%%e" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "%e"
}

test_gpionotify_with_custom_format_single_percent_sign() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=%" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "%"
}

test_gpionotify_with_custom_format_single_percent_sign_between_other_characters() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=foo % bar" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "foo % bar"
}

test_gpionotify_with_custom_format_unknown_specifier() {
	gpiosim_chip sim0 num_lines=8

	local sim0=${GPIOSIM_CHIP_NAME[sim0]}

	dut_run gpionotify --banner --event=requested "--format=%x" -c $sim0 4
	dut_flush

	request_release_line $sim0 4
	dut_read
	output_is "%x"
}

die() {
	echo "$@" 1>&2
	exit 1
}

# Must be done after we sources shunit2 as we need SHUNIT_VERSION to be set.
oneTimeSetUp() {
	test "$SHUNIT_VERSION" = "$MIN_SHUNIT_VERSION" && return 0
	local FIRST=$(printf "$SHUNIT_VERSION\n$MIN_SHUNIT_VERSION\n" | sort -Vr | head -1)
	test "$FIRST" = "$MIN_SHUNIT_VERSION" && \
		die "minimum shunit version required is $MIN_SHUNIT_VERSION (current version is $SHUNIT_VERSION"
}

check_kernel() {
	local REQUIRED=$1
	local CURRENT=$(uname -r)

	SORTED=$(printf "$REQUIRED\n$CURRENT" | sort -V | head -n 1)

	if [ "$SORTED" != "$REQUIRED" ]
	then
		die "linux kernel version must be at least: v$REQUIRED - got: v$CURRENT"
	fi
}

check_prog() {
	local PROG=$1

	which "$PROG" > /dev/null
	if [ "$?" -ne "0" ]
	then
		die "$PROG not found - needed to run the tests"
	fi
}

# Check all required non-coreutils tools
check_prog shunit2
check_prog modprobe
check_prog timeout
check_prog grep

# Check if we're running a kernel at the required version or later
check_kernel $MIN_KERNEL_VERSION

modprobe gpio-sim || die "unable to load the gpio-sim module"
mountpoint /sys/kernel/config/ > /dev/null 2> /dev/null || \
	die "configfs not mounted at /sys/kernel/config/"

. shunit2
