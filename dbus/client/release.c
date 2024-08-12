// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

int gpiocli_release_main(int argc, char **argv)
{
	static const gchar *const summary =
"Release one of the line requests controlled by the manager.";

	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GpiodbusObject) obj = NULL;
	g_autofree gchar *obj_path = NULL;
	g_auto(GStrv) remaining = NULL;
	g_autoptr(GError) err = NULL;
	const gchar *request_name;
	GpiodbusRequest *request;
	gboolean ret;

	const GOptionEntry opts[] = {
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &remaining,
			.arg_description	= "<request>",
		},
		{ }
	};

	parse_options(opts, summary, NULL, &argc, &argv);

	if (!remaining || g_strv_length(remaining) != 1)
		die_parsing_opts("Exactly one request to release must be specified.");

	check_manager();

	request_name = remaining[0];

	obj_path = make_request_obj_path(request_name);
	manager = get_object_manager_client("/io/gpiod1/requests");
	obj = GPIODBUS_OBJECT(g_dbus_object_manager_get_object(manager,
							       obj_path));
	if (!obj)
		goto no_request;

	request = gpiodbus_object_peek_request(obj);
	if (!request)
		goto no_request;

	ret = gpiodbus_request_call_release_sync(request,
						 G_DBUS_CALL_FLAGS_NONE,
						 -1, NULL, &err);
	if (!ret)
		die_gerror(err, "Failed to release request '%s': %s",
			   request_name, err->message);

	return EXIT_SUCCESS;

no_request:
	die("No such request: '%s'", request_name);
}
