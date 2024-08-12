// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <glib.h>
#include <glib-unix.h>
#include <gpiod-glib.h>
#include <stdlib.h>

#include "daemon.h"

static const gchar *const debug_domains[] = {
	"gpio-manager",
	"gpiodglib",
	NULL
};

static gboolean stop_main_loop_on_sig(gpointer data, const gchar *signame)
{
	GMainLoop *loop = data;

	g_debug("%s received", signame);

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

static gboolean on_sigterm(gpointer data)
{
	return stop_main_loop_on_sig(data, "SIGTERM");
}

static gboolean on_sigint(gpointer data)
{
	return stop_main_loop_on_sig(data, "SIGINT");
}

static gboolean on_sighup(gpointer data G_GNUC_UNUSED)
{
	g_debug("SIGHUB received, ignoring");

	return G_SOURCE_CONTINUE;
}

static void on_bus_acquired(GDBusConnection *con,
			    const gchar *name G_GNUC_UNUSED,
			    gpointer data)
{
	GpiodbusDaemon *daemon = data;

	g_debug("D-Bus connection acquired");

	gpiodbus_daemon_start(daemon, con);
}

static void on_name_acquired(GDBusConnection *con G_GNUC_UNUSED,
			     const gchar *name, gpointer data G_GNUC_UNUSED)
{
	g_debug("D-Bus name acquired: '%s'", name);
}

static void on_name_lost(GDBusConnection *con,
			 const gchar *name, gpointer data G_GNUC_UNUSED)
{
	g_debug("D-Bus name lost: '%s'", name);

	if (!con)
		g_error("unable to make connection to the bus");

	if (g_dbus_connection_is_closed(con))
		g_error("connection to the bus closed");

	g_error("name '%s' lost on the bus", name);
}

static void print_version_and_exit(void)
{
	g_print("%s (libgpiod) v%s\n", g_get_prgname(), gpiodglib_api_version());

	exit(EXIT_SUCCESS);
}

static void parse_opts(int argc, char **argv)
{
	gboolean ret, opt_debug = FALSE, opt_version = FALSE;
	g_autoptr(GOptionContext) ctx = NULL;
	g_auto(GStrv) remaining = NULL;
	g_autoptr(GError) err = NULL;

	const GOptionEntry opts[] = {
		{
			.long_name		= "debug",
			.short_name		= 'd',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &opt_debug,
			.description		= "Emit additional debug log messages.",
		},
		{
			.long_name		= "version",
			.short_name		= 'v',
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &opt_version,
			.description		= "Print version and exit.",
		},
		{
			.long_name		= G_OPTION_REMAINING,
			.flags			= G_OPTION_FLAG_NONE,
			.arg			= G_OPTION_ARG_STRING_ARRAY,
			.arg_data		= &remaining,
		},
		{ }
	};

	ctx = g_option_context_new(NULL);
	g_option_context_set_summary(ctx, "D-Bus daemon managing GPIOs.");
	g_option_context_add_main_entries(ctx, opts, NULL);

	ret = g_option_context_parse(ctx, &argc, &argv, &err);
	if (!ret) {
		g_printerr("Option parsing failed: %s\n\nUse %s --help\n",
			   err->message, g_get_prgname());
		exit(EXIT_FAILURE);
	}

	if (remaining) {
		g_printerr("Option parsing failed: additional arguments are not allowed\n");
		exit(EXIT_FAILURE);
	}

	if (opt_version)
		print_version_and_exit();

	if (opt_debug)
		g_log_writer_default_set_debug_domains(debug_domains);
}

int main(int argc, char **argv)
{
	g_autoptr(GpiodbusDaemon) daemon = NULL;
	g_autofree gchar *basename = NULL;
	g_autoptr(GMainLoop) loop = NULL;
	guint bus_id;

	basename = g_path_get_basename(argv[0]);
	g_set_prgname(basename);
	parse_opts(argc, argv);

	g_message("initializing %s", g_get_prgname());

	loop = g_main_loop_new(NULL, FALSE);
	daemon = gpiodbus_daemon_new();

	g_unix_signal_add(SIGTERM, on_sigterm, loop);
	g_unix_signal_add(SIGINT, on_sigint, loop);
	g_unix_signal_add(SIGHUP, on_sighup, NULL); /* Ignore SIGHUP. */

	bus_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "io.gpiod1",
				G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired,
				on_name_acquired, on_name_lost, daemon, NULL);

	g_message("%s started", g_get_prgname());

	g_main_loop_run(loop);

	g_bus_unown_name(bus_id);

	g_message("%s exiting", g_get_prgname());

	return EXIT_SUCCESS;
}
