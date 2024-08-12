/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_MISC_H__
#define __GPIODGLIB_MISC_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * gpiodglib_is_gpiochip_device:
 * @path: Path to check.
 *
 * Check if the file pointed to by path is a GPIO chip character device.
 *
 * Returns: TRUE if the file exists and is either a GPIO chip character device
 * or a symbolic link to one, FALSE otherwise.
 */
gboolean gpiodglib_is_gpiochip_device(const gchar *path);

/**
 * gpiodglib_api_version:
 *
 * Get the API version of the library as a human-readable string.
 *
 * Returns: A valid pointer to a human-readable string containing the library
 * version. The pointer is valid for the lifetime of the program and must not
 * be freed by the caller.
 */
const gchar *gpiodglib_api_version(void);

G_END_DECLS

#endif /* __GPIODGLIB_MISC_H__ */
