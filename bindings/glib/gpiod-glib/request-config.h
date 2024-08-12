/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_REQUEST_CONFIG_H__
#define __GPIODGLIB_REQUEST_CONFIG_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE(GpiodglibRequestConfig, gpiodglib_request_config,
		     GPIODGLIB, REQUEST_CONFIG, GObject);

#define GPIODGLIB_REQUEST_CONFIG_TYPE (gpiodglib_request_config_get_type())
#define GPIODGLIB_REQUEST_CONFIG_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIODGLIB_REQUEST_CONFIG_TYPE, \
				    GpiodglibRequestConfig))

/**
 * gpiodglib_request_config_new:
 * @first_prop: Name of the first property to set.
 *
 * Create a new request config object.
 *
 * Returns: New #GpiodglibRequestConfig.
 *
 * The constructor allows to set object's properties when it's first created
 * instead of having to build an empty object and then call mutators separately.
 *
 * Currently supported properties are: `consumer` and `event-buffer-size`.
 */
GpiodglibRequestConfig *
gpiodglib_request_config_new(const gchar *first_prop, ...);

/**
 * gpiodglib_request_config_set_consumer:
 * @self: #GpiodglibRequestConfig object to manipulate.
 * @consumer: Consumer name.
 *
 * Set the consumer name for the request.
 *
 * If the consumer string is too long, it will be truncated to the max
 * accepted length.
 */
void gpiodglib_request_config_set_consumer(GpiodglibRequestConfig *self,
					   const gchar *consumer);

/**
 * gpiodglib_request_config_dup_consumer:
 * @self: #GpiodglibRequestConfig object to manipulate.
 *
 * Get the consumer name configured in the request config.
 *
 * Returns: Consumer name stored in the request config. The returned string is
 * a copy and must be freed by the caller using g_free().
 */
gchar * G_GNUC_WARN_UNUSED_RESULT
gpiodglib_request_config_dup_consumer(GpiodglibRequestConfig *self);

/**
 * gpiodglib_request_config_set_event_buffer_size:
 * @self: #GpiodglibRequestConfig object to manipulate.
 * @event_buffer_size: New event buffer size.
 *
 * Set the size of the kernel event buffer for the request.
 *
 * The kernel may adjust the value if it's too high. If set to 0, the default
 * value will be used.
 */
void
gpiodglib_request_config_set_event_buffer_size(GpiodglibRequestConfig *self,
					       guint event_buffer_size);


/**
 * gpiodglib_request_config_get_event_buffer_size:
 * @self: #GpiodglibRequestConfig object to manipulate.
 *
 * Get the edge event buffer size for the request config.
 *
 * Returns: Edge event buffer size setting from the request config.
 */
guint
gpiodglib_request_config_get_event_buffer_size(GpiodglibRequestConfig *self);

G_END_DECLS

#endif /* __GPIODGLIB_REQUEST_CONFIG_H__ */
