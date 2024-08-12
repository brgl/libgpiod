// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include "internal.h"

#define get_prop(_obj, _prop, _type) \
	({ \
		_type _ret; \
		g_object_get(_obj, _prop, &_ret, NULL); \
		_ret; \
	})

G_GNUC_INTERNAL gchar *
_gpiodglib_dup_prop_string(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, gchar *);
}

G_GNUC_INTERNAL gboolean
_gpiodglib_get_prop_bool(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, gboolean);
}

G_GNUC_INTERNAL gint _gpiodglib_get_prop_enum(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, gint);
}

G_GNUC_INTERNAL guint _gpiodglib_get_prop_uint(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, guint);
}

G_GNUC_INTERNAL guint64
_gpiodglib_get_prop_uint64(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, guint64);
}

G_GNUC_INTERNAL gulong _gpiodglib_get_prop_ulong(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, gulong);
}

G_GNUC_INTERNAL GTimeSpan
_gpiodglib_get_prop_timespan(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, GTimeSpan);
}

G_GNUC_INTERNAL GObject *
_gpiodglib_get_prop_object(GObject *obj, const gchar *prop)
{
	return G_OBJECT(get_prop(obj, prop, gpointer));
}

G_GNUC_INTERNAL gpointer
_gpiodglib_get_prop_pointer(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, gpointer);
}

G_GNUC_INTERNAL gpointer
_gpiodglib_get_prop_boxed_array(GObject *obj, const gchar *prop)
{
	return get_prop(obj, prop, gpointer);
}

#define set_prop(_obj, _prop, _val) \
	do { \
		g_object_set(_obj, _prop, _val, NULL); \
	} while (0)

G_GNUC_INTERNAL void
_gpiodglib_set_prop_uint(GObject *obj, const gchar *prop, guint val)
{
	set_prop(obj, prop, val);
}

G_GNUC_INTERNAL void
_gpiodglib_set_prop_string(GObject *obj, const gchar *prop, const gchar *val)
{
	set_prop(obj, prop, val);
}

G_GNUC_INTERNAL void
_gpiodglib_set_prop_enum(GObject *obj, const gchar *prop, gint val)
{
	set_prop(obj, prop, val);
}

G_GNUC_INTERNAL void
_gpiodglib_set_prop_bool(GObject *obj, const gchar *prop, gboolean val)
{
	set_prop(obj, prop, val);
}

G_GNUC_INTERNAL void
_gpiodglib_set_prop_timespan(GObject *obj, const gchar *prop, GTimeSpan val)
{
	set_prop(obj, prop, val);
}

