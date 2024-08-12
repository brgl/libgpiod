// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

static const gsize event_buf_size = 64;

/**
 * GpiodglibLineRequest:
 *
 * Line request object allows interacting with a set of requested GPIO lines.
 */
struct _GpiodglibLineRequest {
	GObject parent_instance;
	struct gpiod_line_request *handle;
	struct gpiod_edge_event_buffer *event_buf;
	GSource *edge_event_src;
	guint edge_event_src_id;
	enum gpiod_line_value *val_buf;
	gboolean released;
};

typedef enum {
	GPIODGLIB_LINE_REQUEST_PROP_CHIP_NAME = 1,
	GPIODGLIB_LINE_REQUEST_PROP_REQUESTED_OFFSETS,
} GpiodglibLineRequestProp;

enum {
	GPIODGLIB_LINE_REQUEST_SIGNAL_EDGE_EVENT,
	GPIODGLIB_LINE_REQUEST_SIGNAL_LAST,
};

static guint signals[GPIODGLIB_LINE_REQUEST_SIGNAL_LAST];

G_DEFINE_TYPE(GpiodglibLineRequest, gpiodglib_line_request, G_TYPE_OBJECT);

static gboolean
gpiodglib_line_request_on_edge_event(GIOChannel *source G_GNUC_UNUSED,
				     GIOCondition condition G_GNUC_UNUSED,
				     gpointer data)
{
	struct gpiod_edge_event *event_handle, *event_copy;
	GpiodglibLineRequest *self = data;
	gint ret, i;

	ret = gpiod_line_request_read_edge_events(self->handle,
						  self->event_buf,
						  event_buf_size);
	if (ret < 0)
		return TRUE;

	for (i = 0; i < ret; i++) {
		g_autoptr(GpiodglibEdgeEvent) event = NULL;

		event_handle = gpiod_edge_event_buffer_get_event(
						self->event_buf, i);
		event_copy = gpiod_edge_event_copy(event_handle);
		if (!event_copy)
			g_error("failed to copy the edge event");

		event = _gpiodglib_edge_event_new(event_copy);

		g_signal_emit(self,
			      signals[GPIODGLIB_LINE_REQUEST_SIGNAL_EDGE_EVENT],
			      0,
			      event);
	}

	return TRUE;
}

