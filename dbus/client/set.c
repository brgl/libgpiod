// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

static void free_str(gpointer data)
{
	GString *str = data;

	g_string_free(str, TRUE);
}

int gpiocli_set_main(int argc, char **argv)
{
	static const gchar *const summary =
"Set values of one or more GPIO lines.";

	static const gchar *const description =
"If -r/--request is specified then all the lines must belong to the same\n"
"request (and - by extension - the same chip).";

	const gchar *request_name = NULL, *chip_path, *req_path;
	g_autoptr(GpiodbusObject) chip_obj = NULL;
	g_autoptr(GpiodbusObject) req_obj = NULL;
	g_autoptr(GPtrArray) line_names = NULL;
	g_autoptr(GArray) values = NULL;
	const gchar *chip_name = NULL;
	g_autoptr(GError) err = NULL;
	g_auto(GStrv) lines = NULL;
	GpiodbusRequest *request;
	GVariantBuilder builder;
	GpiodbusLine *line;
	gsize num_lines, i;
	GString *line_name;
	gboolean ret;
	guint offset;
	gint val;

	const GOptionEntry opts[] = {
		{
			.long_name		= "chip",
			.short_name		= 'c',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING,
			.arg_data		= &chip_name,
			.description		=
"Explicitly specify the chip_name on which to resolve the lines which allows to use raw offsets instead of line names.",
			.arg_description	= "<chip name>",
		},
		{
			.long_name		= "request",
			.short_name		= 'r',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING,
			.arg_data		= &request_name,
			.description		= "restrict scope to a particular request",
			.arg_description	= "<request>",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &lines,
			.arg_description	= "<line1=value1> [line2=value2] ...",
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);

	if (!lines)
		die_parsing_opts("at least one line value must be specified");

	num_lines = g_strv_length(lines);
	line_names = g_ptr_array_new_full(num_lines, free_str);
	values = g_array_sized_new(FALSE, TRUE, sizeof(gint), num_lines);

	for (i = 0; i < num_lines; i++) {
		g_auto(GStrv) tokens = NULL;

		tokens = g_strsplit(lines[i], "=", 2);
		if (g_strv_length(tokens) != 2)
			die_parsing_opts("line must have a single value assigned");

		g_ptr_array_add(line_names, g_string_new(tokens[0]));
		val = output_value_from_str(tokens[1]);
		g_array_append_val(values, val);
	}

	check_manager();

	if (request_name) {
		g_autoptr(GVariant) arg_values = NULL;
		g_autoptr(GArray) offsets = NULL;

		req_obj = get_request_obj(request_name);
		request = gpiodbus_object_peek_request(req_obj);
		chip_path = gpiodbus_request_get_chip_path(request);
		chip_obj = get_chip_obj_by_path(chip_path);
		offsets = g_array_sized_new(FALSE, TRUE, sizeof(guint),
					    num_lines);

		for (i = 0; i < num_lines; i++) {
			g_autoptr(GpiodbusObject) line_obj = NULL;

			line_name = g_ptr_array_index(line_names, i);

			line_obj = get_line_obj_by_name_for_chip(chip_obj,
								line_name->str);
			if (!line_obj)
				die("Line not found: %s\n", line_name->str);

			line = gpiodbus_object_peek_line(line_obj);
			offset = gpiodbus_line_get_offset(line);
			g_array_append_val(offsets, offset);
		}

		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
		for (i = 0; i < num_lines; i++) {
			g_variant_builder_add(&builder, "{ui}",
					      g_array_index(offsets, guint, i),
					      g_array_index(values, gint, i));
		}

		arg_values = g_variant_ref_sink(
				g_variant_builder_end(&builder));

		ret = gpiodbus_request_call_set_values_sync(
							request, arg_values,
							G_DBUS_CALL_FLAGS_NONE,
							-1, NULL, &err);
		if (!ret)
			die_gerror(err, "Failed to set line values");

		return EXIT_SUCCESS;
	}

	for (i = 0; i < num_lines; i++) {
		g_autoptr(GpiodbusRequest) req_proxy = NULL;
		g_autoptr(GpiodbusObject) line_obj = NULL;
		g_autoptr(GVariant) arg_values = NULL;

		line_name = g_ptr_array_index(line_names, i);

		if (chip_name) {
			chip_obj = get_chip_obj(chip_name);
			line_obj = get_line_obj_by_name_for_chip(chip_obj, line_name->str);
			if (!line_obj)
				die("Line '%s' not found on chip '%s'", line_name->str, chip_name);
		} else {
			ret = get_line_obj_by_name(line_name->str, &line_obj, NULL);
			if (!ret)
				die("Line not found: %s\n", line_name->str);
		}

		line = gpiodbus_object_peek_line(line_obj);
		req_path = gpiodbus_line_get_request_path(line);

		if (!gpiodbus_line_get_managed(line))
			die("Line '%s' not managed by gpio-manager, must be requested first",
			    line_name->str);

		req_proxy = gpiodbus_request_proxy_new_for_bus_sync(
						G_BUS_TYPE_SYSTEM,
						G_DBUS_PROXY_FLAGS_NONE,
						"io.gpiod1", req_path,
						NULL, &err);
		if (err)
			die_gerror(err, "Failed to get D-Bus proxy for '%s'",
				   req_path);

		offset = gpiodbus_line_get_offset(line);

		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
		g_variant_builder_add(&builder, "{ui}", offset,
				      g_array_index(values, gint, i));
		arg_values = g_variant_ref_sink(
				g_variant_builder_end(&builder));

		ret = gpiodbus_request_call_set_values_sync(
						req_proxy, arg_values,
						G_DBUS_CALL_FLAGS_NONE, -1,
						NULL, &err);
		if (!ret)
			die_gerror(err, "Failed to set line values");
	}

	return EXIT_SUCCESS;
}
