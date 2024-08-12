// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Kent Gibson <warthog618@gmail.com>

#include <glib.h>
#include <gpiod.h>
#include <poll.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiosim-glib.h>

#include "helpers.h"

#define GPIOD_TEST_GROUP "kernel-uapi"

static gpointer falling_and_rising_edge_events(gpointer data)
{
	GPIOSimChip *sim = data;

	/*
	 * needs to be as long as several system timer ticks or resulting
	 * pulse width is unreliable and may get filtered by debounce.
	 */
	g_usleep(50000);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_UP);

	g_usleep(50000);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_DOWN);

	return NULL;
}

GPIOD_TEST_CASE(enable_debounce_then_edge_detection)
{
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	g_autoptr(GThread) thread = NULL;
	g_autoptr(struct_gpiod_edge_event_buffer) buffer = NULL;
	struct gpiod_edge_event *event;
	guint64 ts_rising, ts_falling;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();
	buffer = gpiod_test_create_edge_event_buffer_or_fail(64);

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_debounce_period_us(settings, 10);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);
	request = gpiod_test_chip_request_lines_or_fail(chip, NULL, line_cfg);

	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);
	gpiod_test_line_request_reconfigure_lines_or_fail(request, line_cfg);

	thread = g_thread_new("request-release",
			      falling_and_rising_edge_events, sim);
	g_thread_ref(thread);

	/* First event. */

	ret = gpiod_line_request_wait_edge_events(request, 1000000000);
	g_assert_cmpint(ret, >, 0);
	gpiod_test_join_thread_and_return_if_failed(thread);

	ret = gpiod_line_request_read_edge_events(request, buffer, 1);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_join_thread_and_return_if_failed(thread);

	g_assert_cmpuint(gpiod_edge_event_buffer_get_num_events(buffer), ==, 1);
	event = gpiod_edge_event_buffer_get_event(buffer, 0);
	g_assert_nonnull(event);
	gpiod_test_join_thread_and_return_if_failed(thread);

	g_assert_cmpint(gpiod_edge_event_get_event_type(event), ==,
			GPIOD_EDGE_EVENT_RISING_EDGE);
	g_assert_cmpuint(gpiod_edge_event_get_line_offset(event), ==, 2);
	ts_rising = gpiod_edge_event_get_timestamp_ns(event);

	/* Second event. */

	ret = gpiod_line_request_wait_edge_events(request, 1000000000);
	g_assert_cmpint(ret, >, 0);
	gpiod_test_join_thread_and_return_if_failed(thread);

	ret = gpiod_line_request_read_edge_events(request, buffer, 1);
	g_assert_cmpint(ret, ==, 1);
	gpiod_test_join_thread_and_return_if_failed(thread);

	g_assert_cmpuint(gpiod_edge_event_buffer_get_num_events(buffer), ==, 1);
	event = gpiod_edge_event_buffer_get_event(buffer, 0);
	g_assert_nonnull(event);
	gpiod_test_join_thread_and_return_if_failed(thread);

	g_assert_cmpint(gpiod_edge_event_get_event_type(event), ==,
			GPIOD_EDGE_EVENT_FALLING_EDGE);
	g_assert_cmpuint(gpiod_edge_event_get_line_offset(event), ==, 2);
	ts_falling = gpiod_edge_event_get_timestamp_ns(event);

	g_thread_join(thread);

	g_assert_cmpuint(ts_falling, >, ts_rising);
}