static void gpiodglib_line_request_get_property(GObject *obj, guint prop_id,
						GValue *val, GParamSpec *pspec)
{
	GpiodglibLineRequest *self = GPIODGLIB_LINE_REQUEST_OBJ(obj);
	g_autofree guint *offsets = NULL;
	g_autoptr(GArray) boxed = NULL;
	gsize num_offsets;

	g_assert(self->handle);

	switch ((GpiodglibLineRequestProp)prop_id) {
	case GPIODGLIB_LINE_REQUEST_PROP_CHIP_NAME:
		if (gpiodglib_line_request_is_released(self))
			g_value_set_static_string(val, NULL);
		else
			g_value_set_string(val,
				gpiod_line_request_get_chip_name(self->handle));
		break;
	case GPIODGLIB_LINE_REQUEST_PROP_REQUESTED_OFFSETS:
		boxed = g_array_new(FALSE, TRUE, sizeof(guint));

		if (!gpiodglib_line_request_is_released(self)) {
			num_offsets =
				gpiod_line_request_get_num_requested_lines(
								self->handle);
			offsets = g_malloc0(num_offsets * sizeof(guint));
			gpiod_line_request_get_requested_offsets(self->handle,
								 offsets,
								 num_offsets);
			g_array_append_vals(boxed, offsets, num_offsets);
		}

		g_value_set_boxed(val, boxed);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_line_request_dispose(GObject *obj)
{
	GpiodglibLineRequest *self = GPIODGLIB_LINE_REQUEST_OBJ(obj);

	if (self->edge_event_src_id)
		g_source_remove(self->edge_event_src_id);

	G_OBJECT_CLASS(gpiodglib_line_request_parent_class)->dispose(obj);
}

static void gpiodglib_line_request_finalize(GObject *obj)
{
	GpiodglibLineRequest *self = GPIODGLIB_LINE_REQUEST_OBJ(obj);

	if (!self->released)
		gpiodglib_line_request_release(self);

	g_clear_pointer(&self->event_buf, gpiod_edge_event_buffer_free);
	g_clear_pointer(&self->val_buf, g_free);

	G_OBJECT_CLASS(gpiodglib_line_request_parent_class)->finalize(obj);
}

static void
gpiodglib_line_request_class_init(GpiodglibLineRequestClass *line_request_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_request_class);

	class->get_property = gpiodglib_line_request_get_property;
	class->dispose = gpiodglib_line_request_dispose;
	class->finalize = gpiodglib_line_request_finalize;

	/**
	 * GpiodglibLineRequest:chip-name
	 *
	 * Name of the GPIO chip this request was made on.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_REQUEST_PROP_CHIP_NAME,
		g_param_spec_string("chip-name", "Chip Name",
			"Name of the GPIO chip this request was made on.",
			NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineRequest:requested-offsets
	 *
	 * Array of requested offsets.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_REQUEST_PROP_REQUESTED_OFFSETS,
		g_param_spec_boxed("requested-offsets", "Requested offsets",
			"Array of requested offsets.",
			G_TYPE_ARRAY,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibLineRequest::edge-event:
	 * @chip: #GpiodglibLineRequest receiving the event
	 * @event: The #GpiodglibEdgeEvent
	 *
	 * Emitted when an edge event is detected on one of the requested GPIO
	 * line.
	 */
	signals[GPIODGLIB_LINE_REQUEST_SIGNAL_EDGE_EVENT] =
			g_signal_new("edge-event",
				     G_TYPE_FROM_CLASS(line_request_class),
				     G_SIGNAL_RUN_LAST,
				     0,
				     NULL,
				     NULL,
				     g_cclosure_marshal_generic,
				     G_TYPE_NONE,
				     1,
				     GPIODGLIB_EDGE_EVENT_TYPE);
}

static void gpiodglib_line_request_init(GpiodglibLineRequest *self)
{
	self->handle = NULL;
	self->event_buf = NULL;
	self->edge_event_src = NULL;
	self->released = FALSE;
}

void gpiodglib_line_request_release(GpiodglibLineRequest *self)
{
	g_assert(self);

	g_clear_pointer(&self->edge_event_src, g_source_unref);
	gpiod_line_request_release(self->handle);
	self->released = TRUE;
}

gboolean gpiodglib_line_request_is_released(GpiodglibLineRequest *self)
{
	g_assert(self);

	return self->released;
}

static void set_err_request_released(GError **err)
{
	g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_REQUEST_RELEASED,
		    "line request was released and cannot be used");
}

gchar *gpiodglib_line_request_dup_chip_name(GpiodglibLineRequest *self)
{
	return _gpiodglib_dup_prop_string(G_OBJECT(self), "chip-name");
}

GArray *gpiodglib_line_request_get_requested_offsets(GpiodglibLineRequest *self)
{
	return _gpiodglib_get_prop_boxed_array(G_OBJECT(self),
					      "requested-offsets");
}

gboolean gpiodglib_line_request_reconfigure_lines(GpiodglibLineRequest *self,
						  GpiodglibLineConfig *config,
						  GError **err)
{
	struct gpiod_line_config *config_handle;
	gint ret;

	g_assert(self && self->handle);

	if (gpiodglib_line_request_is_released(self)) {
		set_err_request_released(err);
		return FALSE;
	}

	if (!config) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "line-config is required to reconfigure lines");
		return FALSE;
	}

	config_handle = _gpiodglib_line_config_get_handle(config);

	ret = gpiod_line_request_reconfigure_lines(self->handle, config_handle);
	if (ret) {
		_gpiodglib_set_error_from_errno(err,
						"failed to reconfigure lines");
		return FALSE;
	}

	return TRUE;
}

gboolean
gpiodglib_line_request_get_value(GpiodglibLineRequest *self, guint offset,
				 GpiodglibLineValue *value, GError **err)
{
	enum gpiod_line_value val;

	g_assert(self && self->handle);

	if (gpiodglib_line_request_is_released(self)) {
		set_err_request_released(err);
		return FALSE;
	}

	val = gpiod_line_request_get_value(self->handle, offset);
	if (val == GPIOD_LINE_VALUE_ERROR) {
		_gpiodglib_set_error_from_errno(err,
			    "failed to get line value for offset %u", offset);
		return FALSE;
	}

	*value = _gpiodglib_line_value_from_library(val);
	return TRUE;
}

