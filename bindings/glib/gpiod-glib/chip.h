/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_CHIP_H__
#define __GPIODGLIB_CHIP_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "chip-info.h"
#include "line-config.h"
#include "line-info.h"
#include "line-request.h"
#include "request-config.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibChip, gpiodglib_chip, GPIODGLIB, CHIP, GObject);

#define GPIODGLIB_CHIP_TYPE (gpiodglib_chip_get_type())
#define GPIODGLIB_CHIP_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_CHIP_TYPE, GpiodglibChip))

/**
 * gpiodglib_chip_new:
 * @path: Path to the device file to open.
 * @err: Return location for error or %NULL.
 *
 * Instantiates a new chip object by opening the device file indicated by path.
 *
 * Returns: (transfer full): New GPIO chip object.
 */
GpiodglibChip *gpiodglib_chip_new(const gchar *path, GError **err);

/**
 * gpiodglib_chip_close:
 * @self: #GpiodglibChip to close.
 *
 * Close the GPIO chip device file and free associated resources.
 *
 * The chip object can live after calling this method but any of the chip's
 * methods will result in an error being set.
 */
void gpiodglib_chip_close(GpiodglibChip *self);

/**
 * gpiodglib_chip_is_closed:
 * @self: #GpiodglibChip to manipulate.
 *
 * @brief Check if this object is valid.
 *
 * Returns: TRUE if this object's methods can be used, FALSE otherwise.
 */
gboolean gpiodglib_chip_is_closed(GpiodglibChip *self);

/**
 * gpiodglib_chip_dup_path:
 * @self: #GpiodglibChip to manipulate.
 *
 * Get the filesystem path that was used to open this GPIO chip.
 *
 * Returns: Path to the underlying character device file. The string is a copy
 * and must be freed by the caller with g_free().
 */
gchar * G_GNUC_WARN_UNUSED_RESULT
gpiodglib_chip_dup_path(GpiodglibChip *self);

/**
 * gpiodglib_chip_get_info:
 * @self: #GpiodglibChip to manipulate.
 * @err: Return location for error or %NULL.
 *
 * Get information about the chip.
 *
 * Returns: (transfer full): New #GpiodglibChipInfo.
 */
GpiodglibChipInfo *gpiodglib_chip_get_info(GpiodglibChip *self, GError **err);

/**
 * gpiodglib_chip_get_line_info:
 * @self: #GpiodglibChip to manipulate.
 * @offset: Offset of the line to get the info for.
 * @err: Return location for error or %NULL.
 *
 * Retrieve the current snapshot of line information for a single line.
 *
 * Returns: (transfer full): New #GpiodglibLineInfo.
 */
GpiodglibLineInfo *
gpiodglib_chip_get_line_info(GpiodglibChip *self, guint offset, GError **err);

/**
 * gpiodglib_chip_watch_line_info:
 * @self: #GpiodglibChip to manipulate.
 * @offset: Offset of the line to get the info for and to watch.
 * @err: Return location for error or %NULL.
 *
 * Retrieve the current snapshot of line information for a single line and
 * start watching this line for future changes.
 *
 * Returns: (transfer full): New #GpiodglibLineInfo.
 */
GpiodglibLineInfo *
gpiodglib_chip_watch_line_info(GpiodglibChip *self, guint offset, GError **err);

/**
 * gpiodglib_chip_unwatch_line_info:
 * @self: #GpiodglibChip to manipulate.
 * @offset: Offset of the line to get the info for.
 * @err: Return location for error or %NULL.
 *
 * Stop watching the line at given offset for info events.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean
gpiodglib_chip_unwatch_line_info(GpiodglibChip *self, guint offset,
				 GError **err);

/**
 * gpiodglib_chip_get_line_offset_from_name:
 * @self: #GpiodglibChip to manipulate.
 * @name: Name of the GPIO line to map.
 * @offset: Return location for the mapped offset.
 * @err: Return location for error or %NULL.
 *
 * Map a GPIO line's name to its offset within the chip.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean
gpiodglib_chip_get_line_offset_from_name(GpiodglibChip *self, const gchar *name,
					 guint *offset, GError **err);

/**
 * gpiodglib_chip_request_lines:
 * @self: #GpiodglibChip to manipulate.
 * @req_cfg: Request config object. Can be NULL for default settings.
 * @line_cfg: Line config object.
 * @err: Return location for error or %NULL.
 *
 * Request a set of lines for exclusive usage.
 *
 * Returns: (transfer full): New #GpiodglibLineRequest.
 */
GpiodglibLineRequest *
gpiodglib_chip_request_lines(GpiodglibChip *self,
			     GpiodglibRequestConfig *req_cfg,
			     GpiodglibLineConfig *line_cfg, GError **err);

G_END_DECLS

#endif /* __GPIODGLIB_CHIP_H__ */
