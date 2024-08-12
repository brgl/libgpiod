/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_CHIP_INFO_H__
#define __GPIODGLIB_CHIP_INFO_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibChipInfo, gpiodglib_chip_info,
		     GPIODGLIB, CHIP_INFO, GObject);

#define GPIODGLIB_CHIP_INFO_TYPE (gpiodglib_chip_info_get_type())
#define GPIODGLIB_CHIP_INFO_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_CHIP_INFO_TYPE, \
				    GpiodglibChipInfo))

/**
 * gpiodglib_chip_info_dup_name:
 * @self: #GpiodglibChipInfo to manipulate.
 *
 * Get the name of the chip as represented in the kernel.
 *
 * Returns: (transfer full): Valid pointer to a human-readable string
 * containing the chip name. The returned string is a copy and must be freed by
 * the caller using g_free().
 */
gchar * G_GNUC_WARN_UNUSED_RESULT
gpiodglib_chip_info_dup_name(GpiodglibChipInfo *self);

/**
 * gpiodglib_chip_info_dup_label:
 * @self: #GpiodglibChipInfo to manipulate.
 *
 * Get the label of the chip as represented in the kernel.
 *
 * Returns: (transfer full): Valid pointer to a human-readable string
 * containing the chip label. The returned string is a copy and must be freed
 * by the caller using g_free().
 */
gchar * G_GNUC_WARN_UNUSED_RESULT
gpiodglib_chip_info_dup_label(GpiodglibChipInfo *self);

/**
 * gpiodglib_chip_info_get_num_lines:
 * @self: #GpiodglibChipInfo to manipulate.
 *
 * Get the number of lines exposed by the chip.
 *
 * Returns: Number of GPIO lines.
 */
guint gpiodglib_chip_info_get_num_lines(GpiodglibChipInfo *self);

G_END_DECLS

#endif /* __GPIODGLIB_CHIP_INFO_H__ */
