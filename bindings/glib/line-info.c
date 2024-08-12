// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

/**
 *  GpiodglibLineInfo:
 *
 * Line info object contains an immutable snapshot of a line's status.
 *
 * The line info contains all the publicly available information about a
 * line, which does not include the line value. The line must be requested
 * to access the line value.
 */
struct _GpiodglibLineInfo {
	GObject parent_instance;
	struct gpiod_line_info *handle;
};

typedef enum {
	GPIODGLIB_LINE_INFO_PROP_OFFSET = 1,
	GPIODGLIB_LINE_INFO_PROP_NAME,
	GPIODGLIB_LINE_INFO_PROP_USED,
	GPIODGLIB_LINE_INFO_PROP_CONSUMER,
	GPIODGLIB_LINE_INFO_PROP_DIRECTION,
	GPIODGLIB_LINE_INFO_PROP_EDGE_DETECTION,
	GPIODGLIB_LINE_INFO_PROP_BIAS,
	GPIODGLIB_LINE_INFO_PROP_DRIVE,
	GPIODGLIB_LINE_INFO_PROP_ACTIVE_LOW,
	GPIODGLIB_LINE_INFO_PROP_DEBOUNCED,
	GPIODGLIB_LINE_INFO_PROP_DEBOUNCE_PERIOD,
	GPIODGLIB_LINE_INFO_PROP_EVENT_CLOCK,
} GpiodglibLineInfoProp;

G_DEFINE_TYPE(GpiodglibLineInfo, gpiodglib_line_info, G_TYPE_OBJECT);

