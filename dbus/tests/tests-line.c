// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiodbus.h>
#include <gpiosim-glib.h>

#include "daemon-process.h"
#include "helpers.h"

#define GPIOD_TEST_GROUP "gpiodbus/line"

GPIOD_TEST_CASE(read_line_properties)
{
	static const GPIOSimLineName names[] = {
		{ .offset = 1, .name = "foo", },
		{ .offset = 2, .name = "bar", },
		{ .offset = 4, .name = "baz", },
		{ .offset = 5, .name = "xyz", },
		{ }
	};

	static const GPIOSimHog hogs[] = {
		{
			.offset = 3,
			.name = "hog3",
			.direction = G_GPIOSIM_DIRECTION_OUTPUT_HIGH,
		},
		{
			.offset = 4,
			.name = "hog4",
			.direction = G_GPIOSIM_DIRECTION_OUTPUT_LOW,
		},
		{ }
	};

	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autoptr(GpiodbusLine) line4 = NULL;
	g_autoptr(GpiodbusLine) line6 = NULL;
	g_autofree gchar *obj_path_4 = NULL;
	g_autofree gchar *obj_path_6 = NULL;
	g_autoptr(GPIOSimChip) sim = NULL;
	g_autoptr(GVariant) vnames = g_gpiosim_package_line_names(names);
	g_autoptr(GVariant) vhogs = g_gpiosim_package_hogs(hogs);

	sim = g_gpiosim_chip_new(
			"num-lines", 8,
			"line-names", vnames,
			"hogs", vhogs,
			NULL);

	mgr = gpiodbus_daemon_process_new();
	gpiodbus_test_wait_for_sim_intf(sim);

	obj_path_4 = g_strdup_printf("/io/gpiod1/chips/%s/line4",
				     g_gpiosim_chip_get_name(sim));
	line4 = gpiodbus_test_get_line_proxy_or_fail(obj_path_4);

	obj_path_6 = g_strdup_printf("/io/gpiod1/chips/%s/line6",
				     g_gpiosim_chip_get_name(sim));
	line6 = gpiodbus_test_get_line_proxy_or_fail(obj_path_6);

	g_assert_cmpuint(gpiodbus_line_get_offset(line4), ==, 4);
	g_assert_cmpstr(gpiodbus_line_get_name(line4), ==, "baz");
	g_assert_cmpstr(gpiodbus_line_get_consumer(line4), ==, "hog4");
	g_assert_true(gpiodbus_line_get_used(line4));
	g_assert_false(gpiodbus_line_get_managed(line4));
	g_assert_cmpstr(gpiodbus_line_get_direction(line4), ==, "output");
	g_assert_cmpstr(gpiodbus_line_get_edge_detection(line4), ==, "none");
	g_assert_false(gpiodbus_line_get_active_low(line4));
	g_assert_cmpstr(gpiodbus_line_get_bias(line4), ==, "unknown");
	g_assert_cmpstr(gpiodbus_line_get_drive(line4), ==, "push-pull");
	g_assert_cmpstr(gpiodbus_line_get_event_clock(line4), ==, "monotonic");
	g_assert_false(gpiodbus_line_get_debounced(line4));
	g_assert_cmpuint(gpiodbus_line_get_debounce_period_us(line4), ==, 0);

	g_assert_cmpuint(gpiodbus_line_get_offset(line6), ==, 6);
	g_assert_cmpstr(gpiodbus_line_get_name(line6), ==, "");
	g_assert_cmpstr(gpiodbus_line_get_consumer(line6), ==, "");
	g_assert_false(gpiodbus_line_get_used(line6));
}

static gboolean on_timeout(gpointer user_data)
{
	gboolean *timed_out = user_data;

	*timed_out = TRUE;

	return G_SOURCE_REMOVE;
}

static void
on_properties_changed(GpiodbusLine *line G_GNUC_UNUSED,
		      GVariant *changed_properties,
		      GStrv invalidated_properties G_GNUC_UNUSED,
		      gpointer user_data)
{
	GHashTable *changed_props = user_data;
	GVariantIter iter;
	GVariant *variant;
	gchar *str;

	g_variant_iter_init(&iter, changed_properties);
	while (g_variant_iter_next(&iter, "{sv}", &str, &variant)) {
		g_hash_table_insert(changed_props, str, NULL);
		g_variant_unref(variant);
	}
}

