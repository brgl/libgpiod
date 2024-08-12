// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <stdlib.h>

#include "common.h"

typedef struct {
	gboolean name_done;
	gboolean chip_done;
	const gchar *label;
} WaitData;

static void obj_match_label(GpiodbusObject *chip_obj, WaitData *data)
{
	GpiodbusChip *chip = gpiodbus_object_peek_chip(chip_obj);

	if (g_strcmp0(gpiodbus_chip_get_label(chip), data->label) == 0)
		data->chip_done = TRUE;
}

static void check_label(gpointer elem, gpointer user_data)
{
	WaitData *data = user_data;
	GpiodbusObject *obj = elem;

	obj_match_label(obj, data);
}

static void on_object_added(GDBusObjectManager *manager G_GNUC_UNUSED,
			    GpiodbusObject *obj, gpointer user_data)
{
	WaitData *data = user_data;

	obj_match_label(GPIODBUS_OBJECT(obj), data);
}

static void wait_for_chip(WaitData *data)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autolist(GpiodbusObject) objs = NULL;

	manager = get_object_manager_client("/io/gpiod1/chips");

	g_signal_connect(manager, "object-added",
			 G_CALLBACK(on_object_added), data);

	objs = g_dbus_object_manager_get_objects(manager);
	g_list_foreach(objs, check_label, data);

	while (!data->chip_done)
		g_main_context_iteration(NULL, TRUE);
}

static void on_name_appeared(GDBusConnection *con G_GNUC_UNUSED,
			     const gchar *name G_GNUC_UNUSED,
			     const gchar *name_owner G_GNUC_UNUSED,
			     gpointer user_data)
{
	WaitData *data = user_data;

	data->name_done = TRUE;
}

static void on_name_vanished(GDBusConnection *con G_GNUC_UNUSED,
			     const gchar *name G_GNUC_UNUSED,
			     gpointer user_data)
{
	WaitData *data = user_data;

	if (data->label && data->chip_done)
		die("gpio-manager vanished while waiting for chip");
}

static gboolean on_timeout(gpointer user_data G_GNUC_UNUSED)
{
	die("wait timed out!");
}

static guint schedule_timeout(const gchar *timeout)
{
	gint64 period, multiplier = 0;
	gchar *end;

	period = g_ascii_strtoll(timeout, &end, 10);

	switch (*end) {
	case 'm':
		multiplier = 1;
		end++;
		break;
	case 's':
		multiplier = 1000;
		break;
	case '\0':
		break;
	default:
		goto invalid_timeout;
	}

	if (multiplier) {
		if (*end != 's')
			goto invalid_timeout;

		end++;
	} else {
		/* Default to miliseconds. */
		multiplier = 1;
	}

	period *= multiplier;
	if (period > G_MAXUINT)
		die("timeout must not exceed %u miliseconds\n", G_MAXUINT);

	return g_timeout_add(period, on_timeout, NULL);

invalid_timeout:
	die("invalid timeout value: %s", timeout);
}

int gpiocli_wait_main(int argc, char **argv)
{
	static const gchar *const summary =
"Wait for the gpio-manager interface to appear.";

	static const gchar *const description =
"Timeout period defaults to miliseconds but can be given in seconds or miliseconds\n"
"explicitly .e.g: --timeout=1000, --timeout=1000ms and --timeout=1s all specify\n"
"the same period.";

	const gchar *timeout_str = NULL;
	guint watch_id, timeout_id = 0;
	g_auto(GStrv) remaining = NULL;
	WaitData data = {};

	const GOptionEntry opts[] = {
		{
			.long_name		= "chip",
			.short_name		= 'c',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING,
			.arg_data		= &data.label,
			.description		= "Wait for a specific chip to appear.",
			.arg_description	= "<label>",
		},
		{
			.long_name		= "timeout",
			.short_name		= 't',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING,
			.arg_data		= &timeout_str,
			.description		= "Bail-out if timeout expires.",
			.arg_description	= "<timeout_str>",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &remaining,
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);

	if (remaining)
		die_parsing_opts("command doesn't take additional arguments");

	watch_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM, "io.gpiod1",
				    G_BUS_NAME_WATCHER_FLAGS_NONE,
				    on_name_appeared, on_name_vanished,
				    &data, NULL);

	if (timeout_str)
		timeout_id = schedule_timeout(timeout_str);

	while (!data.name_done)
		g_main_context_iteration(NULL, TRUE);

	if (data.label)
		wait_for_chip(&data);

	g_bus_unwatch_name(watch_id);
	if (timeout_str)
		g_source_remove(timeout_id);

	return EXIT_SUCCESS;
}
