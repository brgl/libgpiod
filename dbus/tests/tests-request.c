// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiodbus.h>
#include <gpiosim-glib.h>

#include "daemon-process.h"
#include "helpers.h"

#define GPIOD_TEST_GROUP "gpiodbus/request"

static GVariant *make_empty_request_config(void)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	return g_variant_ref_sink(g_variant_builder_end(&builder));
}

static GVariant *make_input_lines_line_config(void)
{
	g_autoptr(GVariant) output_values = NULL;
	g_autoptr(GVariant) line_settings = NULL;
	g_autoptr(GVariant) line_offsets = NULL;
	g_autoptr(GVariant) line_configs = NULL;
	g_autoptr(GVariant) line_config = NULL;
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder, g_variant_new_uint32(3));
	g_variant_builder_add_value(&builder, g_variant_new_uint32(5));
	g_variant_builder_add_value(&builder, g_variant_new_uint32(7));
	line_offsets = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder,
				g_variant_new("{sv}", "direction",
					      g_variant_new_string("input")));
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

GPIOD_TEST_CASE(request_input_lines)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autoptr(GVariant) request_config = NULL;
	g_autoptr(GVariant) line_config = NULL;
	g_autofree gchar *request_path = NULL;
	g_autoptr(GpiodbusChip) chip = NULL;
	g_autofree gchar *obj_path = NULL;

	mgr = gpiodbus_daemon_process_new();
	gpiodbus_test_wait_for_sim_intf(sim);

	obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				   g_gpiosim_chip_get_name(sim));
	chip = gpiodbus_test_get_chip_proxy_or_fail(obj_path);

	line_config = make_input_lines_line_config();
	request_config = make_empty_request_config();

	gpiodbus_test_chip_call_request_lines_sync_or_fail(chip, line_config,
							   request_config,
							   &request_path);
}

GPIOD_TEST_CASE(release_request)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autoptr(GVariant) request_config = NULL;
	g_autoptr(GpiodbusRequest) request = NULL;
	g_autoptr(GVariant) line_config = NULL;
	g_autofree gchar *request_path = NULL;
	g_autoptr(GpiodbusChip) chip = NULL;
	g_autofree gchar *obj_path = NULL;

	mgr = gpiodbus_daemon_process_new();
	gpiodbus_test_wait_for_sim_intf(sim);

	obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				   g_gpiosim_chip_get_name(sim));
	chip = gpiodbus_test_get_chip_proxy_or_fail(obj_path);

	line_config = make_input_lines_line_config();
	request_config = make_empty_request_config();

	gpiodbus_test_chip_call_request_lines_sync_or_fail(chip, line_config,
							   request_config,
							   &request_path);

	request = gpiodbus_test_get_request_proxy_or_fail(request_path);
	gpiodbus_test_request_call_release_sync_or_fail(request);
}
