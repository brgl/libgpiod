// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

typedef struct {
	GList *lines;
} MonitorData;

static void on_edge_event(GpiodbusLine *line, GVariant *args,
			  gpointer user_data G_GNUC_UNUSED)
{
	const char *name = gpiodbus_line_get_name(line);
	gulong global_seqno, line_seqno;
	guint64 timestamp;
	gint edge;

	g_variant_get(args, "(ittt)", &edge, &timestamp,
		      &global_seqno, &line_seqno);

	g_print("%lu %s ", timestamp, edge ? "rising " : "falling");
	if (strlen(name))
		g_print("\"%s\"\n", name);
	else
		g_print("%u\n", gpiodbus_line_get_offset(line));
}

static void connect_edge_event(gpointer elem, gpointer user_data)
{
	GpiodbusObject *line_obj = elem;
	MonitorData *data = user_data;
	g_autoptr(GError) err = NULL;
	const gchar *line_obj_path;
	GpiodbusLine *line;

	line_obj_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(line_obj));

	line = gpiodbus_line_proxy_new_for_bus_sync(G_BUS_TYPE_SYSTEM,
						    G_DBUS_PROXY_FLAGS_NONE,
						    "io.gpiod1", line_obj_path,
						    NULL, &err);
	if (err)
		die_gerror(err, "Failed to get D-Bus proxy for '%s'",
			   line_obj_path);

	if (!gpiodbus_line_get_managed(line))
		die("Line must be managed by gpio-manager in order to be monitored");

	if (g_strcmp0(gpiodbus_line_get_edge_detection(line), "none") == 0)
		die("Edge detection must be enabled for monitored lines");

	data->lines = g_list_append(data->lines, line);

	g_signal_connect(line, "edge-event", G_CALLBACK(on_edge_event), NULL);
}

int gpiocli_monitor_main(int argc, char **argv)
{
	static const gchar *const summary =
"Get values of one or more GPIO lines.";

	static const gchar *const description =
"If -r/--request is specified then all the lines must belong to the same\n"
"request (and - by extension - the same chip).\n"
"\n"
"If no lines are specified but -r/--request was passed then all lines within\n"
"the request will be used.";

	g_autoptr(GDBusObjectManager) manager = NULL;
	const gchar *request_name = NULL, *chip_path;
	g_autolist(GpiodbusObject) line_objs = NULL;
	g_autoptr(GpiodbusObject) chip_obj = NULL;
	g_autoptr(GpiodbusObject) req_obj = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GMainLoop) loop = NULL;
	g_auto(GStrv) lines = NULL;
	GpiodbusRequest *request;
	MonitorData data = { };
	gsize num_lines, i;
	guint watch_id;
	gboolean ret;

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
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &lines,
			.arg_description	= "[line0] [line1]...",
		},
		{ }
	};

	parse_options(opts, summary, description, &argc, &argv);

	watch_id = g_bus_watch_name(G_BUS_TYPE_SYSTEM, "io.gpiod1",
				    G_BUS_NAME_WATCHER_FLAGS_NONE,
				    NULL, die_on_name_vanished, NULL, NULL);
	check_manager();

	if (!lines && !request_name)
		die_parsing_opts("either at least one line or the request must be specified");

	if (request_name) {
		req_obj = get_request_obj(request_name);
		request = gpiodbus_object_peek_request(req_obj);
		chip_path = gpiodbus_request_get_chip_path(request);
		chip_obj = get_chip_obj_by_path(chip_path);
		offsets = g_array_new(FALSE, TRUE, sizeof(guint));

		if (lines) {
			num_lines = g_strv_length(lines);

			for (i = 0; i < num_lines; i++) {
				g_autoptr(GpiodbusObject) line_obj = NULL;

				line_obj = get_line_obj_by_name_for_chip(
							chip_obj, lines[i]);
				if (!line_obj)
					die("Line not found: %s\n", lines[i]);

				line_objs = g_list_append(line_objs,
							g_object_ref(line_obj));
			}
		} else {
			offsets = get_request_offsets(request);
			manager = get_object_manager_client(chip_path);

			for (i = 0; i < offsets->len; i++) {
				g_autoptr(GpiodbusObject) line_obj = NULL;
				g_autofree char *obj_path = NULL;

				obj_path = g_strdup_printf("%s/line%u",
							   chip_path,
							   g_array_index(
								offsets,
								guint, i));

				line_obj = GPIODBUS_OBJECT(
					g_dbus_object_manager_get_object(
								manager,
								obj_path));
				if (!line_obj)
					die("Line not found: %u\n",
					    g_array_index(offsets, guint, i));

				line_objs = g_list_append(line_objs,
							g_object_ref(line_obj));
			}
		}
	} else {
		num_lines = g_strv_length(lines);

		for (i = 0; i < num_lines; i++) {
			g_autoptr(GpiodbusObject) line_obj = NULL;

			ret = get_line_obj_by_name(lines[i], &line_obj, NULL);
			if (!ret)
				die("Line not found: %s\n", lines[i]);

			line_objs = g_list_append(line_objs,
						  g_object_ref(line_obj));
		}
	}

	g_list_foreach(line_objs, connect_edge_event, &data);

	loop = g_main_loop_new(NULL, FALSE);
	g_unix_signal_add(SIGTERM, quit_main_loop_on_signal, loop);
	g_unix_signal_add(SIGINT, quit_main_loop_on_signal, loop);

	g_main_loop_run(loop);

	g_bus_unwatch_name(watch_id);

	return EXIT_SUCCESS;
}
