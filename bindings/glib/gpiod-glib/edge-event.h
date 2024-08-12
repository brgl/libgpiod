/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_EDGE_EVENT_H__
#define __GPIODGLIB_EDGE_EVENT_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

#include "line-info.h"

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibEdgeEvent, gpiodglib_edge_event,
		     GPIODGLIB, EDGE_EVENT, GObject);

#define GPIODGLIB_EDGE_EVENT_TYPE (gpiodglib_edge_event_get_type())
#define GPIODGLIB_EDGE_EVENT_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_EDGE_EVENT_TYPE, \
				    GpiodglibEdgeEvent))

/**
 * GpiodglibEdgeEventType:
 * @GPIODGLIB_EDGE_EVENT_RISING_EDGE: Rising edge event.
 * @GPIODGLIB_EDGE_EVENT_FALLING_EDGE: Falling edge event.
 *
 * Edge event types.
 */
typedef enum {
	GPIODGLIB_EDGE_EVENT_RISING_EDGE = 1,
	GPIODGLIB_EDGE_EVENT_FALLING_EDGE,
} GpiodglibEdgeEventType;

/**
 * gpiodglib_edge_event_get_event_type:
 * @self: #GpiodglibEdgeEvent to manipulate.
 *
 * Get the event type.
 *
 * Returns: The event type (@GPIODGLIB_EDGE_EVENT_RISING_EDGE or
 * @GPIODGLIB_EDGE_EVENT_FALLING_EDGE).
 */
GpiodglibEdgeEventType
gpiodglib_edge_event_get_event_type(GpiodglibEdgeEvent *self);

/**
 * gpiodglib_edge_event_get_timestamp_ns:
 * @self: #GpiodglibEdgeEvent to manipulate.
 *
 * Get the timestamp of the event.
 *
 * The source clock for the timestamp depends on the event_clock setting for
 * the line.
 *
 * Returns: Timestamp in nanoseconds.
 */
guint64 gpiodglib_edge_event_get_timestamp_ns(GpiodglibEdgeEvent *self);

/**
 * gpiodglib_edge_event_get_line_offset:
 * @self: #GpiodglibEdgeEvent to manipulate.
 *
 * Get the offset of the line which triggered the event.
 *
 * Returns: Line offset.
 */
guint gpiodglib_edge_event_get_line_offset(GpiodglibEdgeEvent *self);

/**
 * gpiodglib_edge_event_get_global_seqno:
 * @self: #GpiodglibEdgeEvent to manipulate.
 *
 * Get the global sequence number of the event.
 *
 * Returns: Sequence number of the event in the series of events for all lines
 * in the associated line request.
 */
gulong gpiodglib_edge_event_get_global_seqno(GpiodglibEdgeEvent *self);

/**
 * gpiodglib_edge_event_get_line_seqno:
 * @self: #GpiodglibEdgeEvent to manipulate.
 *
 * Get the event sequence number specific to the line.
 *
 * Returns: Sequence number of the event in the series of events only for this
 * line within the lifetime of the associated line request.
 */
gulong gpiodglib_edge_event_get_line_seqno(GpiodglibEdgeEvent *self);

G_END_DECLS

#endif /* __GPIODGLIB_EDGE_EVENT_H__ */
