/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_LINE_REQUEST_H__
#define __GPIODGLIB_LINE_REQUEST_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibLineRequest, gpiodglib_line_request,
		     GPIODGLIB, LINE_REQUEST, GObject);

#define GPIODGLIB_LINE_REQUEST_TYPE (gpiodglib_line_request_get_type())
#define GPIODGLIB_LINE_REQUEST_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_LINE_REQUEST_TYPE, \
				    GpiodglibLineRequest))

/**
 * gpiodglib_line_request_release:
 * @self: #GpiodglibLineRequest to manipulate.
 *
 * Release the requested lines and free all associated resources.
 */
void gpiodglib_line_request_release(GpiodglibLineRequest *self);

/**
 * gpiodglib_line_request_is_released:
 * @self: #GpiodglibLineRequest to manipulate.
 *
 * Check if this request was released.
 *
 * Returns: TRUE if this request was released and is no longer valid, FALSE
 * otherwise.
 */
gboolean gpiodglib_line_request_is_released(GpiodglibLineRequest *self);

/**
 * gpiodglib_line_request_dup_chip_name:
 * @self: #GpiodglibLineRequest to manipulate.
 *
 * Get the name of the chip this request was made on.
 *
 * Returns: Name the GPIO chip device. The string is a copy and must be freed
 * by the caller using g_free().
 */
gchar * G_GNUC_WARN_UNUSED_RESULT
gpiodglib_line_request_dup_chip_name(GpiodglibLineRequest *self);

/**
 * gpiodglib_line_request_get_requested_offsets:
 * @self: #GpiodglibLineRequest to manipulate.
 *
 * Get the offsets of the lines in the request.
 *
 * Returns: (transfer full) (element-type GArray): Array containing the
 * requested offsets.
 */
GArray *
gpiodglib_line_request_get_requested_offsets(GpiodglibLineRequest *self);

/**
 * gpiodglib_line_request_reconfigure_lines:
 * @self: #GpiodglibLineRequest to manipulate.
 * @config: New line config to apply.
 * @err: Return location for error or NULL.
 *
 * Update the configuration of lines associated with a line request.
 *
 * The new line configuration completely replaces the old. Any requested lines
 * without overrides are configured to the requested defaults. Any configured
 * overrides for lines that have not been requested are silently ignored.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean gpiodglib_line_request_reconfigure_lines(GpiodglibLineRequest *self,
						  GpiodglibLineConfig *config,
						  GError **err);

/**
 * gpiodglib_line_request_get_value:
 * @self: #GpiodglibLineRequest to manipulate.
 * @offset: The offset of the line of which the value should be read.
 * @value: Return location for the value.
 * @err: Return location for error or NULL.
 *
 * Get the value of a single requested line.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean
gpiodglib_line_request_get_value(GpiodglibLineRequest *self, guint offset,
				 GpiodglibLineValue *value, GError **err);

/**
 * gpiodglib_line_request_get_values_subset:
 * @self: #GpiodglibLineRequest to manipulate.
 * @offsets: (element-type GArray): Array of offsets identifying the subset of
 * requested lines from which to read values.
 * @values: (element-type GArray): Array in which the values will be stored.
 * Can be NULL in which case a new array will be created and its location
 * stored here.
 * @err: Return location for error or NULL.
 *
 * Get the values of a subset of requested lines.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean gpiodglib_line_request_get_values_subset(GpiodglibLineRequest *self,
						  const GArray *offsets,
						  GArray **values,
						  GError **err);

/**
 * gpiodglib_line_request_get_values:
 * @self: #GpiodglibLineRequest to manipulate.
 * @values: (element-type GArray): Array in which the values will be stored.
 * Can be NULL in which case a new array will be created and its location
 * stored here.
 * @err: Return location for error or NULL.
 *
 * Get the values of all requested lines.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean gpiodglib_line_request_get_values(GpiodglibLineRequest *self,
					   GArray **values, GError **err);

/**
 * gpiodglib_line_request_set_value:
 * @self: #GpiodglibLineRequest to manipulate.
 * @offset: The offset of the line for which the value should be set.
 * @value: Value to set.
 * @err: Return location for error or NULL.
 *
 * Set the value of a single requested line.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean
gpiodglib_line_request_set_value(GpiodglibLineRequest *self, guint offset,
				 GpiodglibLineValue value, GError **err);

/**
 * gpiodglib_line_request_set_values_subset:
 * @self: #GpiodglibLineRequest to manipulate.
 * @offsets: (element-type GArray): Array of offsets identifying the requested
 * lines for which to set values.
 * @values: (element-type GArray): Array in which the values will be stored.
 * Can be NULL in which case a new array will be created and its location
 * stored here.
 * @err: Return location for error or NULL.
 *
 * Set the values of a subset of requested lines.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean gpiodglib_line_request_set_values_subset(GpiodglibLineRequest *self,
						  const GArray *offsets,
						  const GArray *values,
						  GError **err);

/**
 * gpiodglib_line_request_set_values:
 * @self: #GpiodglibLineRequest to manipulate.
 * @values: (element-type GArray): Array containing the values to set. Must be
 * sized to contain the number of values equal to the number of requested lines.
 * Each value is associated with the line identified by the corresponding entry
 * in the offset array filled by @gpiodglib_line_request_get_requested_offsets.
 * @err: Return location for error or NULL.
 *
 * Set the values of all lines associated with a request.
 *
 * Returns: TRUE on success, FALSE on failure.
 */
gboolean gpiodglib_line_request_set_values(GpiodglibLineRequest *self,
					   GArray *values, GError **err);

G_END_DECLS

#endif /* __GPIODGLIB_LINE_REQUEST_H__ */