static void gpiodglib_line_info_get_property(GObject *obj, guint prop_id,
					     GValue *val, GParamSpec *pspec)
{
	GpiodglibLineInfo *self = GPIODGLIB_LINE_INFO_OBJ(obj);

	g_assert(self->handle);

	switch ((GpiodglibLineInfoProp)prop_id) {
	case GPIODGLIB_LINE_INFO_PROP_OFFSET:
		g_value_set_uint(val, gpiod_line_info_get_offset(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_NAME:
		g_value_set_string(val,
				   gpiod_line_info_get_name(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_USED:
		g_value_set_boolean(val, gpiod_line_info_is_used(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_CONSUMER:
		g_value_set_string(val,
				   gpiod_line_info_get_consumer(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_DIRECTION:
		g_value_set_enum(val,
			_gpiodglib_line_direction_from_library(
				gpiod_line_info_get_direction(self->handle),
				FALSE));
		break;
	case GPIODGLIB_LINE_INFO_PROP_EDGE_DETECTION:
		g_value_set_enum(val,
			_gpiodglib_line_edge_from_library(
				gpiod_line_info_get_edge_detection(
					self->handle)));
		break;
	case GPIODGLIB_LINE_INFO_PROP_BIAS:
		g_value_set_enum(val,
			_gpiodglib_line_bias_from_library(
				gpiod_line_info_get_bias(self->handle),
				FALSE));
		break;
	case GPIODGLIB_LINE_INFO_PROP_DRIVE:
		g_value_set_enum(val,
			_gpiodglib_line_drive_from_library(
				gpiod_line_info_get_drive(self->handle)));
		break;
	case GPIODGLIB_LINE_INFO_PROP_ACTIVE_LOW:
		g_value_set_boolean(val,
			gpiod_line_info_is_active_low(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_DEBOUNCED:
		g_value_set_boolean(val,
			gpiod_line_info_is_debounced(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_DEBOUNCE_PERIOD:
		g_value_set_int64(val,
			gpiod_line_info_get_debounce_period_us(self->handle));
		break;
	case GPIODGLIB_LINE_INFO_PROP_EVENT_CLOCK:
		g_value_set_enum(val,
			_gpiodglib_line_clock_from_library(
				gpiod_line_info_get_event_clock(self->handle)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_line_info_finalize(GObject *obj)
{
	GpiodglibLineInfo *self = GPIODGLIB_LINE_INFO_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_line_info_free);

	G_OBJECT_CLASS(gpiodglib_line_info_parent_class)->finalize(obj);
}

static void
gpiodglib_line_info_class_init(GpiodglibLineInfoClass *line_info_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_info_class);

	class->get_property = gpiodglib_line_info_get_property;
	class->finalize = gpiodglib_line_info_finalize;

	/**
	 * GpiodglibLineInfo:offset:
	 *
	 * Offset of the GPIO line.
	 */
	g_object_class_install_property(class, GPIODGLIB_LINE_INFO_PROP_OFFSET,
		g_param_spec_uint("offset", "Offset",
			"Offset of the GPIO line.",
			0, G_MAXUINT, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:name:
	 *
	 * Name of the GPIO line, if named.
	 */
	g_object_class_install_property(class, GPIODGLIB_LINE_INFO_PROP_NAME,
		g_param_spec_string("name", "Name",
			"Name of the GPIO line, if named.",
			NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:used:
	 *
	 * Indicates whether the GPIO line is requested for exclusive usage.
	 */
	g_object_class_install_property(class, GPIODGLIB_LINE_INFO_PROP_USED,
		g_param_spec_boolean("used", "Is Used",
			"Indicates whether the GPIO line is requested for exclusive usage.",
			FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:consumer:
	 *
	 * Name of the consumer of the GPIO line, if requested.
	 */
	g_object_class_install_property(class,
			GPIODGLIB_LINE_INFO_PROP_CONSUMER,
		g_param_spec_string("consumer", "Consumer",
			"Name of the consumer of the GPIO line, if requested.",
			NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:direction:
	 *
	 * Direction of the GPIO line.
	 */
	g_object_class_install_property(class,
			GPIODGLIB_LINE_INFO_PROP_DIRECTION,
		g_param_spec_enum("direction", "Direction",
			"Direction of the GPIO line.",
			GPIODGLIB_LINE_DIRECTION_TYPE,
			GPIODGLIB_LINE_DIRECTION_INPUT,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:edge-detection:
	 *
	 * Edge detection setting of the GPIO line.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_LINE_INFO_PROP_EDGE_DETECTION,
		g_param_spec_enum("edge-detection", "Edge Detection",
			"Edge detection setting of the GPIO line.",
			GPIODGLIB_LINE_EDGE_TYPE,
			GPIODGLIB_LINE_EDGE_NONE,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:bias:
	 *
	 * Bias setting of the GPIO line.
	 */
	g_object_class_install_property(class, GPIODGLIB_LINE_INFO_PROP_BIAS,
		g_param_spec_enum("bias", "Bias",
			"Bias setting of the GPIO line.",
			GPIODGLIB_LINE_BIAS_TYPE,
			GPIODGLIB_LINE_BIAS_UNKNOWN,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:drive:
	 *
	 * Drive setting of the GPIO line.
	 */
	g_object_class_install_property(class, GPIODGLIB_LINE_INFO_PROP_DRIVE,
		g_param_spec_enum("drive", "Drive",
			"Drive setting of the GPIO line.",
			GPIODGLIB_LINE_DRIVE_TYPE,
			GPIODGLIB_LINE_DRIVE_PUSH_PULL,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:active-low:
	 *
	 * Indicates whether the signal of the line is inverted.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_LINE_INFO_PROP_ACTIVE_LOW,
		g_param_spec_boolean("active-low", "Is Active-Low",
			"Indicates whether the signal of the line is inverted.",
			FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:debounced:
	 *
	 * Indicates whether the line is debounced (by hardware or by the
	 * kernel software debouncer).
	 */
	g_object_class_install_property(class,
			GPIODGLIB_LINE_INFO_PROP_DEBOUNCED,
		g_param_spec_boolean("debounced", "Is Debounced",
			"Indicates whether the line is debounced (by hardware or by the kernel software debouncer).",
			FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:debounce-period-us:
	 *
	 * Debounce period of the line (expressed in microseconds).
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_INFO_PROP_DEBOUNCE_PERIOD,
		g_param_spec_int64("debounce-period-us",
			"Debounce Period (in microseconds)",
			"Debounce period of the line (expressed in microseconds).",
			0, G_MAXINT64, 0,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineInfo:event-clock:
	 *
	 * Event clock used to timestamp the edge events of the line.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_LINE_INFO_PROP_EVENT_CLOCK,
		g_param_spec_enum("event-clock", "Event Clock",
			"Event clock used to timestamp the edge events of the line.",
			GPIODGLIB_LINE_CLOCK_TYPE,
			GPIODGLIB_LINE_CLOCK_MONOTONIC,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_line_info_init(GpiodglibLineInfo *self)
{
	self->handle = NULL;
}

guint gpiodglib_line_info_get_offset(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_uint(G_OBJECT(self), "offset");
}

gchar *gpiodglib_line_info_dup_name(GpiodglibLineInfo *self)
{
	return _gpiodglib_dup_prop_string(G_OBJECT(self), "name");
}

gboolean gpiodglib_line_info_is_used(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_bool(G_OBJECT(self), "used");
}

gchar *gpiodglib_line_info_dup_consumer(GpiodglibLineInfo *self)
{
	return _gpiodglib_dup_prop_string(G_OBJECT(self), "consumer");
}

GpiodglibLineDirection
gpiodglib_line_info_get_direction(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "direction");
}

GpiodglibLineEdge
gpiodglib_line_info_get_edge_detection(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "edge-detection");
}

GpiodglibLineBias gpiodglib_line_info_get_bias(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "bias");
}

GpiodglibLineDrive gpiodglib_line_info_get_drive(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "drive");
}

gboolean gpiodglib_line_info_is_active_low(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_bool(G_OBJECT(self), "active-low");
}

gboolean gpiodglib_line_info_is_debounced(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_bool(G_OBJECT(self), "debounced");
}

GTimeSpan gpiodglib_line_info_get_debounce_period_us(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_timespan(G_OBJECT(self),
					   "debounce-period-us");
}

GpiodglibLineClock gpiodglib_line_info_get_event_clock(GpiodglibLineInfo *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "event-clock");
}

GpiodglibLineInfo *_gpiodglib_line_info_new(struct gpiod_line_info *handle)
{
	GpiodglibLineInfo *info;

	info = GPIODGLIB_LINE_INFO_OBJ(g_object_new(GPIODGLIB_LINE_INFO_TYPE,
						    NULL));
	info->handle = handle;

	return info;
}
