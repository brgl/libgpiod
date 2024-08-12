/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_LINE_CONFIG_H__
#define __GPIODGLIB_LINE_CONFIG_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "line-settings.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibLineConfig, gpiodglib_line_config,
		     GPIODGLIB, LINE_CONFIG, GObject);

#define GPIODGLIB_LINE_CONFIG_TYPE (gpiodglib_line_config_get_type())
#define GPIODGLIB_LINE_CONFIG_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_LINE_CONFIG_TYPE, \
				    GpiodglibLineConfig))

/**
 * gpiodglib_line_config_new:
 *
 * Create a new #GpiodglibLineConfig.
 *
 * Returns: (transfer full): Empty #GpiodglibLineConfig.
 */
GpiodglibLineConfig *gpiodglib_line_config_new(void);

/**
 * gpiodglib_line_config_reset:
 * @self: #GpiodglibLineConfig to manipulate.
 *
 * Reset the line config object.
 */
void gpiodglib_line_config_reset(GpiodglibLineConfig *self);

/**
 * gpiodglib_line_config_add_line_settings:
 * @self: #GpiodglibLineConfig to manipulate.
 * @offsets: (element-type GArray): GArray of offsets for which to apply the
 * settings.
 * @settings: #GpiodglibLineSettings to apply.
 * @err: Return location for error or NULL.
 *
 * Add line settings for a set of offsets.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean
gpiodglib_line_config_add_line_settings(GpiodglibLineConfig *self,
					const GArray *offsets,
					GpiodglibLineSettings *settings,
					GError **err);

/**
 * gpiodglib_line_config_get_line_settings:
 * @self: #GpiodglibLineConfig to manipulate.
 * @offset: Offset for which to get line settings.
 *
 * Get line settings for offset.
 *
 * Returns: (transfer full): New reference to a #GpiodglibLineSettings.
 */
GpiodglibLineSettings *
gpiodglib_line_config_get_line_settings(GpiodglibLineConfig *self,
					guint offset);

/**
 * gpiodglib_line_config_set_output_values:
 * @self: #GpiodglibLineConfig to manipulate.
 * @values: (element-type GArray): GArray containing the output values.
 * @err: Return location for error or NULL.
 *
 * @brief Set output values for a number of lines.
 *
 * Returns: TRUE on success, FALSE on error.
 */
gboolean gpiodglib_line_config_set_output_values(GpiodglibLineConfig *self,
						 const GArray *values,
						 GError **err);

/**
 * gpiodglib_line_config_get_configured_offsets:
 * @self: #GpiodglibLineConfig to manipulate.
 *
 * Get configured offsets.
 *
 * Returns: (transfer full) (element-type GArray): GArray containing the
 * offsets for which configuration has been set.
 */
GArray *gpiodglib_line_config_get_configured_offsets(GpiodglibLineConfig *self);

G_END_DECLS

#endif /* __GPIODGLIB_LINE_CONFIG_H__ */
