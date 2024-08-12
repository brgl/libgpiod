/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_INFO_EVENT_H__
#define __GPIODGLIB_INFO_EVENT_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "line-info.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibInfoEvent, gpiodglib_info_event,
		     GPIODGLIB, INFO_EVENT, GObject);

#define GPIODGLIB_INFO_EVENT_TYPE (gpiodglib_info_event_get_type())
#define GPIODGLIB_INFO_EVENT_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_INFO_EVENT_TYPE, \
				    GpiodglibInfoEvent))

/**
 * GpiodglibInfoEventType:
 * @GPIODGLIB_INFO_EVENT_LINE_REQUESTED: Line has been requested.
 * @GPIODGLIB_INFO_EVENT_LINE_RELEASED: Previously requested line has been
 * released.
 * @GPIODGLIB_INFO_EVENT_LINE_CONFIG_CHANGED: Line configuration has changed.
 *
 * Line status change event types.
 */
typedef enum {
	GPIODGLIB_INFO_EVENT_LINE_REQUESTED = 1,
	GPIODGLIB_INFO_EVENT_LINE_RELEASED,
	GPIODGLIB_INFO_EVENT_LINE_CONFIG_CHANGED,
} GpiodglibInfoEventType;

/**
 * gpiodglib_info_event_get_event_type:
 * @self: #GpiodglibInfoEvent to manipulate.
 *
 * Get the event type of the status change event.
 *
 * Returns: One of @GPIODGLIB_INFO_EVENT_LINE_REQUESTED,
 * @GPIODGLIB_INFO_EVENT_LINE_RELEASED or
 * @GPIODGLIB_INFO_EVENT_LINE_CONFIG_CHANGED.
 */
GpiodglibInfoEventType
gpiodglib_info_event_get_event_type(GpiodglibInfoEvent *self);

/**
 * gpiodglib_info_event_get_timestamp_ns:
 * @self: #GpiodglibInfoEvent to manipulate.
 *
 * Get the timestamp of the event.
 *
 * Returns: Timestamp in nanoseconds, read from the monotonic clock.
 */
guint64 gpiodglib_info_event_get_timestamp_ns(GpiodglibInfoEvent *self);

/**
 * gpiodglib_info_event_get_line_info:
 * @self #GpiodglibInfoEvent to manipulate.
 *
 * Get the snapshot of line-info associated with the event.
 *
 * Returns: (transfer full): New reference to the associated line-info object.
 */
GpiodglibLineInfo *gpiodglib_info_event_get_line_info(GpiodglibInfoEvent *self);

G_END_DECLS

#endif /* __GPIODGLIB_INFO_EVENT_H__ */
