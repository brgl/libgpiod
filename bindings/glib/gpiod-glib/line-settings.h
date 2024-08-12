/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_LINE_SETTINGS_H__
#define __GPIODGLIB_LINE_SETTINGS_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "line.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibLineSettings, gpiodglib_line_settings,
		     GPIODGLIB, LINE_SETTINGS, GObject);

#define GPIODGLIB_LINE_SETTINGS_TYPE (gpiodglib_line_settings_get_type())
#define GPIODGLIB_LINE_SETTINGS_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_LINE_SETTINGS_TYPE, \
				    GpiodglibLineSettings))

/**
 * gpiodglib_line_settings_new:
 * @first_prop: Name of the first property to set.
 *
 * Create a new line settings object.
 *
 * The constructor allows to set object's properties when it's first created
 * instead of having to build an empty object and then call mutators separately.
 *
 * Currently supported properties are: `direction`, `edge-detection`, `bias`,
 * `drive`, `debounce-period-us`, `active-low`, 'event-clock` and
 * `output-value`.
 *
 * Returns: New #GpiodglibLineSettings.
 */
GpiodglibLineSettings *
gpiodglib_line_settings_new(const gchar *first_prop, ...);

/**
 * gpiodglib_line_settings_reset:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Reset the line settings object to its default values.
 */
void gpiodglib_line_settings_reset(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_direction:
 * @self: #GpiodglibLineSettings to manipulate.
 * @direction: New direction.
 *
 * Set direction.
 */
void gpiodglib_line_settings_set_direction(GpiodglibLineSettings *self,
					   GpiodglibLineDirection direction);

/**
 * gpiodglib_line_settings_get_direction:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get direction.
 *
 * Returns: Current direction.
 */
GpiodglibLineDirection
gpiodglib_line_settings_get_direction(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_edge_detection:
 * @self: #GpiodglibLineSettings to manipulate.
 * @edge: New edge detection setting.
 *
 * Set edge detection.
 */
void gpiodglib_line_settings_set_edge_detection(GpiodglibLineSettings *self,
						GpiodglibLineEdge edge);

/**
 * gpiodglib_line_settings_get_edge_detection:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get edge detection.
 *
 * Returns: Current edge detection setting.
 */
GpiodglibLineEdge
gpiodglib_line_settings_get_edge_detection(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_bias:
 * @self: #GpiodglibLineSettings to manipulate.
 * @bias: New bias.
 *
 * Set bias.
 */
void gpiodglib_line_settings_set_bias(GpiodglibLineSettings *self,
				      GpiodglibLineBias bias);

/**
 * gpiodglib_line_settings_get_bias:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get bias.
 *
 * Returns: Current bias setting.
 */
GpiodglibLineBias gpiodglib_line_settings_get_bias(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_drive:
 * @self: #GpiodglibLineSettings to manipulate.
 * @drive: New drive setting.
 *
 * Set drive.
 */
void gpiodglib_line_settings_set_drive(GpiodglibLineSettings *self,
				       GpiodglibLineDrive drive);

/**
 * gpiodglib_line_settings_get_drive:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get drive.
 *
 * Returns: Current drive setting.
 */
GpiodglibLineDrive
gpiodglib_line_settings_get_drive(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_active_low:
 * @self: #GpiodglibLineSettings to manipulate.
 * @active_low: New active-low setting.
 *
 * Set active-low setting.
 */
void gpiodglib_line_settings_set_active_low(GpiodglibLineSettings *self,
					    gboolean active_low);

/**
 * gpiodglib_line_settings_get_active_low:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get active-low setting.
 *
 * Returns: TRUE if active-low is enabled, FALSE otherwise.
 */
gboolean gpiodglib_line_settings_get_active_low(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_debounce_period_us:
 * @self: #GpiodglibLineSettings to manipulate.
 * @period: New debounce period in microseconds.
 *
 * Set debounce period.
 */
void gpiodglib_line_settings_set_debounce_period_us(GpiodglibLineSettings *self,
						    GTimeSpan period);

/**
 * gpiodglib_line_settings_get_debounce_period_us:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get debounce period.
 *
 * Returns: Current debounce period in microseconds.
 */
GTimeSpan
gpiodglib_line_settings_get_debounce_period_us(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_event_clock:
 * @self: #GpiodglibLineSettings to manipulate.
 * @event_clock: New event clock.
 *
 * Set event clock.
 */
void gpiodglib_line_settings_set_event_clock(GpiodglibLineSettings *self,
					     GpiodglibLineClock event_clock);

/**
 * gpiodglib_line_settings_get_event_clock:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get event clock setting.
 *
 * Returns: Current event clock setting.
 */
GpiodglibLineClock
gpiodglib_line_settings_get_event_clock(GpiodglibLineSettings *self);

/**
 * gpiodglib_line_settings_set_output_value:
 * @self: #GpiodglibLineSettings to manipulate.
 * @value: New output value.
 *
 * Set the output value.
 */
void gpiodglib_line_settings_set_output_value(GpiodglibLineSettings *self,
					      GpiodglibLineValue value);

/**
 * gpiodglib_line_settings_get_output_value:
 * @self: #GpiodglibLineSettings to manipulate.
 *
 * Get the output value.
 *
 * Returns: Current output value.
 */
GpiodglibLineValue
gpiodglib_line_settings_get_output_value(GpiodglibLineSettings *self);

G_END_DECLS

#endif /* __GPIODGLIB_LINE_SETTINGS_H__ */
