// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

static void show_request(gpointer elem, gpointer user_data G_GNUC_UNUSED)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autofree gchar *request_name = NULL;
	g_autofree gchar *offsets_str = NULL;
	g_autoptr(GVariant) voffsets = NULL;
	g_autofree gchar *chip_name = NULL;
	g_autoptr(GArray) offsets = NULL;
	GpiodbusObject *obj = elem;
	GpiodbusRequest *request;
	GVariantBuilder builder;
	const gchar *chip_path;
	gsize i;

	request_name = g_path_get_basename(
			g_dbus_object_get_object_path(G_DBUS_OBJECT(obj)));
	request = gpiodbus_object_peek_request(obj);
	chip_path = gpiodbus_request_get_chip_path(request);
	manager = get_object_manager_client(chip_path);
	/* FIXME: Use chip proxy? */
	chip_name = g_path_get_basename(chip_path);

	offsets = get_request_offsets(request);
	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	for (i = 0; i < offsets->len; i++)
		g_variant_builder_add(&builder, "u",
				      g_array_index(offsets, guint, i));
	voffsets = g_variant_ref_sink(g_variant_builder_end(&builder));
	offsets_str = g_variant_print(voffsets, FALSE);

	g_print("%s (%s) Offsets: %s\n",
		request_name, chip_name, offsets_str);
}

int gpiocli_requests_main(int argc, char **argv)
{
	static const gchar *const summary =
"List all line requests controlled by the manager.";

	g_autolist(GpiodbusObject) request_objs = NULL;
	g_auto(GStrv) remaining = NULL;

	const GOptionEntry opts[] = {
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &remaining,
			.arg_description	= NULL,
		},
		{ }
	};

	parse_options(opts, summary, NULL, &argc, &argv);
	check_manager();

	if (remaining)
		die_parsing_opts("command doesn't take additional arguments");

	request_objs = get_request_objs();
	g_list_foreach(request_objs, show_request, NULL);

	return EXIT_SUCCESS;
}
