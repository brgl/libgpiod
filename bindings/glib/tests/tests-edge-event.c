// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gpiod-glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiosim-glib.h>

#include "helpers.h"

#define GPIOD_TEST_GROUP "glib/edge-event"

static gpointer falling_and_rising_edge_events(gpointer data)
{
	GPIOSimChip *sim = data;

	g_usleep(1000);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_UP);

	g_usleep(1000);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_DOWN);

	return NULL;
}

typedef struct {
	gboolean rising;
	gboolean falling;
	gboolean failed;
	guint64 falling_ts;
	guint64 rising_ts;
	guint falling_offset;
	guint rising_offset;
} EdgeEventCallbackData;

static void on_edge_event(GpiodglibLineRequest *request G_GNUC_UNUSED,
			  GpiodglibEdgeEvent *event, gpointer data)
{
	EdgeEventCallbackData *cb_data = data;

	if (gpiodglib_edge_event_get_event_type(event) ==
	    GPIODGLIB_EDGE_EVENT_FALLING_EDGE) {
		cb_data->falling = TRUE;
		cb_data->falling_ts =
			gpiodglib_edge_event_get_timestamp_ns(event);
		cb_data->falling_offset =
			gpiodglib_edge_event_get_line_offset(event);
	} else {
		cb_data->rising = TRUE;
		cb_data->rising_ts =
			gpiodglib_edge_event_get_timestamp_ns(event);
		cb_data->rising_offset =
			gpiodglib_edge_event_get_line_offset(event);
	}
}

static gboolean on_timeout(gpointer data)
{
	EdgeEventCallbackData *cb_data = data;

	g_test_fail_printf("timeout while waiting for edge events");
	cb_data->failed = TRUE;

	return G_SOURCE_CONTINUE;
}

GPIOD_TEST_CASE(read_both_events)
{
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) config = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GThread) thread = NULL;
	EdgeEventCallbackData cb_data = { };
	guint timeout_id;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT,
			"edge-detection", GPIODGLIB_LINE_EDGE_BOTH, NULL);
	config = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(config, offsets,
							     settings);

	request = gpiodglib_test_chip_request_lines_or_fail(chip, NULL, config);

	g_signal_connect(request, "edge-event",
			 G_CALLBACK(on_edge_event), &cb_data);
	timeout_id = g_timeout_add_seconds(5, on_timeout, &cb_data);

	thread = g_thread_new("rising-falling-edge-events",
			      falling_and_rising_edge_events, sim);
	g_thread_ref(thread);

	while (!cb_data.failed && (!cb_data.falling || !cb_data.rising))
		g_main_context_iteration(NULL, TRUE);

	g_source_remove(timeout_id);
	g_thread_join(thread);

	g_assert_cmpuint(cb_data.falling_ts, >, cb_data.rising_ts);
	g_assert_cmpuint(cb_data.falling_offset, ==, offset);
	g_assert_cmpuint(cb_data.rising_offset, ==, offset);
}

typedef struct {
	gboolean failed;
	gboolean first;
	gboolean second;
	guint first_offset;
	guint second_offset;
	gulong first_line_seqno;
	gulong second_line_seqno;
	gulong first_global_seqno;
	gulong second_global_seqno;
} SeqnoCallbackData;

static void on_seqno_edge_event(GpiodglibLineRequest *request G_GNUC_UNUSED,
				GpiodglibEdgeEvent *event, gpointer data)
{
	SeqnoCallbackData *cb_data = data;

	if (!cb_data->first) {
		cb_data->first_offset =
			gpiodglib_edge_event_get_line_offset(event);
		cb_data->first_line_seqno =
			gpiodglib_edge_event_get_line_seqno(event);
		cb_data->first_global_seqno =
			gpiodglib_edge_event_get_global_seqno(event);
		cb_data->first = TRUE;
	} else {
		cb_data->second_offset =
			gpiodglib_edge_event_get_line_offset(event);
		cb_data->second_line_seqno =
			gpiodglib_edge_event_get_line_seqno(event);
		cb_data->second_global_seqno =
			gpiodglib_edge_event_get_global_seqno(event);
		cb_data->second = TRUE;
	}
}

static gpointer rising_edge_events_on_two_offsets(gpointer data)
{
	GPIOSimChip *sim = data;

	g_usleep(1000);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_UP);

	g_usleep(1000);

	g_gpiosim_chip_set_pull(sim, 3, G_GPIOSIM_PULL_UP);

	return NULL;
}

static gboolean on_seqno_timeout(gpointer data)
{
	SeqnoCallbackData *cb_data = data;

	g_test_fail_printf("timeout while waiting for edge events");
	cb_data->failed = TRUE;

	return G_SOURCE_CONTINUE;
}

GPIOD_TEST_CASE(seqno)
{
	static const guint offset_vals[] = { 2, 3 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) config = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GThread) thread = NULL;
	SeqnoCallbackData cb_data = { };
	guint timeout_id;

	chip = gpiodglib_test_new_chip_or_fail(
		g_gpiosim_chip_get_dev_path(sim));
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT,
			"edge-detection", GPIODGLIB_LINE_EDGE_BOTH, NULL);
	config = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(offset_vals, 2,
						  sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(config, offsets,
							     settings);

	request = gpiodglib_test_chip_request_lines_or_fail(chip, NULL,
							    config);
	g_signal_connect(request, "edge-event",
			 G_CALLBACK(on_seqno_edge_event), &cb_data);

	timeout_id = g_timeout_add_seconds(5, on_seqno_timeout, &cb_data);

	thread = g_thread_new("two-rising-edge-events",
			      rising_edge_events_on_two_offsets, sim);
	g_thread_ref(thread);

	while (!cb_data.failed && (!cb_data.first || !cb_data.second))
		g_main_context_iteration(NULL, TRUE);

	g_source_remove(timeout_id);
	g_thread_join(thread);

	g_assert_cmpuint(cb_data.first_offset, ==, 2);
	g_assert_cmpuint(cb_data.second_offset, ==, 3);
	g_assert_cmpuint(cb_data.first_line_seqno, ==, 1);
	g_assert_cmpuint(cb_data.second_line_seqno, ==, 1);
	g_assert_cmpuint(cb_data.first_global_seqno, ==, 1);
	g_assert_cmpuint(cb_data.second_global_seqno, ==, 2);
}
