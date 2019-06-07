// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <string.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "misc"

GPIOD_TEST_CASE(version_string, 0, { 1 })
{
	g_autoptr(GRegex) regex = NULL;
	GError *err = NULL;
	const gchar *ver;

	ver = gpiod_version_string();
	g_assert_nonnull(ver);
	gpiod_test_return_if_failed();
	g_assert_cmpuint(strlen(ver), >, 0);

	regex = g_regex_new("^[0-9]+\\.[0-9]+[0-9a-zA-Z\\.]*$", 0, 0, &err);
	g_assert_null(err);
	gpiod_test_return_if_failed();
	g_assert_true(g_regex_match(regex, ver, 0, NULL));
}
