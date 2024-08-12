// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiodbus.h>
#include <gpiosim-glib.h>

#include "daemon-process.h"
#include "helpers.h"

#define GPIOD_TEST_GROUP "gpiodbus/chip"

GPIOD_TEST_CASE(read_chip_info)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8,
							"label", "foobar",
							NULL);
	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autoptr(GpiodbusChip) chip = NULL;
	g_autofree gchar *obj_path = NULL;

	mgr = gpiodbus_daemon_process_new();
	gpiodbus_test_wait_for_sim_intf(sim);

	obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				   g_gpiosim_chip_get_name(sim));
	chip = gpiodbus_test_get_chip_proxy_or_fail(obj_path);

	g_assert_cmpstr(gpiodbus_chip_get_name(chip), ==,
			g_gpiosim_chip_get_name(sim));
	g_assert_cmpstr(gpiodbus_chip_get_label(chip), ==, "foobar");
	g_assert_cmpuint(gpiodbus_chip_get_num_lines(chip), ==, 8);
	g_assert_cmpstr(gpiodbus_chip_get_path(chip), ==,
			g_gpiosim_chip_get_dev_path(sim));
}

static gboolean on_timeout(gpointer user_data)
{
	gboolean *timed_out = user_data;

	*timed_out = TRUE;

	return G_SOURCE_REMOVE;
}

static void on_object_event(GDBusObjectManager *manager G_GNUC_UNUSED,
			    GpiodbusObject *object, gpointer user_data)
{
	gchar **obj_path = user_data;

	*obj_path = g_strdup(g_dbus_object_get_object_path(
						G_DBUS_OBJECT(object)));
}

GPIOD_TEST_CASE(chip_added)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autofree gchar *sim_obj_path = NULL;
	g_autoptr(GPIOSimChip) sim = NULL;
	g_autofree gchar *obj_path = NULL;
	gboolean timed_out = FALSE;
	guint timeout_id;

	mgr = gpiodbus_daemon_process_new();

	manager = gpiodbus_test_get_chip_object_manager_or_fail();

	g_signal_connect(manager, "object-added", G_CALLBACK(on_object_event),
			 &obj_path);
	timeout_id = g_timeout_add_seconds(5, on_timeout, &timed_out);

	sim = g_gpiosim_chip_new(NULL);

	while (!obj_path && !timed_out)
		g_main_context_iteration(NULL, TRUE);

	if (timed_out) {
		g_test_fail_printf("timeout reached waiting for chip to be added");
		return;
	}

	sim_obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				       g_gpiosim_chip_get_name(sim));

	g_assert_cmpstr(sim_obj_path, ==, obj_path);

	g_source_remove(timeout_id);
}

GPIOD_TEST_CASE(chip_removed)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GpiodbusDaemonProcess) mgr = NULL;
	g_autofree gchar *sim_obj_path = NULL;
	g_autoptr(GpiodbusChip) chip = NULL;
	g_autofree gchar *obj_path = NULL;
	gboolean timed_out = FALSE;
	guint timeout_id;

	sim_obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				       g_gpiosim_chip_get_name(sim));

	mgr = gpiodbus_daemon_process_new();
	gpiodbus_test_wait_for_sim_intf(sim);

	obj_path = g_strdup_printf("/io/gpiod1/chips/%s",
				   g_gpiosim_chip_get_name(sim));
	chip = gpiodbus_test_get_chip_proxy_or_fail(obj_path);
	manager = gpiodbus_test_get_chip_object_manager_or_fail();

	g_signal_connect(manager, "object-removed", G_CALLBACK(on_object_event),
			 &obj_path);
	timeout_id = g_timeout_add_seconds(5, on_timeout, &timed_out);

	g_clear_object(&sim);

	while (!obj_path && !timed_out)
		g_main_context_iteration(NULL, TRUE);

	if (timed_out) {
		g_test_fail_printf("timeout reached waiting for chip to be removed");
		return;
	}

	g_assert_cmpstr(sim_obj_path, ==, obj_path);

	g_source_remove(timeout_id);
}
