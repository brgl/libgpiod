// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

typedef struct {
	LineConfigOpts line_cfg_opts;
	const gchar *consumer;
} RequestOpts;

typedef struct {
	const gchar *request_path;
	gboolean done;
} RequestWaitData;

static GVariant *make_request_config(RequestOpts *opts)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "consumer",
				      g_variant_new_string(opts->consumer)));

	return g_variant_ref_sink(g_variant_builder_end(&builder));
}

static gboolean on_timeout(gpointer user_data G_GNUC_UNUSED)
{
	die("wait for request to appear timed out!");
}

static void obj_match_request_path(GpiodbusObject *obj, RequestWaitData *data)
{
	if (g_strcmp0(g_dbus_object_get_object_path(G_DBUS_OBJECT(obj)),
		      data->request_path) == 0)
		data->done = TRUE;
}

static void match_request_path(gpointer elem, gpointer user_data)
{
	RequestWaitData *data = user_data;
	GpiodbusObject *obj = elem;

	obj_match_request_path(obj, data);
}

static void on_object_added(GDBusObjectManager *manager G_GNUC_UNUSED,
			    GpiodbusObject *obj, gpointer user_data)
{
	RequestWaitData *data = user_data;

	obj_match_request_path(GPIODBUS_OBJECT(obj), data);
}

static void wait_for_request(const gchar *request_path)
{
	RequestWaitData data = { .request_path = request_path };
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autolist(GpiodbusObject) objs = NULL;

	manager = get_object_manager_client("/io/gpiod1/requests");

	g_signal_connect(manager, "object-added",
			 G_CALLBACK(on_object_added), &data);

	objs = g_dbus_object_manager_get_objects(manager);
	g_list_foreach(objs, match_request_path, &data);

	g_timeout_add(5000, on_timeout, NULL);

	while (!data.done)
		g_main_context_iteration(NULL, TRUE);
}

static int
request_lines(GList *line_names, const gchar *chip_name, RequestOpts *req_opts)
{
	g_autoptr(GpiodbusObject) chip_obj = NULL;
	g_autoptr(GVariant) request_config = NULL;
	g_autoptr(GVariant) line_config = NULL;
	g_autofree gchar *request_path = NULL;
	g_autofree gchar *request_name = NULL;
	g_autofree gchar *dyn_name = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	GpiodbusLine *line;
	GpiodbusChip *chip;
	GString *line_name;
	guint i, *offset;
	gboolean ret;
	GList *pos;
	gsize llen;

	llen = g_list_length(line_names);
	offsets = g_array_sized_new(FALSE, TRUE, sizeof(guint), llen);
	g_array_set_size(offsets, llen);

	if (chip_name)
		chip_obj = get_chip_obj(chip_name);

	for (i = 0, pos = g_list_first(line_names);
	     i < llen;
	     i++, pos = g_list_next(pos)) {
		g_autoptr(GpiodbusObject) line_obj = NULL;

		line_name = pos->data;

		if (chip_obj) {
			line_obj = get_line_obj_by_name_for_chip(chip_obj,
								line_name->str);
			if (!line_obj) {
				if (dyn_name) {
					ret = get_line_obj_by_name(
							line_name->str,
							&line_obj, NULL);
					if (ret)
						/*
						 * This means the line exists
						 * but on a different chip.
						 */
						die("all requested lines must belong to the same chip");
				}

				die("no line '%s' on chip '%s'",
				    line_name->str, chip_name);
			}
		} else {
			ret = get_line_obj_by_name(line_name->str, &line_obj,
						   &chip_obj);
			if (!ret)
				die("line '%s' not found", line_name->str);

			dyn_name = g_path_get_basename(
					g_dbus_object_get_object_path(
						G_DBUS_OBJECT(chip_obj)));
			chip_name = dyn_name;
		}

		line = gpiodbus_object_peek_line(line_obj);
		offset = &g_array_index(offsets, guint, i);
		*offset = gpiodbus_line_get_offset(line);
	}

	chip = gpiodbus_object_peek_chip(chip_obj);
	line_config = make_line_config(offsets, &req_opts->line_cfg_opts);
	request_config = make_request_config(req_opts);

	ret = gpiodbus_chip_call_request_lines_sync(chip, line_config,
						    request_config,
						    G_DBUS_CALL_FLAGS_NONE, -1,
						    &request_path, NULL, &err);
	if (err)
		die_gerror(err, "failed to request lines from chip '%s'",
			   chip_name);

	wait_for_request(request_path);

	request_name = g_path_get_basename(request_path);
	g_print("%s\n", request_name);

	return EXIT_SUCCESS;
}

int gpiocli_request_main(int argc, char **argv)
{
	static const gchar *const summary =
"Request a set of GPIO lines for exclusive usage by the gpio-manager.";

	g_autoptr(GArray) output_values = NULL;
	g_autolist(GString) line_names = NULL;
	const gchar *chip_name = NULL;
	g_auto(GStrv) lines = NULL;
	RequestOpts req_opts = {};
	gsize llen;
	gint val;
	guint i;

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
			.long_name		= "consumer",
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING,
			.arg_data		= &req_opts.consumer,
			.description		= "Consumer string (defaults to program name)",
			.arg_description	= "<consumer name>",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &lines,
			.arg_description	= "<line1>[=value1] [line2[=value2]] ...",
		},
		LINE_CONFIG_OPTIONS(&req_opts.line_cfg_opts),
		{ }
	};

	parse_options(opts, summary, NULL, &argc, &argv);
	validate_line_config_opts(&req_opts.line_cfg_opts);

	if (!lines)
		die_parsing_opts("At least one line must be specified");

	if (!req_opts.consumer)
		req_opts.consumer = "gpio-manager";

	for (i = 0, llen = g_strv_length(lines); i < llen; i++) {
		g_auto(GStrv) tokens = NULL;

		tokens = g_strsplit(lines[i], "=", 2);
		line_names = g_list_append(line_names, g_string_new(tokens[0]));
		if (g_strv_length(tokens) == 2) {
			if (!req_opts.line_cfg_opts.output)
				die_parsing_opts("Output values can only be set in output mode");

			if (!output_values)
				output_values = g_array_sized_new(FALSE, TRUE,
								  sizeof(gint),
								  llen);
			val = output_value_from_str(tokens[1]);
			g_array_append_val(output_values, val);
		}
	}

	if (output_values && req_opts.line_cfg_opts.input)
		die_parsing_opts("cannot set output values in input mode");

	if (output_values &&
	    (g_list_length(line_names) != output_values->len))
		die_parsing_opts("if values are set, they must be set for all lines");

	req_opts.line_cfg_opts.output_values = output_values;

	check_manager();

	return request_lines(line_names, chip_name, &req_opts);
}
