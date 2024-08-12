// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

static void show_chip(gpointer elem, gpointer user_data G_GNUC_UNUSED)
{
	GpiodbusObject *chip_obj = elem;
	GpiodbusChip *chip;

	chip = gpiodbus_object_peek_chip(chip_obj);

	g_print("%s [%s] (%u lines)\n",
		gpiodbus_chip_get_name(chip),
		gpiodbus_chip_get_label(chip),
		gpiodbus_chip_get_num_lines(chip));
}

int gpiocli_detect_main(int argc, char **argv)
{
	static const gchar *const summary =
"List GPIO chips, print their labels and number of GPIO lines.";

	static const gchar *const description =
"Chips may be identified by name or number. e.g. '0' and 'gpiochip0' refer to\n"
"the same chip.\n"
"\n"
"If no chips are specified - display information for all chips in the system.";

	g_autolist(GpiodbusObject) chip_objs = NULL;
	g_auto(GStrv) chip_names = NULL;

	const GOptionEntry opts[] = {
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &chip_names,
			.arg_description	= "[chip]...",
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);
	check_manager();

	chip_objs = get_chip_objs(chip_names);
	g_list_foreach(chip_objs, show_chip, NULL);

	return EXIT_SUCCESS;
}
