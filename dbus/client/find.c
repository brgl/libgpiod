// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

static void find_line_in_chip(gpointer elem, gpointer user_data)
{
	g_autoptr(GpiodbusObject) line_obj = NULL;
	GpiodbusObject *chip_obj = elem;
	const gchar *name = user_data;
	GpiodbusChip *chip;
	GpiodbusLine *line;

	line_obj = get_line_obj_by_name_for_chip(chip_obj, name);
	if (!line_obj)
		return;

	chip = gpiodbus_object_peek_chip(chip_obj);
	line = gpiodbus_object_peek_line(line_obj);

	g_print("%s %u\n",
		gpiodbus_chip_get_name(chip),
		gpiodbus_line_get_offset(line));

	exit(EXIT_SUCCESS);
}

int gpiocli_find_main(int argc, char **argv)
{
	static const gchar *const summary =
"Gicen a line name, find the name of the parent chip and offset of the line within that chip.";

	static const gchar *const description =
"As line names are not guaranteed to be unique, this command finds the first line with given name.";

	g_autolist(GpiodbusObject) objs = NULL;
	g_auto(GStrv) line_name = NULL;

	const GOptionEntry opts[] = {
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &line_name,
			.arg_description	= "<line name>",
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);
	check_manager();

	if (!line_name)
		die_parsing_opts("line name must be specified");
	if (g_strv_length(line_name) != 1)
		die_parsing_opts("only one line can be mapped");

	objs = get_chip_objs(NULL);
	g_list_foreach(objs, find_line_in_chip, line_name[0]);

	/* If we got here, the line was not found. */
	die("line '%s' not found", line_name[0]);
	return EXIT_FAILURE;
}
