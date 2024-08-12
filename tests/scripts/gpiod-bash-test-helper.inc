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

# mappings from local name to system chip name, path, dev name
declare -A GPIOSIM_CHIP_NAME
declare -A GPIOSIM_CHIP_PATH
declare -A GPIOSIM_DEV_NAME
GPIOSIM_CONFIGFS="/sys/kernel/config/gpio-sim"
GPIOSIM_SYSFS="/sys/devices/platform/"
GPIOSIM_APP_NAME="gpio-tools-test"

MIN_KERNEL_VERSION="5.17.4"
MIN_SHUNIT_VERSION="2.1.8"

# Run the command in $@ and fail the test if the command succeeds.
assert_fail() {
	"$@" || return 0
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
	local NUM_LINES
	NUM_LINES=$(echo "$output" | wc -l)
	assertEquals " number of lines:" "$1" "$NUM_LINES"
}

status_is() {
	assertEquals " status:" "$1" "$status"
}

# Same as above but match against the regex pattern in $1.
output_regex_match() {
	[[ "$output" =~ $1 ]]
	assertEquals " '$output' did not match '$1'" "0" "$?"
}

gpiosim_chip() {
	local VAR=$1
	local NAME=${GPIOSIM_APP_NAME}-$$-${VAR}
	local DEVPATH=$GPIOSIM_CONFIGFS/$NAME
	local BANKPATH=$DEVPATH/bank0

	mkdir -p "$BANKPATH"

	for ARG in "$@"
	do
		local KEY VAL
		KEY=$(echo "$ARG" | cut -d"=" -f1)
		VAL=$(echo "$ARG" | cut -d"=" -f2)

		if [ "$KEY" = "num_lines" ]
		then
			echo "$VAL" > "$BANKPATH/num_lines"
		elif [ "$KEY" = "line_name" ]
		then
			local OFFSET LINENAME
			OFFSET=$(echo "$VAL" | cut -d":" -f1)
			LINENAME=$(echo "$VAL" | cut -d":" -f2)
			local LINEPATH=$BANKPATH/line$OFFSET

			mkdir -p "$LINEPATH"
			echo "$LINENAME" > "$LINEPATH/name"
		fi
	done

	echo 1 > "$DEVPATH/live"

	local CHIP_NAME
	CHIP_NAME=$(<"$BANKPATH/chip_name")
	GPIOSIM_CHIP_NAME[$1]=$CHIP_NAME
	GPIOSIM_CHIP_PATH[$1]="/dev/$CHIP_NAME"
	GPIOSIM_DEV_NAME[$1]=$(<"$DEVPATH/dev_name")
}

gpiosim_chip_number() {
	local NAME=${GPIOSIM_CHIP_NAME[$1]}
	echo "${NAME#gpiochip}"
}

gpiosim_chip_symlink() {
	GPIOSIM_CHIP_LINK="$2/${GPIOSIM_APP_NAME}-$$-lnk"
	ln -s "${GPIOSIM_CHIP_PATH[$1]}" "$GPIOSIM_CHIP_LINK"
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

	echo "$PULL" > "$GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/pull"
}

gpiosim_check_value() {
	local OFFSET=$2
	local EXPECTED=$3
	local DEVNAME=${GPIOSIM_DEV_NAME[$1]}
	local CHIPNAME=${GPIOSIM_CHIP_NAME[$1]}

	VAL=$(<"$GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/value")
	[ "$VAL" = "$EXPECTED" ]
}

gpiosim_wait_value() {
	local OFFSET=$2
	local EXPECTED=$3
	local DEVNAME=${GPIOSIM_DEV_NAME[$1]}
	local CHIPNAME=${GPIOSIM_CHIP_NAME[$1]}
	local PORT=$GPIOSIM_SYSFS/$DEVNAME/$CHIPNAME/sim_gpio$OFFSET/value

	for _i in {1..30}; do
		[ "$(<"$PORT")" = "$EXPECTED" ] && return
		sleep 0.01
	done
	return 1
}

gpiosim_cleanup() {
	for CHIP in "${!GPIOSIM_CHIP_NAME[@]}"
	do
		local NAME=${GPIOSIM_APP_NAME}-$$-$CHIP

		local DEVPATH=$GPIOSIM_CONFIGFS/$NAME

		echo 0 > "$DEVPATH/live"
		find "$DEVPATH" -type d -name hog -exec rmdir '{}' '+'
		find "$DEVPATH" -type d -name "line*" -exec rmdir '{}' '+'
		find "$DEVPATH" -type d -name "bank*" -exec rmdir '{}' '+'
		rmdir "$DEVPATH"
	done

	gpiosim_chip_symlink_cleanup

	GPIOSIM_CHIP_NAME=()
	GPIOSIM_CHIP_PATH=()
	GPIOSIM_DEV_NAME=()
}

