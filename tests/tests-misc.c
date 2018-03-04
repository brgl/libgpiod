/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Misc test cases. */

#include "gpiod-test.h"

#include <errno.h>

static void version_string(void)
{
	/* Check that gpiod_version_string() returns an actual string. */
	TEST_ASSERT_NOT_NULL(gpiod_version_string());
	TEST_ASSERT(strlen(gpiod_version_string()) > 0);
	TEST_ASSERT_REGEX_MATCH(gpiod_version_string(),
				"^[0-9]+\\.[0-9]+[0-9a-zA-Z\\.]*$");
}
TEST_DEFINE(version_string,
	    "gpiod_version_string()",
	    0, { });
