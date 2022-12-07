// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <glib.h>
#include <gpiod.h>

#include "gpiod-test.h"
#include "gpiod-test-helpers.h"

#define GPIOD_TEST_GROUP "request-config"

GPIOD_TEST_CASE(default_config)
{
	g_autoptr(struct_gpiod_request_config) config = NULL;

	config = gpiod_test_create_request_config_or_fail();

	g_assert_null(gpiod_request_config_get_consumer(config));
	g_assert_cmpuint(gpiod_request_config_get_event_buffer_size(config), ==,
			 0);
}

GPIOD_TEST_CASE(set_consumer)
{
	g_autoptr(struct_gpiod_request_config) config = NULL;

	config = gpiod_test_create_request_config_or_fail();

	gpiod_request_config_set_consumer(config, "foobar");
	g_assert_cmpstr(gpiod_request_config_get_consumer(config), ==,
			"foobar");
}

GPIOD_TEST_CASE(set_event_buffer_size)
{
	g_autoptr(struct_gpiod_request_config) config = NULL;

	config = gpiod_test_create_request_config_or_fail();

	gpiod_request_config_set_event_buffer_size(config, 128);
	g_assert_cmpuint(gpiod_request_config_get_event_buffer_size(config), ==,
			 128);
}
