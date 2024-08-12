// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>
#include <stdarg.h>

#include "internal.h"

/**
 * GpiodglibRequestConfig:
 *
 * Request config objects are used to pass a set of options to the kernel at
 * the time of the line request.
 */
struct _GpiodglibRequestConfig {
	GObject parent_instance;
	struct gpiod_request_config *handle;
};

typedef enum {
	GPIODGLIB_REQUEST_CONFIG_PROP_CONSUMER = 1,
	GPIODGLIB_REQUEST_CONFIG_PROP_EVENT_BUFFER_SIZE,
} GpiodglibRequestConfigProp;

G_DEFINE_TYPE(GpiodglibRequestConfig, gpiodglib_request_config, G_TYPE_OBJECT);

static void gpiodglib_request_config_get_property(GObject *obj, guint prop_id,
						  GValue *val,
						  GParamSpec *pspec)
{
	GpiodglibRequestConfig *self = GPIODGLIB_REQUEST_CONFIG_OBJ(obj);

	switch ((GpiodglibRequestConfigProp)prop_id) {
	case GPIODGLIB_REQUEST_CONFIG_PROP_CONSUMER:
		g_value_set_string(val,
			gpiod_request_config_get_consumer(self->handle));
		break;
	case GPIODGLIB_REQUEST_CONFIG_PROP_EVENT_BUFFER_SIZE:
		g_value_set_uint(val,
			gpiod_request_config_get_event_buffer_size(
				self->handle));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_request_config_set_property(GObject *obj, guint prop_id,
						  const GValue *val,
						  GParamSpec *pspec)
{
	GpiodglibRequestConfig *self = GPIODGLIB_REQUEST_CONFIG_OBJ(obj);

	switch ((GpiodglibRequestConfigProp)prop_id) {
	case GPIODGLIB_REQUEST_CONFIG_PROP_CONSUMER:
		gpiod_request_config_set_consumer(self->handle,
						  g_value_get_string(val));
		break;
	case GPIODGLIB_REQUEST_CONFIG_PROP_EVENT_BUFFER_SIZE:
		gpiod_request_config_set_event_buffer_size(self->handle,
							g_value_get_uint(val));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_request_config_finalize(GObject *obj)
{
	GpiodglibRequestConfig *self = GPIODGLIB_REQUEST_CONFIG_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_request_config_free);

	G_OBJECT_CLASS(gpiodglib_request_config_parent_class)->finalize(obj);
}

static void gpiodglib_request_config_class_init(
			GpiodglibRequestConfigClass *request_config_class)
{
	GObjectClass *class = G_OBJECT_CLASS(request_config_class);

	class->set_property = gpiodglib_request_config_set_property;
	class->get_property = gpiodglib_request_config_get_property;
	class->finalize = gpiodglib_request_config_finalize;

	/**
	 * GpiodglibRequestConfig:consumer:
	 *
	 * Name of the request consumer.
	 */
	g_object_class_install_property(class,
					GPIODGLIB_REQUEST_CONFIG_PROP_CONSUMER,
		g_param_spec_string("consumer", "Consumer",
			"Name of the request consumer.",
			NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibRequestConfig:event-buffer-size:
	 *
	 * Size of the kernel event buffer size of the request.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_REQUEST_CONFIG_PROP_EVENT_BUFFER_SIZE,
		g_param_spec_uint("event-buffer-size", "Event Buffer Size",
			"Size of the kernel event buffer size of the request.",
			0, G_MAXUINT, 64,
			G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_request_config_init(GpiodglibRequestConfig *self)
{
	self->handle = gpiod_request_config_new();
	if (!self->handle)
		/* The only possible error is ENOMEM. */
		g_error("Failed to allocate memory for the request-config object.");
}

GpiodglibRequestConfig *
gpiodglib_request_config_new(const gchar *first_prop, ...)
{
	GpiodglibRequestConfig *settings;
	va_list va;

	va_start(va, first_prop);
	settings = GPIODGLIB_REQUEST_CONFIG_OBJ(
			g_object_new_valist(GPIODGLIB_REQUEST_CONFIG_TYPE,
					    first_prop, va));
	va_end(va);

	return settings;
}

void gpiodglib_request_config_set_consumer(GpiodglibRequestConfig *self,
					   const gchar *consumer)
{
	g_assert(self);

	_gpiodglib_set_prop_string(G_OBJECT(self), "consumer", consumer);
}

gchar *gpiodglib_request_config_dup_consumer(GpiodglibRequestConfig *self)
{
	g_assert(self);

	return _gpiodglib_dup_prop_string(G_OBJECT(self), "consumer");
}

void
gpiodglib_request_config_set_event_buffer_size(GpiodglibRequestConfig *self,
					       guint event_buffer_size)
{
	g_assert(self);

	_gpiodglib_set_prop_uint(G_OBJECT(self), "event-buffer-size",
				 event_buffer_size);
}

guint
gpiodglib_request_config_get_event_buffer_size(GpiodglibRequestConfig *self)
{
	g_assert(self);

	return _gpiodglib_get_prop_uint(G_OBJECT(self), "event-buffer-size");
}

struct gpiod_request_config *
_gpiodglib_request_config_get_handle(GpiodglibRequestConfig *req_cfg)
{
	return req_cfg->handle;
}