static void check_props_requested(GHashTable *props)
{
	if (!g_hash_table_contains(props, "Direction") ||
	    !g_hash_table_contains(props, "Consumer") ||
	    !g_hash_table_contains(props, "Used") ||
	    !g_hash_table_contains(props, "RequestPath") ||
	    !g_hash_table_contains(props, "Managed"))
		g_test_fail_printf("Not all expected properties have changed");
}

static void check_props_released(GHashTable *props)
{
	if (!g_hash_table_contains(props, "RequestPath") ||
	    !g_hash_table_contains(props, "Consumer") ||
	    !g_hash_table_contains(props, "Used") ||
	    !g_hash_table_contains(props, "Managed"))
		g_test_fail_printf("Not all expected properties have changed");
}

static GVariant *make_props_changed_line_config(void)
{
	g_autoptr(GVariant) output_values = NULL;
	g_autoptr(GVariant) line_settings = NULL;
	g_autoptr(GVariant) line_offsets = NULL;
	g_autoptr(GVariant) line_configs = NULL;
	g_autoptr(GVariant) line_config = NULL;
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder, g_variant_new_uint32(4));
	line_offsets = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder,
				g_variant_new("{sv}", "direction",
					      g_variant_new_string("output")));
	line_settings = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&builder, g_variant_ref(line_offsets));
	g_variant_builder_add_value(&builder, g_variant_ref(line_settings));
	line_config = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder, g_variant_ref(line_config));
	line_configs = g_variant_builder_end(&builder);

	output_values = g_variant_new("ai", NULL);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&builder, g_variant_ref(line_configs));
	g_variant_builder_add_value(&builder, g_variant_ref(output_values));

	return g_variant_ref_sink(g_variant_builder_end(&builder));
}

GPIOD_TEST_CASE(properties_changed)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autoptr(GHashTable) changed_props = NULL;
	g_autoptr(GpiodbusRequest) request = NULL;
	g_autoptr(GVariant) request_config = NULL;
	g_autoptr(GVariant) line_config = NULL;
	g_autofree gchar *line_obj_path = NULL;
	g_autofree gchar *chip_obj_path = NULL;
	g_autofree gchar *request_path = NULL;
	g_autoptr(GpiodbusChip) chip = NULL;
	g_autoptr(GpiodbusLine) line = NULL;
	gboolean timed_out = FALSE;
	guint timeout_id;

	mgr = gpiodbus_daemon_process_new();
	gpiodbus_test_wait_for_sim_intf(sim);

	line_obj_path = g_strdup_printf("/io/gpiod1/chips/%s/line4",
					g_gpiosim_chip_get_name(sim));
	line = gpiodbus_test_get_line_proxy_or_fail(line_obj_path);

	chip_obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
					g_gpiosim_chip_get_name(sim));
	chip = gpiodbus_test_get_chip_proxy_or_fail(chip_obj_path);

	changed_props = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
						      NULL);

	g_signal_connect(line, "g-properties-changed",
			 G_CALLBACK(on_properties_changed), changed_props);
	timeout_id = g_timeout_add_seconds(5, on_timeout, &timed_out);

	line_config = make_props_changed_line_config();
	request_config = gpiodbus_test_make_empty_request_config();

	gpiodbus_test_chip_call_request_lines_sync_or_fail(chip, line_config,
							   request_config,
							   &request_path);

	while (g_hash_table_size(changed_props) < 5 && !timed_out)
		g_main_context_iteration(NULL, TRUE);

	check_props_requested(changed_props);

	g_hash_table_destroy(g_hash_table_ref(changed_props));

	request = gpiodbus_test_get_request_proxy_or_fail(request_path);
	gpiodbus_test_request_call_release_sync_or_fail(request);

	while (g_hash_table_size(changed_props) < 4 && !timed_out)
		g_main_context_iteration(NULL, TRUE);

	check_props_released(changed_props);

	if (timed_out) {
		g_test_fail_printf("timeout reached waiting for line properties to change");
		return;
	}

	g_source_remove(timeout_id);
}
