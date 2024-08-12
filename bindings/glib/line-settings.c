// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <stdarg.h>

#include "internal.h"

/**
 * GpiodglibLineSettings:
 *
 * Line settings object contains a set of line properties that can be used
 * when requesting lines or reconfiguring an existing request.
 */
struct _GpiodglibLineSettings {
	GObject parent_instance;
	struct gpiod_line_settings *handle;
};

typedef enum {
	GPIODGLIB_LINE_SETTINGS_PROP_DIRECTION = 1,
	GPIODGLIB_LINE_SETTINGS_PROP_EDGE_DETECTION,
	GPIODGLIB_LINE_SETTINGS_PROP_BIAS,
	GPIODGLIB_LINE_SETTINGS_PROP_DRIVE,
	GPIODGLIB_LINE_SETTINGS_PROP_ACTIVE_LOW,
	GPIODGLIB_LINE_SETTINGS_PROP_DEBOUNCE_PERIOD_US,
	GPIODGLIB_LINE_SETTINGS_PROP_EVENT_CLOCK,
	GPIODGLIB_LINE_SETTINGS_PROP_OUTPUT_VALUE,
} GpiodglibLineSettingsProp;

G_DEFINE_TYPE(GpiodglibLineSettings, gpiodglib_line_settings, G_TYPE_OBJECT);

static void gpiodglib_line_settings_init_handle(GpiodglibLineSettings *self)
{
	if (!self->handle) {
		self->handle = gpiod_line_settings_new();
		if (!self->handle)
			/* The only possible error is ENOMEM. */
			g_error("Failed to allocate memory for the line-settings object.");
	}
}

