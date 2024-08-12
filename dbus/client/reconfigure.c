// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

int gpiocli_reconfigure_main(int argc, char **argv)
{
	static const gchar *const summary =
"Change the line configuration for an existing request.";

	g_autoptr(GpiodbusObject) req_obj = NULL;
	g_autoptr(GVariant) line_config = NULL;
	g_autoptr(GArray) output_values = NULL;
	LineConfigOpts line_cfg_opts = { };
	g_autoptr(GArray) offsets = NULL;
	g_auto(GStrv) remaining = NULL;
	g_autoptr(GError) err = NULL;
	GpiodbusRequest *request;
	gsize num_values;
	gboolean ret;
	gint val;
	guint i;

	const GOptionEntry opts[] = {
		LINE_CONFIG_OPTIONS(&line_cfg_opts),
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &remaining,
			.arg_description	= "<request> [value1] [value2]...",
		},
		{ }
	};

	parse_options(opts, summary, NULL, &argc, &argv);
	validate_line_config_opts(&line_cfg_opts);

	if (!remaining || g_strv_length(remaining) == 0)
		die_parsing_opts("Exactly one request to reconfigure must be specified.");

	num_values = g_strv_length(remaining) - 1;

	check_manager();

	req_obj = get_request_obj(remaining[0]);
	request = gpiodbus_object_peek_request(req_obj);
	offsets = get_request_offsets(request);

	if (num_values) {
		if (num_values != offsets->len)
			die_parsing_opts("The number of output values must correspond to the number of lines in the request");

		output_values = g_array_sized_new(FALSE, TRUE, sizeof(gint),
						  num_values);

		for (i = 0; i < num_values; i++) {
			val = output_value_from_str(remaining[i + 1]);
			g_array_append_val(output_values, val);
		}
	}

	line_cfg_opts.output_values = output_values;
	line_config = make_line_config(offsets, &line_cfg_opts);

	ret = gpiodbus_request_call_reconfigure_lines_sync(
						request, line_config,
						G_DBUS_CALL_FLAGS_NONE,
						-1, NULL, &err);
	if (!ret)
		die_gerror(err, "Failed to reconfigure lines");

	return EXIT_SUCCESS;
}
