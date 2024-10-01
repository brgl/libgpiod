// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>
#include <string.h>

#include "common.h"

static gchar *make_line_name(const gchar *name)
{
	if (!name)
		return g_strdup("unnamed");

	return g_strdup_printf("\"%s\"", name);
}

static void do_print_line_info(GpiodbusObject *line_obj,
			       GpiodbusObject *chip_obj)
{
	g_autoptr(LineProperties) props = NULL;
	g_autoptr(GString) attributes = NULL;
	g_autofree gchar *line_name = NULL;
	GpiodbusChip *chip;

	props = get_line_properties(gpiodbus_object_peek_line(line_obj));
	line_name = make_line_name(props->name);

	attributes = g_string_new("[");

	if (props->used)
		g_string_append_printf(attributes, "used,consumer=\"%s\",",
				       props->consumer);

	if (props->managed)
		g_string_append_printf(attributes, "managed=\"%s\",",
				       props->request_name);

	if (props->edge) {
		g_string_append_printf(attributes, "edges=%s,event-clock=%s,",
				       props->edge, props->event_clock);
		if (props->debounced)
			g_string_append_printf(attributes,
					       "debounce-period=%"G_GUINT64_FORMAT",",
					       props->debounce_period);
	}

	if (props->bias)
		g_string_append_printf(attributes, "bias=%s,", props->bias);

	if (props->active_low)
		attributes = g_string_append(attributes, "active-low,");

	g_string_append_printf(attributes, "%s", props->direction);

	if (g_strcmp0(props->direction, "output") == 0)
		g_string_append_printf(attributes, ",%s", props->drive);

	attributes = g_string_append(attributes, "]");

	if (chip_obj) {
		chip = gpiodbus_object_peek_chip(chip_obj);
		g_print("%s ", gpiodbus_chip_get_name(chip));
	} else {
		g_print("\tline ");
	}

	g_print("%3u:\t%s\t\t%s\n", props->offset, line_name, attributes->str);
}

static void print_line_info(gpointer elem, gpointer user_data G_GNUC_UNUSED)
{
	GpiodbusObject *line_obj = elem;

	do_print_line_info(line_obj, NULL);
}

static void do_show_chip(GpiodbusObject *chip_obj)
{
	GpiodbusChip *chip = gpiodbus_object_peek_chip(chip_obj);
	g_autolist(GpiodbusObject) line_objs = NULL;

	g_print("%s - %u lines:\n",
		gpiodbus_chip_get_name(chip),
		gpiodbus_chip_get_num_lines(chip));

	line_objs = get_all_line_objs_for_chip(chip_obj);
	g_list_foreach(line_objs, print_line_info, NULL);
}

static void show_chip(gpointer elem, gpointer user_data G_GNUC_UNUSED)
{
	GpiodbusObject *chip_obj = elem;

	do_show_chip(chip_obj);
}

static void show_line_with_chip(gpointer elem, gpointer user_data)
{
	g_autoptr(GpiodbusObject) line_obj = NULL;
	GpiodbusObject *chip_obj = user_data;
	g_autofree gchar *chip_name = NULL;
	GString *line_name = elem;

	line_obj = get_line_obj_by_name_for_chip(chip_obj, line_name->str);
	if (!line_obj) {
		chip_name = g_path_get_basename(
			g_dbus_object_get_object_path(G_DBUS_OBJECT(chip_obj)));
		die("no line '%s' on chip '%s'", line_name->str, chip_name);
	}

	do_print_line_info(line_obj, chip_obj);
}

static void show_line(gpointer elem, gpointer user_data G_GNUC_UNUSED)
{
	g_autoptr(GpiodbusObject) line_obj = NULL;
	g_autoptr(GpiodbusObject) chip_obj = NULL;
	GString *line_name = elem;
	gboolean ret;

	ret = get_line_obj_by_name(line_name->str, &line_obj, &chip_obj);
	if (!ret)
		die("line '%s' not found", line_name->str);

	do_print_line_info(line_obj, chip_obj);
}

int gpiocli_info_main(int argc, char **argv)
{
	static const gchar *const summary =
"Print information about GPIO lines.";

	static const gchar *const description =
"Lines are specified by name, or optionally by offset if the chip option\n"
"is provided.\n";

	g_autolist(GpiodbusObject) chip_objs = NULL;
	g_autolist(GString) line_name_list = NULL;
	g_autoptr(GpiodbusObject) chip_obj = NULL;
	g_auto(GStrv) line_names = NULL;
	const gchar *chip_name = NULL;

	const GOptionEntry opts[] = {
		{
			.long_name		= "chip",
			.short_name		= 'c',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING,
			.arg_data		= &chip_name,
			.description		= "restrict scope to a particular chip",
			.arg_description	= "<chip>",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &line_names,
			.arg_description	= "[line1] [line2] ...",
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);
	check_manager();

	if (chip_name)
		chip_obj = get_chip_obj(chip_name);

	if (line_names) {
		line_name_list = strv_to_gstring_list(line_names);
		if (chip_obj)
			g_list_foreach(line_name_list, show_line_with_chip,
				       chip_obj);
		else
			g_list_foreach(line_name_list, show_line, NULL);
	} else if (chip_obj) {
		do_show_chip(chip_obj);
	} else {
		chip_objs = get_chip_objs(NULL);
		g_list_foreach(chip_objs, show_chip, NULL);
	}

	return EXIT_SUCCESS;
}
