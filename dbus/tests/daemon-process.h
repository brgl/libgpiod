/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODBUS_TEST_DAEMON_PROCESS_H__
#define __GPIODBUS_TEST_DAEMON_PROCESS_H__

#include <glib.h>

G_DECLARE_FINAL_TYPE(GpiodbusDaemonProcess, gpiodbus_daemon_process,
		     GPIODBUS, DAEMON_PROCESS, GObject);

#define GPIODBUS_DAEMON_PROCESS_TYPE (gpiodbus_daemon_process_get_type())
#define GPIODBUS_DAEMON_PROCESS_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST(obj, \
	 GPIODBUS_DAEMON_PROCESS_TYPE, \
	 GpiodbusDaemonProcess))

GpiodbusDaemonProcess *gpiodbus_daemon_process_new(void);

#endif /* __GPIODBUS_TEST_DAEMON_PROCESS_H__ */
