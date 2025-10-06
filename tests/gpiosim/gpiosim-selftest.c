// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <stdio.h>
#include <stdlib.h>

#include "gpiosim.h"

#define UNUSED		__attribute__((unused))
#define NORETURN	__attribute__((noreturn))

static const char *const line_names[] = {
	"foo",
	"bar",
	"foobar",
	NULL,
	"barfoo",
};

static NORETURN void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

#define expect_or_die(_cond, _msg) \
	do { \
		if (!(_cond)) \
			die(_msg); \
	} while (0)

int main(int argc UNUSED, char **argv UNUSED)
{
	struct gpiosim_bank *bank0, *bank1;
	struct gpiosim_dev *dev;
	struct gpiosim_ctx *ctx;
	enum gpiosim_pull pull;
	enum gpiosim_value val;
	int ret, i;

	printf("Creating gpiosim context\n");

	ctx = gpiosim_ctx_new();
	expect_or_die(ctx, "unable to create the gpios-sim context");

	printf("Creating a chip\n");

	dev = gpiosim_dev_new(ctx);
	expect_or_die(dev, "Unable to create a chip");

	printf("Creating a bank\n");

	bank0 = gpiosim_bank_new(dev);
	expect_or_die(bank0, "Unable to create a bank");

	printf("Creating a second bank\n");

	bank1 = gpiosim_bank_new(dev);
	expect_or_die(bank1, "Unable to create a bank");

	printf("Setting the label of bank #2 to foobar\n");

	ret = gpiosim_bank_set_label(bank1, "foobar");
	expect_or_die(ret == 0, "Unable to set the label of bank #2");

	printf("Setting the number of lines in bank #1 to 16\n");

	ret = gpiosim_bank_set_num_lines(bank0, 16);
	expect_or_die(ret == 0, "Unable to set the number of lines");

	printf("Setting the number of lines in bank #2 to 8\n");

	ret = gpiosim_bank_set_num_lines(bank1, 8);
	expect_or_die(ret == 0, "Unable to set the number of lines");

	printf("Setting names for some lines in bank #1\n");

	for (i = 0; i < 5; i++) {
		ret = gpiosim_bank_set_line_name(bank0, i, line_names[i]);
		expect_or_die(ret == 0, "Unable to set line names");
	}

	printf("Hog a line on bank #2\n");

	ret = gpiosim_bank_hog_line(bank1, 3, "xyz",
				    GPIOSIM_DIRECTION_OUTPUT_HIGH);
	expect_or_die(ret == 0, "Unable to hog a line");

	printf("Enabling the GPIO device\n");

	ret = gpiosim_dev_enable(dev);
	expect_or_die(ret == 0, "Unable to enable the device");

	expect_or_die(gpiosim_dev_is_live(dev), "Failed to enable the device");

	printf("Setting the pull of a single line to pull-up\n");

	ret = gpiosim_bank_set_pull(bank0, 6, GPIOSIM_PULL_UP);
	expect_or_die(ret == 0, "Unable to set the pull");

	printf("Reading the pull back\n");

	ret = gpiosim_bank_get_pull(bank0, 6);
	expect_or_die(ret >= 0, "Unable to read the pull");

	if (ret != GPIOSIM_PULL_UP) {
		fprintf(stderr, "Invalid pull value read\n");
		return EXIT_FAILURE;
	}

	printf("Reading the value\n");

	val = gpiosim_bank_get_value(bank0, 6);
	expect_or_die(val != GPIOSIM_VALUE_ERROR, "Unable to read the value");

	if (val != GPIOSIM_VALUE_ACTIVE) {
		fprintf(stderr, "Invalid value read\n");
		return EXIT_FAILURE;
	}

	printf("Disabling the GPIO device\n");

	expect_or_die(gpiosim_dev_is_live(dev), "Failed to disable the device");

	ret = gpiosim_dev_disable(dev);
	expect_or_die(ret == 0, "Error while disabling the device");

	printf("Clearing the GPIO hog on bank #2\n");

	ret = gpiosim_bank_clear_hog(bank1, 3);
	expect_or_die(ret == 0, "Error while clearing the hog");

	printf("Mark one line as invalid\n");

	ret = gpiosim_bank_set_line_valid(bank0, 1, false);
	expect_or_die(ret == 0, "Unable to mark line as invalid");

	printf("Re-enabling the GPIO device\n");

	ret = gpiosim_dev_enable(dev);
	expect_or_die(ret == 0, "Unable to re-enable the device");

	printf("Checking the pull has been reset\n");

	pull = gpiosim_bank_get_pull(bank0, 6);
	expect_or_die(pull != GPIOSIM_PULL_ERROR, "Unable to read the pull");

	if (pull != GPIOSIM_PULL_DOWN) {
		fprintf(stderr, "Invalid pull value read\n");
		return EXIT_FAILURE;
	}

	printf("Re-disabling the device\n");

	ret = gpiosim_dev_disable(dev);
	expect_or_die(ret == 0, "Error while re-disabling the device");

	gpiosim_bank_unref(bank1);
	gpiosim_bank_unref(bank0);
	gpiosim_dev_unref(dev);
	gpiosim_ctx_unref(ctx);

	printf("ALL TESTS OK\n");

	return EXIT_SUCCESS;
}
