// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <glib.h>
#include <gpiod-glib.h>
#include <gpiod-test.h>

#include "helpers.h"

#define GPIOD_TEST_GROUP "glib/request-config"

GPIOD_TEST_CASE(default_config)
{
	g_autoptr(GpiodglibRequestConfig) config = NULL;
	g_autofree gchar *consumer = NULL;

	config = gpiodglib_request_config_new(NULL);
	consumer = gpiodglib_request_config_dup_consumer(config);

	g_assert_null(consumer);
	g_assert_cmpuint(gpiodglib_request_config_get_event_buffer_size(config),
			 ==, 0);
}

GPIOD_TEST_CASE(set_consumer)
{
	g_autoptr(GpiodglibRequestConfig) config = NULL;
	g_autofree gchar *consumer = NULL;

	config = gpiodglib_request_config_new(NULL);

	gpiodglib_request_config_set_consumer(config, "foobar");
	consumer = gpiodglib_request_config_dup_consumer(config);
	g_assert_cmpstr(consumer, ==, "foobar");

	gpiodglib_request_config_set_consumer(config, NULL);
	g_free(consumer);
	consumer = gpiodglib_request_config_dup_consumer(config);
	g_assert_null(consumer);
}

GPIOD_TEST_CASE(set_event_buffer_size)
{
	g_autoptr(GpiodglibRequestConfig) config = NULL;

	config = gpiodglib_request_config_new(NULL);

	gpiodglib_request_config_set_event_buffer_size(config, 128);
	g_assert_cmpuint(gpiodglib_request_config_get_event_buffer_size(config),
			 ==, 128);
}

GPIOD_TEST_CASE(set_properties_in_constructor)
{
	g_autoptr(GpiodglibRequestConfig) config = NULL;
	g_autofree gchar *consumer = NULL;

	config = gpiodglib_request_config_new("consumer", "foobar",
					    "event-buffer-size", 64, NULL);
	consumer = gpiodglib_request_config_dup_consumer(config);
	g_assert_cmpstr(consumer, ==, "foobar");
	g_assert_cmpuint(gpiodglib_request_config_get_event_buffer_size(config),
			 ==, 64);
}