G_GNUC_INTERNAL GpiodglibLineDirection
_gpiodglib_line_direction_from_library(enum gpiod_line_direction direction,
				       gboolean allow_as_is)
{
	switch (direction) {
	case GPIOD_LINE_DIRECTION_AS_IS:
		if (allow_as_is)
			return GPIODGLIB_LINE_DIRECTION_AS_IS;
		break;
	case GPIOD_LINE_DIRECTION_INPUT:
		return GPIODGLIB_LINE_DIRECTION_INPUT;
	case GPIOD_LINE_DIRECTION_OUTPUT:
		return GPIODGLIB_LINE_DIRECTION_OUTPUT;
	}

	g_error("invalid line direction value returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibLineEdge
_gpiodglib_line_edge_from_library(enum gpiod_line_edge edge)
{
	switch (edge) {
	case GPIOD_LINE_EDGE_NONE:
		return GPIODGLIB_LINE_EDGE_NONE;
	case GPIOD_LINE_EDGE_RISING:
		return GPIODGLIB_LINE_EDGE_RISING;
	case GPIOD_LINE_EDGE_FALLING:
		return GPIODGLIB_LINE_EDGE_FALLING;
	case GPIOD_LINE_EDGE_BOTH:
		return GPIODGLIB_LINE_EDGE_BOTH;
	}

	g_error("invalid line edge value returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibLineBias
_gpiodglib_line_bias_from_library(enum gpiod_line_bias bias,
				  gboolean allow_as_is)
{
	switch (bias) {
	case GPIOD_LINE_BIAS_AS_IS:
		if (allow_as_is)
			return GPIODGLIB_LINE_BIAS_AS_IS;
		break;
	case GPIOD_LINE_BIAS_UNKNOWN:
		return GPIODGLIB_LINE_BIAS_UNKNOWN;
	case GPIOD_LINE_BIAS_DISABLED:
		return GPIODGLIB_LINE_BIAS_DISABLED;
	case GPIOD_LINE_BIAS_PULL_UP:
		return GPIODGLIB_LINE_BIAS_PULL_UP;
	case GPIOD_LINE_BIAS_PULL_DOWN:
		return GPIODGLIB_LINE_BIAS_PULL_DOWN;
	}

	g_error("invalid line bias value returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibLineDrive
_gpiodglib_line_drive_from_library(enum gpiod_line_drive drive)
{
	switch (drive) {
	case GPIOD_LINE_DRIVE_PUSH_PULL:
		return GPIODGLIB_LINE_DRIVE_PUSH_PULL;
	case GPIOD_LINE_DRIVE_OPEN_DRAIN:
		return GPIODGLIB_LINE_DRIVE_OPEN_DRAIN;
	case GPIOD_LINE_DRIVE_OPEN_SOURCE:
		return GPIODGLIB_LINE_DRIVE_OPEN_SOURCE;
	}

	g_error("invalid line drive value returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibLineClock
_gpiodglib_line_clock_from_library(enum gpiod_line_clock event_clock)
{
	switch (event_clock) {
	case GPIOD_LINE_CLOCK_MONOTONIC:
		return GPIODGLIB_LINE_CLOCK_MONOTONIC;
	case GPIOD_LINE_CLOCK_REALTIME:
		return GPIODGLIB_LINE_CLOCK_REALTIME;
	case GPIOD_LINE_CLOCK_HTE:
		return GPIODGLIB_LINE_CLOCK_HTE;
	}

	g_error("invalid line event clock value returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibLineValue
_gpiodglib_line_value_from_library(enum gpiod_line_value value)
{
	switch (value) {
	case GPIOD_LINE_VALUE_INACTIVE:
		return GPIODGLIB_LINE_VALUE_INACTIVE;
	case GPIOD_LINE_VALUE_ACTIVE:
		return GPIODGLIB_LINE_VALUE_ACTIVE;
	default:
		break;
	}

	g_error("invalid line value returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibInfoEventType
_gpiodglib_info_event_type_from_library(enum gpiod_info_event_type type)
{
	switch (type) {
	case GPIOD_INFO_EVENT_LINE_REQUESTED:
		return GPIODGLIB_INFO_EVENT_LINE_REQUESTED;
	case GPIOD_INFO_EVENT_LINE_RELEASED:
		return GPIODGLIB_INFO_EVENT_LINE_RELEASED;
	case GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED:
		return GPIODGLIB_INFO_EVENT_LINE_CONFIG_CHANGED;
	}
	
	g_error("invalid info-event type returned by libgpiod");
}

G_GNUC_INTERNAL GpiodglibEdgeEventType
_gpiodglib_edge_event_type_from_library(enum gpiod_edge_event_type type)
{
	switch (type) {
	case GPIOD_EDGE_EVENT_RISING_EDGE:
		return GPIODGLIB_EDGE_EVENT_RISING_EDGE;
	case GPIOD_EDGE_EVENT_FALLING_EDGE:
		return GPIODGLIB_EDGE_EVENT_FALLING_EDGE;
	}

	g_error("invalid edge-event type returned by libgpiod");
}

G_GNUC_INTERNAL enum gpiod_line_direction
_gpiodglib_line_direction_to_library(GpiodglibLineDirection direction)
{
	switch (direction) {
	case GPIODGLIB_LINE_DIRECTION_AS_IS:
		return GPIOD_LINE_DIRECTION_AS_IS;
	case GPIODGLIB_LINE_DIRECTION_INPUT:
		return GPIOD_LINE_DIRECTION_INPUT;
	case GPIODGLIB_LINE_DIRECTION_OUTPUT:
		return GPIOD_LINE_DIRECTION_OUTPUT;
	}

	g_error("invalid line direction value");
}

G_GNUC_INTERNAL enum gpiod_line_edge
_gpiodglib_line_edge_to_library(GpiodglibLineEdge edge)
{
	switch (edge) {
	case GPIODGLIB_LINE_EDGE_NONE:
		return GPIOD_LINE_EDGE_NONE;
	case GPIODGLIB_LINE_EDGE_RISING:
		return GPIOD_LINE_EDGE_RISING;
	case GPIODGLIB_LINE_EDGE_FALLING:
		return GPIOD_LINE_EDGE_FALLING;
	case GPIODGLIB_LINE_EDGE_BOTH:
		return GPIOD_LINE_EDGE_BOTH;
	}

	g_error("invalid line edge value");
}

G_GNUC_INTERNAL enum gpiod_line_bias
_gpiodglib_line_bias_to_library(GpiodglibLineBias bias)
{
	switch (bias) {
	case GPIODGLIB_LINE_BIAS_AS_IS:
		return GPIOD_LINE_BIAS_AS_IS;
	case GPIODGLIB_LINE_BIAS_DISABLED:
		return GPIOD_LINE_BIAS_DISABLED;
	case GPIODGLIB_LINE_BIAS_PULL_UP:
		return GPIOD_LINE_BIAS_PULL_UP;
	case GPIODGLIB_LINE_BIAS_PULL_DOWN:
		return GPIOD_LINE_BIAS_PULL_DOWN;
	default:
		break;
	}

	g_error("invalid line bias value");
}

G_GNUC_INTERNAL enum gpiod_line_drive
_gpiodglib_line_drive_to_library(GpiodglibLineDrive drive)
{
	switch (drive) {
	case GPIODGLIB_LINE_DRIVE_PUSH_PULL:
		return GPIOD_LINE_DRIVE_PUSH_PULL;
	case GPIODGLIB_LINE_DRIVE_OPEN_SOURCE:
		return GPIOD_LINE_DRIVE_OPEN_SOURCE;
	case GPIODGLIB_LINE_DRIVE_OPEN_DRAIN:
		return GPIOD_LINE_DRIVE_OPEN_DRAIN;
	}

	g_error("invalid line drive value");
}

G_GNUC_INTERNAL enum gpiod_line_clock
_gpiodglib_line_clock_to_library(GpiodglibLineClock event_clock)
{
	switch (event_clock) {
	case GPIODGLIB_LINE_CLOCK_MONOTONIC:
		return GPIOD_LINE_CLOCK_MONOTONIC;
	case GPIODGLIB_LINE_CLOCK_REALTIME:
		return GPIOD_LINE_CLOCK_REALTIME;
	case GPIODGLIB_LINE_CLOCK_HTE:
		return GPIOD_LINE_CLOCK_HTE;
	}

	g_error("invalid line clock value");
}

G_GNUC_INTERNAL enum gpiod_line_value
_gpiodglib_line_value_to_library(GpiodglibLineValue value)
{
	switch (value) {
	case GPIODGLIB_LINE_VALUE_INACTIVE:
		return GPIOD_LINE_VALUE_INACTIVE;
	case GPIODGLIB_LINE_VALUE_ACTIVE:
		return GPIOD_LINE_VALUE_ACTIVE;
	}

	g_error("invalid line value");
}
