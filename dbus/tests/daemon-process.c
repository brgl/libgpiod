// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <signal.h>

#include "daemon-process.h"

struct _GpiodbusDaemonProcess {
	GObject parent_instance;
	GSubprocess *proc;
};

G_DEFINE_TYPE(GpiodbusDaemonProcess, gpiodbus_daemon_process, G_TYPE_OBJECT);

static gboolean on_timeout(gpointer data G_GNUC_UNUSED)
{
	g_error("timeout reached waiting for the daemon name to appear on the system bus");

	return G_SOURCE_REMOVE;
}

static void on_name_appeared(GDBusConnection *con G_GNUC_UNUSED,
			     const gchar *name G_GNUC_UNUSED,
			     const gchar *name_owner G_GNUC_UNUSED,
			     gpointer data)
{
	gboolean *name_state = data;

	*name_state = TRUE;
}

static void gpiodbus_daemon_process_constructed(GObject *obj)
{
	GpiodbusDaemonProcess *self = GPIODBUS_DAEMON_PROCESS_OBJ(obj);
	const gchar *path = g_getenv("GPIODBUS_TEST_DAEMON_PATH");
	g_autoptr(GDBusConnection) con = NULL;
	g_autofree gchar *addr = NULL;
	g_autoptr(GError) err = NULL;
	gboolean name_state = FALSE;
	guint watch_id, timeout_id;

	if (!path)
		g_error("GPIODBUS_TEST_DAEMON_PATH environment variable must be set");

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!addr)
		g_error("failed to get an address for system bus: %s",
			err->message);

	con = g_dbus_connection_new_for_address_sync(addr,
			G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
			G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
			NULL, NULL, &err);
	if (!con)
		g_error("failed to get a dbus connection: %s", err->message);

	watch_id = g_bus_watch_name_on_connection(con, "io.gpiod1",
						  G_BUS_NAME_WATCHER_FLAGS_NONE,
						  on_name_appeared, NULL,
						  &name_state, NULL);

	self->proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_SILENCE |
				      G_SUBPROCESS_FLAGS_STDERR_SILENCE,
				      &err, path, NULL);
	if (!self->proc)
		g_error("failed to launch the gpio-manager process: %s",
			err->message);

	timeout_id = g_timeout_add_seconds(5, on_timeout, NULL);

	while (!name_state)
		g_main_context_iteration(NULL, TRUE);

	g_bus_unwatch_name(watch_id);
	g_source_remove(timeout_id);

	G_OBJECT_CLASS(gpiodbus_daemon_process_parent_class)->constructed(obj);
}

static void gpiodbus_daemon_process_kill(GSubprocess *proc)
{
	g_autoptr(GError) err = NULL;
	gint status;

	g_subprocess_send_signal(proc, SIGTERM);
	g_subprocess_wait(proc, NULL, &err);
	if (err)
		g_error("failed to collect the exit status of gpio-manager: %s",
			err->message);

	if (!g_subprocess_get_if_exited(proc))
		g_error("dbus-manager process did not exit normally");

	status = g_subprocess_get_exit_status(proc);
	if (status != 0)
		g_error("dbus-manager process exited with a non-zero status: %d",
			status);

	g_object_unref(proc);
}

static void gpiodbus_daemon_process_dispose(GObject *obj)
{
	GpiodbusDaemonProcess *self = GPIODBUS_DAEMON_PROCESS_OBJ(obj);

	g_clear_pointer(&self->proc, gpiodbus_daemon_process_kill);

	G_OBJECT_CLASS(gpiodbus_daemon_process_parent_class)->dispose(obj);
}

static void
gpiodbus_daemon_process_class_init(GpiodbusDaemonProcessClass *proc_class)
{
	GObjectClass *class = G_OBJECT_CLASS(proc_class);

	class->constructed = gpiodbus_daemon_process_constructed;
	class->dispose = gpiodbus_daemon_process_dispose;
}

static void gpiodbus_daemon_process_init(GpiodbusDaemonProcess *self)
{
	self->proc = NULL;
}

GpiodbusDaemonProcess *gpiodbus_daemon_process_new(void)
{
	return g_object_new(GPIODBUS_DAEMON_PROCESS_TYPE, NULL);
}