gboolean gpiodglib_line_request_get_values_subset(GpiodglibLineRequest *self,
						  const GArray *offsets,
						  GArray **values, GError **err)
{
	guint i;
	int ret;

	g_assert(self && self->handle);

	if (gpiodglib_line_request_is_released(self)) {
		set_err_request_released(err);
		return FALSE;
	}

	if (!offsets || !values) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "offsets and values must not be NULL");
		return FALSE;
	}

	ret = gpiod_line_request_get_values_subset(self->handle, offsets->len,
					(const unsigned int *)offsets->data,
					self->val_buf);
	if (ret) {
		_gpiodglib_set_error_from_errno(err, "failed to read line values");
		return FALSE;
	}

	if (!(*values)) {
		*values = g_array_sized_new(FALSE, TRUE,
					    sizeof(GpiodglibLineValue),
					    offsets->len);
	}

	g_array_set_size(*values, offsets->len);

	for (i = 0; i < offsets->len; i++) {
		GpiodglibLineValue *val = &g_array_index(*values,
							 GpiodglibLineValue, i);
		*val = _gpiodglib_line_value_from_library(self->val_buf[i]);
	}

	return TRUE;
}

gboolean gpiodglib_line_request_get_values(GpiodglibLineRequest *self,
					 GArray **values, GError **err)
{
	g_autoptr(GArray) offsets = NULL;

	offsets = gpiodglib_line_request_get_requested_offsets(self);

	return gpiodglib_line_request_get_values_subset(self, offsets,
							values, err);
}

gboolean gpiodglib_line_request_set_value(GpiodglibLineRequest *self,
					  guint offset,
					  GpiodglibLineValue value,
					  GError **err)
{
	int ret;

	g_assert(self && self->handle);

	if (gpiodglib_line_request_is_released(self)) {
		set_err_request_released(err);
		return FALSE;
	}

	ret = gpiod_line_request_set_value(self->handle, offset,
				_gpiodglib_line_value_to_library(value));
	if (ret) {
		_gpiodglib_set_error_from_errno(err,
			"failed to set line value for offset: %u", offset);
		return FALSE;
	}

	return TRUE;
}

gboolean gpiodglib_line_request_set_values_subset(GpiodglibLineRequest *self,
						  const GArray *offsets,
						  const GArray *values,
						  GError **err)
{
	guint i;
	int ret;

	g_assert(self && self->handle);

	if (gpiodglib_line_request_is_released(self)) {
		set_err_request_released(err);
		return FALSE;
	}

	if (!offsets || !values) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "offsets and values must not be NULL");
		return FALSE;
	}

	if (offsets->len != values->len) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "offsets and values must have the sme size");
		return FALSE;
	}

	for (i = 0; i < values->len; i++)
		self->val_buf[i] = _gpiodglib_line_value_to_library(
					g_array_index(values,
						      GpiodglibLineValue, i));

	ret = gpiod_line_request_set_values_subset(self->handle,
						  offsets->len,
						  (unsigned int *)offsets->data,
						  self->val_buf);
	if (ret) {
		_gpiodglib_set_error_from_errno(err,
					       "failed to set line values");
		return FALSE;
	}

	return TRUE;
}

gboolean gpiodglib_line_request_set_values(GpiodglibLineRequest *self,
					   GArray *values, GError **err)
{
	g_autoptr(GArray) offsets = NULL;

	offsets = gpiodglib_line_request_get_requested_offsets(self);

	return gpiodglib_line_request_set_values_subset(self, offsets,
							values, err);
}

GpiodglibLineRequest *
_gpiodglib_line_request_new(struct gpiod_line_request *handle)
{
	g_autoptr(GIOChannel) channel = NULL;
	GpiodglibLineRequest *req;
	gsize num_lines;

	req = GPIODGLIB_LINE_REQUEST_OBJ(
		g_object_new(GPIODGLIB_LINE_REQUEST_TYPE, NULL));
	req->handle = handle;

	req->event_buf = gpiod_edge_event_buffer_new(event_buf_size);
	if (!req->event_buf)
		g_error("failed to allocate the edge event buffer");

	channel = g_io_channel_unix_new(
			gpiod_line_request_get_fd(req->handle));
	req->edge_event_src = g_io_create_watch(channel, G_IO_IN);
	g_source_set_callback(
			req->edge_event_src,
			G_SOURCE_FUNC(gpiodglib_line_request_on_edge_event),
			req, NULL);
	req->edge_event_src_id = g_source_attach(req->edge_event_src, NULL);

	num_lines = gpiod_line_request_get_num_requested_lines(req->handle);
	req->val_buf = g_malloc0(sizeof(enum gpiod_line_value) * num_lines);


	return req;
}
