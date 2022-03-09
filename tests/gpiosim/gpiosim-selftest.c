// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <stdio.h>
#include <stdlib.h>

#include "gpiosim.h"

#define UNUSED __attribute__((unused))

static const char *const line_names[] = {
	"foo",
	"bar",
	"foobar",
	NULL,
	"barfoo",
};

int main(int argc UNUSED, char **argv UNUSED)
{
	struct gpiosim_bank *bank0, *bank1;
	struct gpiosim_dev *dev;
	struct gpiosim_ctx *ctx;
	int ret, i;

	printf("Creating gpiosim context\n");

	ctx = gpiosim_ctx_new();
	if (!ctx) {
		perror("unable to create the gpios-sim context");
		return EXIT_FAILURE;
	}

	printf("Creating a chip with random name\n");

	dev = gpiosim_dev_new(ctx, NULL);
	if (!dev) {
		perror("Unable to create a chip with random name");
		return EXIT_FAILURE;
	}

	printf("Creating a bank with a random name\n");

	bank0 = gpiosim_bank_new(dev, NULL);
	if (!bank0) {
		perror("Unable to create a bank with random name");
		return EXIT_FAILURE;
	}

	printf("Creating a bank with a specific name\n");

	bank1 = gpiosim_bank_new(dev, "foobar");
	if (!bank1) {
		perror("Unable to create a bank with a specific name");
		return EXIT_FAILURE;
	}

	printf("Setting the label of bank #2 to foobar\n");

	ret = gpiosim_bank_set_label(bank1, "foobar");
	if (ret) {
		perror("Unable to set the label of bank #2");
		return EXIT_FAILURE;
	}

	printf("Setting the number of lines in bank #1 to 16\n");

	ret = gpiosim_bank_set_num_lines(bank0, 16);
	if (ret) {
		perror("Unable to set the number of lines");
		return EXIT_FAILURE;
	}

	printf("Setting the number of lines in bank #2 to 8\n");

	ret = gpiosim_bank_set_num_lines(bank1, 8);
	if (ret) {
		perror("Unable to set the number of lines");
		return EXIT_FAILURE;
	}

	printf("Setting names for some lines in bank #1\n");

	for (i = 0; i < 5; i++) {
		ret = gpiosim_bank_set_line_name(bank0, i, line_names[i]);
		if (ret) {
			perror("Unable to set line names");
			return EXIT_FAILURE;
		}
	}

	printf("Hog a line on bank #2\n");

	ret = gpiosim_bank_hog_line(bank1, 3, "xyz",
				    GPIOSIM_HOG_DIR_OUTPUT_HIGH);
	if (ret) {
		perror("Unable to hog a line");
		return EXIT_FAILURE;
	}

	printf("Enabling the GPIO device\n");

	ret = gpiosim_dev_enable(dev);
	if (ret) {
		perror("Unable to enable the device");
		return EXIT_FAILURE;
	}

	printf("Setting the pull of a single line to pull-up\n");

	ret = gpiosim_bank_set_pull(bank0, 6, GPIOSIM_PULL_UP);
	if (ret) {
		perror("Unable to set the pull");
		return EXIT_FAILURE;
	}

	printf("Reading the pull back\n");

	ret = gpiosim_bank_get_pull(bank0, 6);
	if (ret < 0) {
		perror("Unable to read the pull");
		return EXIT_FAILURE;
	}

	if (ret != GPIOSIM_PULL_UP) {
		fprintf(stderr, "Invalid pull value read\n");
		return EXIT_FAILURE;
	}

	printf("Reading the value\n");

	ret = gpiosim_bank_get_value(bank0, 6);
	if (ret < 0) {
		perror("Unable to read the value");
		return EXIT_FAILURE;
	}

	if (ret != GPIOSIM_VALUE_ACTIVE) {
		fprintf(stderr, "Invalid value read\n");
		return EXIT_FAILURE;
	}

	printf("Disabling the GPIO device\n");

	ret = gpiosim_dev_disable(dev);
	if (ret) {
		perror("Error while disabling the device");
		return EXIT_FAILURE;
	}

	gpiosim_bank_unref(bank1);
	gpiosim_bank_unref(bank0);
	gpiosim_dev_unref(dev);
	gpiosim_ctx_unref(ctx);

	return EXIT_SUCCESS;
}
