// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

int gpiocli_get_main(int argc, char **argv)
{
	static const gchar *const summary =
"Get values of one or more GPIO lines.";

	static const gchar *const description =
"If -r/--request is specified then all the lines must belong to the same\n"
"request (and - by extension - the same chip).\n"
"\n"
"If no lines are specified but -r/--request was passed then all lines within\n"
"the request will be used.";

	const gchar *request_name = NULL, *chip_path, *req_path;
	gboolean ret, unquoted = FALSE, numeric = FALSE;
	g_autoptr(GpiodbusObject) chip_obj = NULL;
	g_autoptr(GpiodbusObject) req_obj = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	g_auto(GStrv) lines = NULL;
	GpiodbusRequest *request;
	GVariantBuilder builder;
	GpiodbusLine *line;
	gsize num_lines, i;
	GVariantIter iter;
	guint offset;
	gint value;

	const GOptionEntry opts[] = {
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
			.long_name		= "unquoted",
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &unquoted,
			.description		= "don't quote line names",
		},
		{
			.long_name		= "numeric",
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &numeric,
			.description		= "display line values as '0' (inactive) or '1' (active)",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &lines,
			.arg_description	= "[line0] [line1]...",
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);
	check_manager();

	if (!lines && !request_name)
		die_parsing_opts("either at least one line or the request must be specified");

	offsets = g_array_new(FALSE, TRUE, sizeof(guint));
	num_lines = lines ? g_strv_length(lines) : 0;

	if (!request_name) {
		/*
		 * TODO Limit the number of D-Bus calls by gathering the requests
		 * and their relevant lines into a container of some kind first.
		 */

		values = g_array_sized_new(FALSE, TRUE, sizeof(gint),
					   num_lines);

		for (i = 0; i < num_lines; i++) {
			g_autoptr(GpiodbusRequest) req_proxy = NULL;
			g_autoptr(GpiodbusObject) line_obj = NULL;
			g_autoptr(GVariant) arg_offsets = NULL;
			g_autoptr(GVariant) arg_values = NULL;

			ret = get_line_obj_by_name(lines[i], &line_obj, NULL);
			if (!ret)
				die("Line not found: %s\n", lines[i]);

			line = gpiodbus_object_peek_line(line_obj);
			req_path = gpiodbus_line_get_request_path(line);

			if (!gpiodbus_line_get_managed(line))
				die("Line '%s' not managed by gpio-manager, must be requested first",
				    lines[i]);

			req_proxy = gpiodbus_request_proxy_new_for_bus_sync(
							G_BUS_TYPE_SYSTEM,
							G_DBUS_PROXY_FLAGS_NONE,
							"io.gpiod1", req_path,
							NULL, &err);
			if (err)
				die_gerror(err,
					   "Failed to get D-Bus proxy for '%s'",
					   req_path);

			offset = gpiodbus_line_get_offset(line);
			g_array_append_val(offsets, offset);

			g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
			g_variant_builder_add(&builder, "u", offset);
			arg_offsets = g_variant_ref_sink(
					g_variant_builder_end(&builder));

			ret = gpiodbus_request_call_get_values_sync(
							req_proxy, arg_offsets,
							G_DBUS_CALL_FLAGS_NONE,
							-1, &arg_values, NULL,
							&err);
			if (!ret)
				die_gerror(err, "Failed to get line values");

			g_variant_iter_init(&iter, arg_values);
			while (g_variant_iter_next(&iter, "i", &value))
				g_array_append_val(values, value);
		}
	} else {
		g_autoptr(GVariant) arg_offsets = NULL;
		g_autoptr(GVariant) arg_values = NULL;

		req_obj = get_request_obj(request_name);
		request = gpiodbus_object_peek_request(req_obj);
		chip_path = gpiodbus_request_get_chip_path(request);
		chip_obj = get_chip_obj_by_path(chip_path);

		if (lines) {
			for (i = 0; i < num_lines; i++) {
				g_autoptr(GpiodbusObject) line_obj = NULL;

				line_obj = get_line_obj_by_name_for_chip(
							chip_obj, lines[i]);
				if (!line_obj)
					die("Line not found: %s\n", lines[i]);

				line = gpiodbus_object_peek_line(line_obj);

				if (!gpiodbus_line_get_managed(line))
					die("Line '%s' not managed by gpio-manager, must be requested first",
					    lines[i]);

				offset = gpiodbus_line_get_offset(line);
				g_array_append_val(offsets, offset);
			}
		} else {
			offsets = get_request_offsets(request);
			num_lines = offsets->len;
		}

		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
		for (i = 0; i < offsets->len; i++)
			g_variant_builder_add(&builder, "u",
					      g_array_index(offsets, guint, i));
		arg_offsets = g_variant_ref_sink(
					g_variant_builder_end(&builder));

		ret = gpiodbus_request_call_get_values_sync(
							request, arg_offsets,
							G_DBUS_CALL_FLAGS_NONE,
							-1, &arg_values, NULL,
							&err);
		if (!ret)
			die_gerror(err, "Failed to get line values");

		values = g_array_sized_new(FALSE, TRUE, sizeof(gint),
					   g_variant_n_children(arg_values));

		g_variant_iter_init(&iter, arg_values);
		while (g_variant_iter_next(&iter, "i", &value))
			g_array_append_val(values, value);
	}

	for (i = 0; i < num_lines; i++) {
		if (!unquoted)
			g_print("\"");

		if (lines)
			g_print("%s", lines[i]);
		else
			g_print("%u", g_array_index(offsets, guint, i));

		if (!unquoted)
			g_print("\"");

		g_print("=%s", g_array_index(values, guint, i) ?
					numeric ? "1" : "active" :
					numeric ? "0" : "inactive");

		if (i != (num_lines - 1))
			g_print(" ");
	}
	g_print("\n");

	return EXIT_SUCCESS;
}