static void gpiodglib_line_settings_get_property(GObject *obj, guint prop_id,
						 GValue *val, GParamSpec *pspec)
{
	GpiodglibLineSettings *self = GPIODGLIB_LINE_SETTINGS_OBJ(obj);

	gpiodglib_line_settings_init_handle(self);

	switch ((GpiodglibLineSettingsProp)prop_id) {
	case GPIODGLIB_LINE_SETTINGS_PROP_DIRECTION:
		g_value_set_enum(val,
			_gpiodglib_line_direction_from_library(
				gpiod_line_settings_get_direction(
							self->handle), TRUE));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_EDGE_DETECTION:
		g_value_set_enum(val,
			_gpiodglib_line_edge_from_library(
				gpiod_line_settings_get_edge_detection(
							self->handle)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_BIAS:
		g_value_set_enum(val,
			_gpiodglib_line_bias_from_library(
				gpiod_line_settings_get_bias(self->handle),
				TRUE));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_DRIVE:
		g_value_set_enum(val,
			_gpiodglib_line_drive_from_library(
				gpiod_line_settings_get_drive(self->handle)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_ACTIVE_LOW:
		g_value_set_boolean(val,
			gpiod_line_settings_get_active_low(self->handle));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_DEBOUNCE_PERIOD_US:
		g_value_set_int64(val,
			gpiod_line_settings_get_debounce_period_us(
							self->handle));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_EVENT_CLOCK:
		g_value_set_enum(val,
			_gpiodglib_line_clock_from_library(
				gpiod_line_settings_get_event_clock(
							self->handle)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_OUTPUT_VALUE:
		g_value_set_enum(val,
			_gpiodglib_line_value_from_library(
				gpiod_line_settings_get_output_value(
							self->handle)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_line_settings_set_property(GObject *obj, guint prop_id,
						 const GValue *val,
						 GParamSpec *pspec)
{
	GpiodglibLineSettings *self = GPIODGLIB_LINE_SETTINGS_OBJ(obj);

	gpiodglib_line_settings_init_handle(self);

	switch ((GpiodglibLineSettingsProp)prop_id) {
	case GPIODGLIB_LINE_SETTINGS_PROP_DIRECTION:
		gpiod_line_settings_set_direction(self->handle,
			_gpiodglib_line_direction_to_library(
				g_value_get_enum(val)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_EDGE_DETECTION:
		gpiod_line_settings_set_edge_detection(self->handle,
			_gpiodglib_line_edge_to_library(g_value_get_enum(val)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_BIAS:
		gpiod_line_settings_set_bias(self->handle,
			_gpiodglib_line_bias_to_library(g_value_get_enum(val)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_DRIVE:
		gpiod_line_settings_set_drive(self->handle,
			_gpiodglib_line_drive_to_library(g_value_get_enum(val)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_ACTIVE_LOW:
		gpiod_line_settings_set_active_low(self->handle,
						   g_value_get_boolean(val));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_DEBOUNCE_PERIOD_US:
		gpiod_line_settings_set_debounce_period_us(self->handle,
						g_value_get_int64(val));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_EVENT_CLOCK:
		gpiod_line_settings_set_event_clock(self->handle,
			_gpiodglib_line_clock_to_library(g_value_get_enum(val)));
		break;
	case GPIODGLIB_LINE_SETTINGS_PROP_OUTPUT_VALUE:
		gpiod_line_settings_set_output_value(self->handle,
			_gpiodglib_line_value_to_library(g_value_get_enum(val)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_line_settings_finalize(GObject *obj)
{
	GpiodglibLineSettings *self = GPIODGLIB_LINE_SETTINGS_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_line_settings_free);

	G_OBJECT_CLASS(gpiodglib_line_settings_parent_class)->finalize(obj);
}

static void gpiodglib_line_settings_class_init(
			GpiodglibLineSettingsClass *line_settings_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_settings_class);

	class->set_property = gpiodglib_line_settings_set_property;
	class->get_property = gpiodglib_line_settings_get_property;
	class->finalize = gpiodglib_line_settings_finalize;

	/**
	 * GpiodglibLineSettings:direction
	 *
	 * Line direction setting.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_LINE_SETTINGS_PROP_DIRECTION,
		g_param_spec_enum("direction", "Direction",
			"Line direction setting.",
			GPIODGLIB_LINE_DIRECTION_TYPE,
			GPIODGLIB_LINE_DIRECTION_AS_IS,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:edge-detection
	 *
	 * Line edge detection setting.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_SETTINGS_PROP_EDGE_DETECTION,
		g_param_spec_enum("edge-detection", "Edge Detection",
			"Line edge detection setting.",
			GPIODGLIB_LINE_EDGE_TYPE,
			GPIODGLIB_LINE_EDGE_NONE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:bias
	 *
	 * Line bias setting.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_SETTINGS_PROP_BIAS,
		g_param_spec_enum("bias", "Bias",
			"Line bias setting.",
			GPIODGLIB_LINE_BIAS_TYPE,
			GPIODGLIB_LINE_BIAS_AS_IS,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:drive
	 *
	 * Line drive setting.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_SETTINGS_PROP_DRIVE,
		g_param_spec_enum("drive", "Drive",
			"Line drive setting.",
			GPIODGLIB_LINE_DRIVE_TYPE,
			GPIODGLIB_LINE_DRIVE_PUSH_PULL,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:active-low
	 *
	 * Line active-low settings.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_LINE_SETTINGS_PROP_ACTIVE_LOW,
		g_param_spec_boolean("active-low", "Active-Low",
			"Line active-low settings.",
			FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:debounce-period-us
	 *
	 * Line debounce period (expressed in microseconds).
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_SETTINGS_PROP_DEBOUNCE_PERIOD_US,
		g_param_spec_int64("debounce-period-us",
			"Debounce Period (in microseconds)",
			"Line debounce period (expressed in microseconds).",
			0, G_MAXINT64, 0,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:event-clock
	 *
	 * Clock used to timestamp edge events.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_SETTINGS_PROP_EVENT_CLOCK,
		g_param_spec_enum("event-clock", "Event Clock",
			"Clock used to timestamp edge events.",
			GPIODGLIB_LINE_CLOCK_TYPE,
			GPIODGLIB_LINE_CLOCK_MONOTONIC,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineSettings:output-value
	 *
	 * Line output value.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_SETTINGS_PROP_OUTPUT_VALUE,
		g_param_spec_enum("output-value", "Output Value",
			"Line output value.",
			GPIODGLIB_LINE_VALUE_TYPE,
			GPIODGLIB_LINE_VALUE_INACTIVE,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_line_settings_init(GpiodglibLineSettings *self)
{
	self->handle = NULL;
}

GpiodglibLineSettings *gpiodglib_line_settings_new(const gchar *first_prop, ...)
{
	GpiodglibLineSettings *settings;
	va_list va;

	va_start(va, first_prop);
	settings = GPIODGLIB_LINE_SETTINGS_OBJ(
			g_object_new_valist(GPIODGLIB_LINE_SETTINGS_TYPE,
					    first_prop, va));
	va_end(va);

	return settings;
}

void gpiodglib_line_settings_reset(GpiodglibLineSettings *self)
{
	g_assert(self);

	if (self->handle)
		gpiod_line_settings_reset(self->handle);
}

void gpiodglib_line_settings_set_direction(GpiodglibLineSettings *self,
					   GpiodglibLineDirection direction)
{
	_gpiodglib_set_prop_enum(G_OBJECT(self), "direction", direction);
}

GpiodglibLineDirection
gpiodglib_line_settings_get_direction(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "direction");
}

void gpiodglib_line_settings_set_edge_detection(GpiodglibLineSettings *self,
						GpiodglibLineEdge edge)
{
	_gpiodglib_set_prop_enum(G_OBJECT(self), "edge-detection", edge);
}

GpiodglibLineEdge
gpiodglib_line_settings_get_edge_detection(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "edge-detection");
}

void gpiodglib_line_settings_set_bias(GpiodglibLineSettings *self,
				      GpiodglibLineBias bias)
{
	_gpiodglib_set_prop_enum(G_OBJECT(self), "bias", bias);
}

GpiodglibLineBias gpiodglib_line_settings_get_bias(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "bias");
}

void gpiodglib_line_settings_set_drive(GpiodglibLineSettings *self,
				       GpiodglibLineDrive drive)
{
	_gpiodglib_set_prop_enum(G_OBJECT(self), "drive", drive);
}

GpiodglibLineDrive
gpiodglib_line_settings_get_drive(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "drive");
}

void gpiodglib_line_settings_set_active_low(GpiodglibLineSettings *self,
					    gboolean active_low)
{
	_gpiodglib_set_prop_bool(G_OBJECT(self), "active-low", active_low);
}

gboolean gpiodglib_line_settings_get_active_low(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_bool(G_OBJECT(self), "active-low");
}

void gpiodglib_line_settings_set_debounce_period_us(GpiodglibLineSettings *self,
						    GTimeSpan period)
{
	_gpiodglib_set_prop_timespan(G_OBJECT(self),
				     "debounce-period-us", period);
}

GTimeSpan
gpiodglib_line_settings_get_debounce_period_us(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_timespan(G_OBJECT(self),
					   "debounce-period-us");
}

void gpiodglib_line_settings_set_event_clock(GpiodglibLineSettings *self,
					     GpiodglibLineClock event_clock)
{
	_gpiodglib_set_prop_enum(G_OBJECT(self), "event-clock", event_clock);
}

GpiodglibLineClock
gpiodglib_line_settings_get_event_clock(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "event-clock");
}

void gpiodglib_line_settings_set_output_value(GpiodglibLineSettings *self,
					      GpiodglibLineValue value)
{
	_gpiodglib_set_prop_enum(G_OBJECT(self), "output-value", value);
}

GpiodglibLineValue
gpiodglib_line_settings_get_output_value(GpiodglibLineSettings *self)
{
	return _gpiodglib_get_prop_enum(G_OBJECT(self), "output-value");
}

struct gpiod_line_settings *
_gpiodglib_line_settings_get_handle(GpiodglibLineSettings *settings)
{
	return settings->handle;
}

GpiodglibLineSettings *
_gpiodglib_line_settings_new(struct gpiod_line_settings *handle)
{
	GpiodglibLineSettings *settings;

	settings = GPIODGLIB_LINE_SETTINGS_OBJ(
			g_object_new(GPIODGLIB_LINE_SETTINGS_TYPE, NULL));
	gpiod_line_settings_free(settings->handle);
	settings->handle = handle;

	return settings;
}
