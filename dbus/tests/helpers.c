// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "helpers.h"

GDBusConnection *gpiodbus_test_get_dbus_connection(void)
{
	g_autoptr(GDBusConnection) con = NULL;
	g_autofree gchar *addr = NULL;
	g_autoptr(GError) err = NULL;

	addr = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (!addr)
		g_error("Failed to get address on the bus: %s", err->message);

	con = g_dbus_connection_new_for_address_sync(addr,
		G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
		G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
		NULL, NULL, &err);
	if (!con)
		g_error("Failed to get system bus connection: %s",
			err->message);

	return g_object_ref(con);
}

typedef struct {
	gboolean *added;
	gchar *obj_path;
} OnObjectAddedData;

static void on_object_added(GDBusObjectManager *manager G_GNUC_UNUSED,
			    GpiodbusObject *object, gpointer data)
{
	OnObjectAddedData *cb_data = data;
	const gchar *path;

	path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

	if (g_strcmp0(path, cb_data->obj_path) == 0)
		*cb_data->added = TRUE;
}

static gboolean on_timeout(gpointer data G_GNUC_UNUSED)
{
	g_error("timeout reached waiting for the gpiochip interface to appear on the bus");

	return G_SOURCE_REMOVE;
}

void gpiodbus_test_wait_for_sim_intf(GPIOSimChip *sim)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GDBusConnection) con = NULL;
	g_autoptr(GpiodbusObject) obj = NULL;
	g_autoptr(GError) err = NULL;
	g_autofree gchar *obj_path;
	OnObjectAddedData cb_data;
	gboolean added = FALSE;
	guint timeout_id;

	con = gpiodbus_test_get_dbus_connection();
	if (!con)
		g_error("failed to obtain a bus connection: %s", err->message);

	obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				   g_gpiosim_chip_get_name(sim));

	cb_data.added = &added;
	cb_data.obj_path = obj_path;

	manager = gpiodbus_object_manager_client_new_sync(con,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"io.gpiod1", "/io/gpiod1/chips", NULL, &err);
	if (!manager)
		g_error("failed to create the object manager client: %s",
			err->message);

	g_signal_connect(manager, "object-added", G_CALLBACK(on_object_added),
			 &cb_data);

	obj = GPIODBUS_OBJECT(g_dbus_object_manager_get_object(manager,
							       obj_path));
	if (obj) {
		if (g_strcmp0(g_dbus_object_get_object_path(G_DBUS_OBJECT(obj)),
			      obj_path) == 0)
			added = TRUE;
	}

	timeout_id = g_timeout_add_seconds(5, on_timeout, NULL);

	while (!added)
		g_main_context_iteration(NULL, TRUE);

	g_source_remove(timeout_id);
}

GVariant *gpiodbus_test_make_empty_request_config(void)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));

	return g_variant_ref_sink(g_variant_builder_end(&builder));
}
