// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

/**
 * GpiodglibInfoEvent:
 *
 * #GpiodglibInfoEvent contains information about the event itself (timestamp,
 * type) as well as a snapshot of line's status in the form of a line-info
 * object.
 */
struct _GpiodglibInfoEvent {
	GObject parent_instance;
	struct gpiod_info_event *handle;
	GpiodglibLineInfo *info;
};

typedef enum {
	GPIODGLIB_INFO_EVENT_PROP_EVENT_TYPE = 1,
	GPIODGLIB_INFO_EVENT_PROP_TIMESTAMP,
	GPIODGLIB_INFO_EVENT_PROP_LINE_INFO,
} GpiodglibInfoEventProp;

G_DEFINE_TYPE(GpiodglibInfoEvent, gpiodglib_info_event, G_TYPE_OBJECT);

static void gpiodglib_info_event_get_property(GObject *obj, guint prop_id,
					      GValue *val, GParamSpec *pspec)
{
	GpiodglibInfoEvent *self = GPIODGLIB_INFO_EVENT_OBJ(obj);
	struct gpiod_line_info *info, *cpy;
	GpiodglibInfoEventType type;

	g_assert(self->handle);

	switch ((GpiodglibInfoEventProp)prop_id) {
	case GPIODGLIB_INFO_EVENT_PROP_EVENT_TYPE:
		type = _gpiodglib_info_event_type_from_library(
				gpiod_info_event_get_event_type(self->handle));
		g_value_set_enum(val, type);
		break;
	case GPIODGLIB_INFO_EVENT_PROP_TIMESTAMP:
		g_value_set_uint64(val,
			gpiod_info_event_get_timestamp_ns(self->handle));
		break;
	case GPIODGLIB_INFO_EVENT_PROP_LINE_INFO:
		if (!self->info) {
			info = gpiod_info_event_get_line_info(self->handle);
			cpy = gpiod_line_info_copy(info);
			if (!cpy)
				g_error("Failed to allocate memory for line-info object");

			self->info = _gpiodglib_line_info_new(cpy);
		}

		g_value_set_object(val, self->info);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_info_event_dispose(GObject *obj)
{
	GpiodglibInfoEvent *self = GPIODGLIB_INFO_EVENT_OBJ(obj);

	g_clear_object(&self->info);

	G_OBJECT_CLASS(gpiodglib_info_event_parent_class)->dispose(obj);
}

static void gpiodglib_info_event_finalize(GObject *obj)
{
	GpiodglibInfoEvent *self = GPIODGLIB_INFO_EVENT_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_info_event_free);

	G_OBJECT_CLASS(gpiodglib_info_event_parent_class)->finalize(obj);
}

static void gpiodglib_info_event_class_init(GpiodglibInfoEventClass *info_event_class)
{
	GObjectClass *class = G_OBJECT_CLASS(info_event_class);

	class->get_property = gpiodglib_info_event_get_property;
	class->dispose = gpiodglib_info_event_dispose;
	class->finalize = gpiodglib_info_event_finalize;

	/**
	 * GpiodglibInfoEvent:event-type
	 *
	 * Type of the info event. One of @GPIODGLIB_INFO_EVENT_LINE_REQUESTED,
	 * @GPIODGLIB_INFO_EVENT_LINE_RELEASED or
	 * @GPIODGLIB_INFO_EVENT_LINE_CONFIG_CHANGED.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_INFO_EVENT_PROP_EVENT_TYPE,
		g_param_spec_enum("event-type", "Event Type",
			"Type of the info event.",
			GPIODGLIB_INFO_EVENT_TYPE_TYPE,
			GPIODGLIB_INFO_EVENT_LINE_REQUESTED,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibInfoEvent:timestamp-ns
	 *
	 * Timestamp (in nanoseconds).
	 */
	g_object_class_install_property(class,
					GPIODGLIB_INFO_EVENT_PROP_TIMESTAMP,
		g_param_spec_uint64("timestamp-ns",
			"Timestamp (in nanoseconds).",
			"Timestamp of the info event expressed in nanoseconds.",
			0, G_MAXUINT64, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibInfoEvent:line-info
	 *
	 * New line-info snapshot associated with this info event.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_INFO_EVENT_PROP_LINE_INFO,
		g_param_spec_object("line-info", "Line Info",
			"New line-info snapshot associated with this info event.",
			GPIODGLIB_LINE_INFO_TYPE,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_info_event_init(GpiodglibInfoEvent *self)
{
	self->handle = NULL;
	self->info = NULL;
}

GpiodglibInfoEventType gpiodglib_info_event_get_event_type(GpiodglibInfoEvent *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "event-type");
}

guint64 gpiodglib_info_event_get_timestamp_ns(GpiodglibInfoEvent *self)
{
	return _gpiodglib_get_prop_uint64(G_OBJECT(self), "timestamp-ns");
}

GpiodglibLineInfo *gpiodglib_info_event_get_line_info(GpiodglibInfoEvent *self)
{
	return GPIODGLIB_LINE_INFO_OBJ(
			_gpiodglib_get_prop_object(G_OBJECT(self), "line-info"));
}

GpiodglibInfoEvent *_gpiodglib_info_event_new(struct gpiod_info_event *handle)
{
	GpiodglibInfoEvent *event;

	event = GPIODGLIB_INFO_EVENT_OBJ(
			g_object_new(GPIODGLIB_INFO_EVENT_TYPE, NULL));
	event->handle = handle;

	return event;
}
