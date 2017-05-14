/*
 * Misc test cases for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-test.h"

#include <errno.h>

static void version_string(void)
{
	/* Check that gpiod_version_string() returns an actual string. */
	TEST_ASSERT_NOT_NULL(gpiod_version_string());
	TEST_ASSERT(strlen(gpiod_version_string()) > 0);
}
TEST_DEFINE(version_string,
	    "gpiod_version_string()",
	    TEST_LINES_UNNAMED, { 1 });

static void error_handling(void)
{
	struct gpiod_chip *chip;
	int err;

	chip = gpiod_chip_open("/dev/nonexistent_gpiochip");
	TEST_ASSERT_NULL(chip);

	err = gpiod_errno();
	TEST_ASSERT_EQ(err, ENOENT);

	TEST_ASSERT_NOT_NULL(gpiod_strerror(err));
	TEST_ASSERT(strlen(gpiod_strerror(err)) > 0);
	TEST_ASSERT_STR_EQ(gpiod_strerror(err), gpiod_last_strerror());
}
TEST_DEFINE(error_handling,
	    "error handling",
	    TEST_LINES_UNNAMED, { 1 });
