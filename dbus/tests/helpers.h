/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODBUS_TEST_INTERNAL_H__
#define __GPIODBUS_TEST_INTERNAL_H__

#include <gio/gio.h>
#include <glib.h>
#include <gpiodbus.h>
#include <gpiosim-glib.h>

#define __gpiodbus_test_check_gboolean_and_error(_ret, _err) \
	do { \
		g_assert_true(_ret); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
	} while (0)

#define __gpiodbus_test_check_nonnull_and_error(_ptr, _err) \
	do { \
		g_assert_nonnull(_ptr); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
	} while (0)

#define gpiodbus_test_get_chip_proxy_or_fail(_obj_path) \
	({ \
		g_autoptr(GDBusConnection) _con = NULL; \
		g_autoptr(GError) _err = NULL; \
		g_autoptr(GpiodbusChip) _chip = NULL; \
		_con = gpiodbus_test_get_dbus_connection(); \
		_chip = gpiodbus_chip_proxy_new_sync(_con, \
						     G_DBUS_PROXY_FLAGS_NONE, \
						     "io.gpiod1", _obj_path, \
						     NULL, &_err); \
		__gpiodbus_test_check_nonnull_and_error(_chip, _err); \
		g_object_ref(_chip); \
	})

#define gpiodbus_test_get_line_proxy_or_fail(_obj_path) \
	({ \
		g_autoptr(GDBusConnection) _con = NULL; \
		g_autoptr(GError) _err = NULL; \
		g_autoptr(GpiodbusLine) _line = NULL; \
		_con = gpiodbus_test_get_dbus_connection(); \
		_line = gpiodbus_line_proxy_new_sync(_con, \
						     G_DBUS_PROXY_FLAGS_NONE, \
						     "io.gpiod1", _obj_path, \
						     NULL, &_err); \
		__gpiodbus_test_check_nonnull_and_error(_line, _err); \
		g_object_ref(_line); \
	})

#define gpiodbus_test_get_request_proxy_or_fail(_obj_path) \
	({ \
		g_autoptr(GDBusConnection) _con = NULL; \
		g_autoptr(GError) _err = NULL; \
		g_autoptr(GpiodbusRequest) _req = NULL; \
		_con = gpiodbus_test_get_dbus_connection(); \
		_req = gpiodbus_request_proxy_new_sync(_con, \
						G_DBUS_PROXY_FLAGS_NONE, \
						"io.gpiod1", _obj_path, \
						NULL, &_err); \
		__gpiodbus_test_check_nonnull_and_error(_req, _err); \
		g_object_ref(_req); \
	})

#define gpiodbus_test_get_chip_object_manager_or_fail() \
	({ \
		g_autoptr(GDBusObjectManager) _manager = NULL; \
		g_autoptr(GDBusConnection) _con = NULL; \
		g_autoptr(GError) _err = NULL; \
		_con = gpiodbus_test_get_dbus_connection(); \
		_manager = gpiodbus_object_manager_client_new_sync( \
				_con, \
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, \
				"io.gpiod1", "/io/gpiod1/chips", NULL, \
				&_err); \
		__gpiodbus_test_check_nonnull_and_error(_manager, _err); \
		g_object_ref(_manager); \
	})

#define gpiodbus_test_chip_call_request_lines_sync_or_fail(_chip, \
							   _line_config, \
							   _request_config, \
							   _request_path) \
	do { \
		g_autoptr(GError) _err = NULL; \
		gboolean _ret; \
		_ret = gpiodbus_chip_call_request_lines_sync( \
						_chip, _line_config, \
						_request_config, \
						G_DBUS_CALL_FLAGS_NONE, -1, \
						_request_path, NULL, &_err); \
		__gpiodbus_test_check_gboolean_and_error(_ret, _err); \
	} while (0)

#define gpiodbus_test_request_call_release_sync_or_fail(_request) \
	do { \
		g_autoptr(GError) _err = NULL; \
		gboolean _ret; \
		_ret = gpiodbus_request_call_release_sync( \
						_request, \
						G_DBUS_CALL_FLAGS_NONE, \
						-1, NULL, &_err); \
		__gpiodbus_test_check_gboolean_and_error(_ret, _err); \
	} while (0)

GDBusConnection *gpiodbus_test_get_dbus_connection(void);
void gpiodbus_test_wait_for_sim_intf(GPIOSimChip *sim);
GVariant *gpiodbus_test_make_empty_request_config(void);

#endif /* __GPIODBUS_TEST_INTERNAL_H__ */