run_prog() {
	# Executables to test are expected to be in the same directory as the
	# testing script.
	cmd=$1
	shift
	output=$(timeout 10s "$SOURCE_DIR/$cmd" "$@" 2>&1)
	status=$?
}

dut_run() {
	cmd=$1
	shift
	coproc timeout 10s "$SOURCE_DIR/$cmd" "$@" 2>&1
	DUT_PID=$COPROC_PID
	read -r -t1 -n1 -u "${COPROC[0]}" DUT_FIRST_CHAR
}

dut_run_redirect() {
	cmd=$1
	shift
	coproc timeout 10s "$SOURCE_DIR/$cmd" "$@" > "$SHUNIT_TMPDIR/$DUT_OUTPUT" 2>&1
	DUT_PID=$COPROC_PID
	# give the process time to spin up
	# FIXME - find a better solution
	sleep 0.2
}

dut_read_redirect() {
	output=$(<"$SHUNIT_TMPDIR/$DUT_OUTPUT")
	local ORIG_IFS="$IFS"
	IFS=$'\n' mapfile -t lines <<< "$output"
	IFS="$ORIG_IFS"
}

dut_read() {
	local LINE
	lines=()
	while read -r -t 0.2 -u "${COPROC[0]}" LINE
	do
		if [ -n "$DUT_FIRST_CHAR" ]
		then
			LINE=${DUT_FIRST_CHAR}${LINE}
			unset DUT_FIRST_CHAR
		fi
		lines+=("$LINE")
	done
	output="${lines[*]}"
}

dut_readable() {
	read -t 0 -u "${COPROC[0]}" LINE
}

dut_flush() {
	local _JUNK
	lines=()
	output=
	unset DUT_FIRST_CHAR
	while read -t 0 -u "${COPROC[0]}" _JUNK
	do
		read -r -t 0.1 -u "${COPROC[0]}" _JUNK || true
	done
}

# check the next line of output matches the regex
dut_regex_match() {
	PATTERN=$1

	read -r -t 0.2 -u "${COPROC[0]}" LINE || (echo Timeout && false)
	if [ -n "$DUT_FIRST_CHAR" ]
	then
		LINE=${DUT_FIRST_CHAR}${LINE}
		unset DUT_FIRST_CHAR
	fi
	[[ $LINE =~ $PATTERN ]]
	assertEquals "'$LINE' did not match '$PATTERN'" "0" "$?"
}

dut_write() {
	echo "$@" >&"${COPROC[1]}"
}

dut_kill() {
	kill "$@" "$DUT_PID"
}

dut_wait() {
	wait "$DUT_PID"
	export status=$?
	unset DUT_PID
}

dut_cleanup() {
	if [ -n "$DUT_PID" ]
	then
		kill -SIGTERM "$DUT_PID" 2> /dev/null
		wait "$DUT_PID" || false
	fi
	rm -f "$SHUNIT_TMPDIR/$DUT_OUTPUT"
}

tearDown() {
	dut_cleanup
	gpiosim_cleanup
}

request_release_line() {
	"$SOURCE_DIR/gpioget" -c "$@" >/dev/null
}

die() {
	echo "$@" 1>&2
	exit 1
}

# Must be done after we sources shunit2 as we need SHUNIT_VERSION to be set.
oneTimeSetUp() {
	test "$SHUNIT_VERSION" = "$MIN_SHUNIT_VERSION" && return 0
	local FIRST
	FIRST=$(printf "%s\n%s\n" "$SHUNIT_VERSION" "$MIN_SHUNIT_VERSION" | sort -Vr | head -1)
	test "$FIRST" = "$MIN_SHUNIT_VERSION" && \
		die "minimum shunit version required is $MIN_SHUNIT_VERSION (current version is $SHUNIT_VERSION"
}

check_kernel() {
	local REQUIRED=$1
	local CURRENT
	CURRENT=$(uname -r)

	SORTED=$(printf "%s\n%s" "$REQUIRED" "$CURRENT" | sort -V | head -n 1)

	if [ "$SORTED" != "$REQUIRED" ]
	then
		die "linux kernel version must be at least: v$REQUIRED - got: v$CURRENT"
	fi
}

check_prog() {
	local PROG=$1

	if ! which "$PROG" > /dev/null
	then
		die "$PROG not found - needed to run the tests"
	fi
}

# Check all required non-coreutils tools
check_prog shunit2
check_prog modprobe
check_prog timeout

# Check if we're running a kernel at the required version or later
check_kernel $MIN_KERNEL_VERSION

modprobe gpio-sim || die "unable to load the gpio-sim module"
mountpoint /sys/kernel/config/ > /dev/null 2> /dev/null || \
	die "configfs not mounted at /sys/kernel/config/"
