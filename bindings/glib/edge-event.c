// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

/**
 * GpiodglibEdgeEvent:
 *
 * #GpiodglibEdgeEvent stores information about a single line edge event.
 * It contains the event type, timestamp and the offset of the line on which
 * the event occurred as well as two sequence numbers (global for all lines
 * in the associated request and local for this line only).
 */
struct _GpiodglibEdgeEvent {
	GObject parent_instance;
	struct gpiod_edge_event *handle;
};

typedef enum {
	GPIODGLIB_EDGE_EVENT_PROP_EVENT_TYPE = 1,
	GPIODGLIB_EDGE_EVENT_PROP_TIMESTAMP_NS,
	GPIODGLIB_EDGE_EVENT_PROP_LINE_OFFSET,
	GPIODGLIB_EDGE_EVENT_PROP_GLOBAL_SEQNO,
	GPIODGLIB_EDGE_EVENT_PROP_LINE_SEQNO,
} GpiodglibEdgeEventProp;

G_DEFINE_TYPE(GpiodglibEdgeEvent, gpiodglib_edge_event, G_TYPE_OBJECT);

static void gpiodglib_edge_event_get_property(GObject *obj, guint prop_id,
					      GValue *val, GParamSpec *pspec)
{
	GpiodglibEdgeEvent *self = GPIODGLIB_EDGE_EVENT_OBJ(obj);
	GpiodglibEdgeEventType type;

	g_assert(self->handle);

	switch ((GpiodglibEdgeEventProp)prop_id) {
	case GPIODGLIB_EDGE_EVENT_PROP_EVENT_TYPE:
		type = _gpiodglib_edge_event_type_from_library(
				gpiod_edge_event_get_event_type(self->handle));
		g_value_set_enum(val, type);
		break;
	case GPIODGLIB_EDGE_EVENT_PROP_TIMESTAMP_NS:
		g_value_set_uint64(val,
			gpiod_edge_event_get_timestamp_ns(self->handle));
		break;
	case GPIODGLIB_EDGE_EVENT_PROP_LINE_OFFSET:
		g_value_set_uint(val,
			gpiod_edge_event_get_line_offset(self->handle));
		break;
	case GPIODGLIB_EDGE_EVENT_PROP_GLOBAL_SEQNO:
		g_value_set_ulong(val,
			gpiod_edge_event_get_global_seqno(self->handle));
		break;
	case GPIODGLIB_EDGE_EVENT_PROP_LINE_SEQNO:
		g_value_set_ulong(val,
			gpiod_edge_event_get_line_seqno(self->handle));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_edge_event_finalize(GObject *obj)
{
	GpiodglibEdgeEvent *self = GPIODGLIB_EDGE_EVENT_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_edge_event_free);

	G_OBJECT_CLASS(gpiodglib_edge_event_parent_class)->finalize(obj);
}

static void
gpiodglib_edge_event_class_init(GpiodglibEdgeEventClass *edge_event_class)
{
	GObjectClass *class = G_OBJECT_CLASS(edge_event_class);

	class->get_property = gpiodglib_edge_event_get_property;
	class->finalize = gpiodglib_edge_event_finalize;

	/**
	 * GpiodglibEdgeEvent:event-type:
	 *
	 * Type of the edge event.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_EDGE_EVENT_PROP_EVENT_TYPE,
		g_param_spec_enum("event-type", "Event Type",
			"Type of the edge event.",
			GPIODGLIB_EDGE_EVENT_TYPE_TYPE,
			GPIODGLIB_EDGE_EVENT_RISING_EDGE,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibEdgeEvent:timestamp-ns:
	 *
	 * Timestamp of the edge event expressed in nanoseconds.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_EDGE_EVENT_PROP_TIMESTAMP_NS,
		g_param_spec_uint64("timestamp-ns",
			"Timestamp (in nanoseconds)",
			"Timestamp of the edge event expressed in nanoseconds.",
			0, G_MAXUINT64, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibEdgeEvent:line-offset:
	 *
	 * Offset of the line on which this event was registered.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_EDGE_EVENT_PROP_LINE_OFFSET,
		g_param_spec_uint("line-offset", "Line Offset",
			"Offset of the line on which this event was registered.",
			0, G_MAXUINT, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibEdgeEvent:global-seqno:
	 *
	 * Global sequence number of this event.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_EDGE_EVENT_PROP_GLOBAL_SEQNO,
		g_param_spec_ulong("global-seqno", "Global Sequence Number",
			"Global sequence number of this event.",
			0, G_MAXULONG, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibEdgeEvent:line-seqno:
	 *
	 * Event sequence number specific to the line.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_EDGE_EVENT_PROP_LINE_SEQNO,
		g_param_spec_ulong("line-seqno", "Line Sequence Number",
			"Event sequence number specific to the line.",
			0, G_MAXULONG, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_edge_event_init(GpiodglibEdgeEvent *self)
{
	self->handle = NULL;
}

GpiodglibEdgeEventType
gpiodglib_edge_event_get_event_type(GpiodglibEdgeEvent *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "event-type");
}

guint64 gpiodglib_edge_event_get_timestamp_ns(GpiodglibEdgeEvent *self)
{
	return _gpiodglib_get_prop_uint64(G_OBJECT(self), "timestamp-ns");
}

guint gpiodglib_edge_event_get_line_offset(GpiodglibEdgeEvent *self)
{
	return _gpiodglib_get_prop_uint(G_OBJECT(self), "line-offset");
}

gulong gpiodglib_edge_event_get_global_seqno(GpiodglibEdgeEvent *self)
{
	return _gpiodglib_get_prop_ulong(G_OBJECT(self), "global-seqno");
}

gulong gpiodglib_edge_event_get_line_seqno(GpiodglibEdgeEvent *self)
{
	return _gpiodglib_get_prop_ulong(G_OBJECT(self), "line-seqno");
}

GpiodglibEdgeEvent *_gpiodglib_edge_event_new(struct gpiod_edge_event *handle)
{
	GpiodglibEdgeEvent *event;

	event = GPIODGLIB_EDGE_EVENT_OBJ(
			g_object_new(GPIODGLIB_EDGE_EVENT_TYPE, NULL));
	event->handle = handle;

	return event;
}
