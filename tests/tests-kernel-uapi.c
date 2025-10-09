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

GPIOD_TEST_CASE(open_drain_emulation)
{
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	g_autoptr(struct_gpiod_line_info) info = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_drive(settings, GPIOD_LINE_DRIVE_OPEN_DRAIN);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);
	request = gpiod_test_chip_request_lines_or_fail(chip, NULL, line_cfg);

	ret = gpiod_line_request_set_value(request, offset,
					   GPIOD_LINE_VALUE_ACTIVE);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/*
	 * The open-drain emulation in the kernel will set the line's direction
	 * to input but NOT set FLAG_IS_OUT. Let's verify the direction is
	 * still reported as output.
	 */
	info = gpiod_test_chip_get_line_info_or_fail(chip, offset);
	g_assert_cmpint(gpiod_line_info_get_direction(info), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_info_get_drive(info), ==,
			GPIOD_LINE_DRIVE_OPEN_DRAIN);

	/*
	 * The actual line is not being actively driven, so check that too on
	 * the gpio-sim end.
	 */
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, offset), ==,
			G_GPIOSIM_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(open_source_emulation)
{
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	g_autoptr(struct_gpiod_line_info) info = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_drive(settings, GPIOD_LINE_DRIVE_OPEN_SOURCE);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);
	request = gpiod_test_chip_request_lines_or_fail(chip, NULL, line_cfg);

	ret = gpiod_line_request_set_value(request, offset,
					   GPIOD_LINE_VALUE_INACTIVE);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/*
	 * The open-source emulation in the kernel will set the line's direction
	 * to input but NOT set FLAG_IS_OUT. Let's verify the direction is
	 * still reported as output.
	 */
	info = gpiod_test_chip_get_line_info_or_fail(chip, offset);
	g_assert_cmpint(gpiod_line_info_get_direction(info), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_info_get_drive(info), ==,
			GPIOD_LINE_DRIVE_OPEN_SOURCE);
}

GPIOD_TEST_CASE(valid_lines)
{
	static const guint invalid_lines[] = { 2, 4 };

	g_autoptr(GPIOSimChip) sim = NULL;
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_info) info0 = NULL;
	g_autoptr(struct_gpiod_line_info) info1 = NULL;
	g_autoptr(GVariant) vinval = NULL;

	vinval = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
					   invalid_lines, 2, sizeof(guint));
	g_variant_ref_sink(vinval);

	sim = g_gpiosim_chip_new("num-lines", 8,
				 "invalid-lines", vinval,
				 NULL);

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	info0 = gpiod_chip_get_line_info(chip, 0);
	info1 = gpiod_chip_get_line_info(chip, 2);
	g_assert_nonnull(info0);
	g_assert_nonnull(info1);
	gpiod_test_return_if_failed();

	g_assert_false(gpiod_line_info_is_used(info0));
	g_assert_true(gpiod_line_info_is_used(info1));
}
