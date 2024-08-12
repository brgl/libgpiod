// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gpiod.h>
#include <gpiod-glib.h>

gboolean gpiodglib_is_gpiochip_device(const gchar *path)
{
	g_assert(path);

	return gpiod_is_gpiochip_device(path);
}

const gchar *gpiodglib_api_version(void)
{
	return gpiod_api_version();
}
