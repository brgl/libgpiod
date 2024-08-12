// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

/**
 * GpiodglibChip:
 *
 * Represents a single GPIO chip.
 */
struct _GpiodglibChip {
	GObject parent_instance;
	GString *path;
	GError *construct_err;
	struct gpiod_chip *handle;
	GSource *info_event_src;
	guint info_event_src_id;
};

typedef enum {
	GPIODGLIB_CHIP_PROP_PATH = 1,
} GpiodglibChipProp;

enum {
	GPIODGLIB_CHIP_SIGNAL_INFO_EVENT,
	GPIODGLIB_CHIP_SIGNAL_LAST,
};

static guint signals[GPIODGLIB_CHIP_SIGNAL_LAST];

static void g_string_free_complete(GString *str)
{
	g_string_free(str, TRUE);
}

static gboolean
gpiodglib_chip_on_info_event(GIOChannel *source G_GNUC_UNUSED,
			     GIOCondition condition G_GNUC_UNUSED,
			     gpointer data)
{
	g_autoptr(GpiodglibInfoEvent) event = NULL;
	struct gpiod_info_event *event_handle;
	GpiodglibChip *self = data;

	event_handle = gpiod_chip_read_info_event(self->handle);
	if (!event_handle)
		return TRUE;

	event = _gpiodglib_info_event_new(event_handle);

	g_signal_emit(self, signals[GPIODGLIB_CHIP_SIGNAL_INFO_EVENT], 0,
		      event);

	return TRUE;
}

static gboolean
gpiodglib_chip_initable_init(GInitable *initable,
			     GCancellable *cancellable G_GNUC_UNUSED,
			     GError **err)
{
	GpiodglibChip *self = GPIODGLIB_CHIP_OBJ(initable);

	if (self->construct_err) {
		g_propagate_error(err, g_steal_pointer(&self->construct_err));
		return FALSE;
	}

	return TRUE;
}

static void gpiodglib_chip_initable_iface_init(GInitableIface *iface)
{
	iface->init = gpiodglib_chip_initable_init;
}

G_DEFINE_TYPE_WITH_CODE(GpiodglibChip, gpiodglib_chip, G_TYPE_OBJECT,
			G_IMPLEMENT_INTERFACE(
				G_TYPE_INITABLE,
				gpiodglib_chip_initable_iface_init));

static void gpiodglib_chip_constructed(GObject *obj)
{
	GpiodglibChip *self = GPIODGLIB_CHIP_OBJ(obj);
	g_autoptr(GIOChannel) channel = NULL;

	g_assert(!self->handle);
	g_assert(self->path);

	self->handle = gpiod_chip_open(self->path->str);
	if (!self->handle) {
		_gpiodglib_set_error_from_errno(&self->construct_err,
					       "unable to open GPIO chip '%s'",
					       self->path->str);
		return;
	}

	channel = g_io_channel_unix_new(gpiod_chip_get_fd(self->handle));
	self->info_event_src = g_io_create_watch(channel, G_IO_IN);
	g_source_set_callback(self->info_event_src,
			      G_SOURCE_FUNC(gpiodglib_chip_on_info_event),
			      self, NULL);
	self->info_event_src_id = g_source_attach(self->info_event_src, NULL);

	G_OBJECT_CLASS(gpiodglib_chip_parent_class)->constructed(obj);
}

static void gpiodglib_chip_get_property(GObject *obj, guint prop_id,
					GValue *val, GParamSpec *pspec)
{
	GpiodglibChip *self = GPIODGLIB_CHIP_OBJ(obj);

	switch ((GpiodglibChipProp)prop_id) {
	case GPIODGLIB_CHIP_PROP_PATH:
		g_value_set_string(val, self->path->str);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_chip_set_property(GObject *obj, guint prop_id,
					const GValue *val, GParamSpec *pspec)
{
	GpiodglibChip *self = GPIODGLIB_CHIP_OBJ(obj);

	switch ((GpiodglibChipProp)prop_id) {
	case GPIODGLIB_CHIP_PROP_PATH:
		self->path = g_string_new(g_value_get_string(val));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

void gpiodglib_chip_close(GpiodglibChip *self)
{
	g_clear_pointer(&self->info_event_src, g_source_unref);
	g_clear_pointer(&self->handle, gpiod_chip_close);
}

static void gpiodglib_chip_dispose(GObject *obj)
{
	GpiodglibChip *self = GPIODGLIB_CHIP_OBJ(obj);

	if (self->info_event_src_id)
		g_source_remove(self->info_event_src_id);

	gpiodglib_chip_close(self);

	G_OBJECT_CLASS(gpiodglib_chip_parent_class)->dispose(obj);
}

static void gpiodglib_chip_finalize(GObject *obj)
{
	GpiodglibChip *self = GPIODGLIB_CHIP_OBJ(obj);

	g_clear_error(&self->construct_err);
	g_clear_pointer(&self->path, g_string_free_complete);

	G_OBJECT_CLASS(gpiodglib_chip_parent_class)->finalize(obj);
}

static void gpiodglib_chip_class_init(GpiodglibChipClass *chip_class)
{
	GObjectClass *class = G_OBJECT_CLASS(chip_class);

	class->constructed = gpiodglib_chip_constructed;
	class->get_property = gpiodglib_chip_get_property;
	class->set_property = gpiodglib_chip_set_property;
	class->dispose = gpiodglib_chip_dispose;
	class->finalize = gpiodglib_chip_finalize;

	/**
	 * GpiodglibChip:path:
	 *
	 * Path that was used to open this GPIO chip.
	 */
	g_object_class_install_property(class, GPIODGLIB_CHIP_PROP_PATH,
		g_param_spec_string("path", "Path",
			"Path to the GPIO chip device used to create this chip.",
			NULL,
			G_PARAM_CONSTRUCT_ONLY |
			G_PARAM_READWRITE |
			G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibChip::info-event:
	 * @chip: #GpiodglibChip receiving the event
	 * @event: The #GpiodglibInfoEvent
	 *
	 * Emitted when the state of a watched GPIO line changes.
	 */
	signals[GPIODGLIB_CHIP_SIGNAL_INFO_EVENT] =
				g_signal_new("info-event",
					     G_TYPE_FROM_CLASS(chip_class),
					     G_SIGNAL_RUN_LAST,
					     0,
					     NULL,
					     NULL,
					     g_cclosure_marshal_generic,
					     G_TYPE_NONE,
					     1,
					     GPIODGLIB_INFO_EVENT_TYPE);
}

static void gpiodglib_chip_init(GpiodglibChip *self)
{
	self->path = NULL;
	self->construct_err = NULL;
	self->handle = NULL;
	self->info_event_src = NULL;
	self->info_event_src_id = 0;
}

GpiodglibChip *gpiodglib_chip_new(const gchar *path, GError **err)
{
	return GPIODGLIB_CHIP_OBJ(g_initable_new(GPIODGLIB_CHIP_TYPE, NULL, err,
						 "path", path, NULL));
}

gboolean gpiodglib_chip_is_closed(GpiodglibChip *self)
{
	return !self->handle;
}

gchar *gpiodglib_chip_dup_path(GpiodglibChip *self)
{
	return _gpiodglib_dup_prop_string(G_OBJECT(self), "path");
}

static void set_err_chip_closed(GError **err)
{
	g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_CHIP_CLOSED,
		    "Chip was closed and cannot be used");
}

GpiodglibChipInfo *gpiodglib_chip_get_info(GpiodglibChip *self, GError **err)
{
	struct gpiod_chip_info *info;

	g_assert(self);

	if (gpiodglib_chip_is_closed(self)) {
		set_err_chip_closed(err);
		return NULL;
	}

	info = gpiod_chip_get_info(self->handle);
	if (!info) {
		_gpiodglib_set_error_from_errno(err,
			"unable to retrieve GPIO chip information");
		return NULL;
	}

	return _gpiodglib_chip_info_new(info);
}

static GpiodglibLineInfo *
gpiodglib_chip_do_get_line_info(GpiodglibChip *self, guint offset, GError **err,
			struct gpiod_line_info *(*func)(struct gpiod_chip *,
							unsigned int),
			const gchar *err_action)
{
	struct gpiod_line_info *info;

	g_assert(self);

	if (gpiodglib_chip_is_closed(self)) {
		set_err_chip_closed(err);
		return NULL;
	}

	info = func(self->handle, offset);
	if (!info) {
		_gpiodglib_set_error_from_errno(err, "unable to %s for offset %u",
						err_action, offset);
		return NULL;
	}

	return _gpiodglib_line_info_new(info);
}

GpiodglibLineInfo *
gpiodglib_chip_get_line_info(GpiodglibChip *self, guint offset, GError **err)
{
	return gpiodglib_chip_do_get_line_info(self, offset, err,
					       gpiod_chip_get_line_info,
					       "retrieve GPIO line-info");
}

GpiodglibLineInfo *
gpiodglib_chip_watch_line_info(GpiodglibChip *self, guint offset, GError **err)
{
	return gpiodglib_chip_do_get_line_info(self, offset, err,
					       gpiod_chip_watch_line_info,
					       "setup a line-info watch");
}

gboolean
gpiodglib_chip_unwatch_line_info(GpiodglibChip *self, guint offset,
				 GError **err)
{
	int ret;

	g_assert(self);

	if (gpiodglib_chip_is_closed(self)) {
		set_err_chip_closed(err);
		return FALSE;
	}

	ret = gpiod_chip_unwatch_line_info(self->handle, offset);
	if (ret) {
		_gpiodglib_set_error_from_errno(err,
			    "unable to unwatch line-info events for offset %u",
			    offset);
		return FALSE;
	}

	return TRUE;
}

gboolean
gpiodglib_chip_get_line_offset_from_name(GpiodglibChip *self, const gchar *name,
					 guint *offset, GError **err)
{
	gint ret;

	g_assert(self);

	if (gpiodglib_chip_is_closed(self)) {
		set_err_chip_closed(err);
		return FALSE;
	}

	if (!name) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "name must not be NULL");
		return FALSE;
	}

	ret = gpiod_chip_get_line_offset_from_name(self->handle, name);
	if (ret < 0) {
		if (errno != ENOENT)
			_gpiodglib_set_error_from_errno(err,
				    "failed to map line name to offset");
		else
			errno = 0;

		return FALSE;
	}

	if (offset)
		*offset = ret;

	return TRUE;
}

GpiodglibLineRequest *
gpiodglib_chip_request_lines(GpiodglibChip *self,
			     GpiodglibRequestConfig *req_cfg,
			     GpiodglibLineConfig *line_cfg, GError **err)
{
	struct gpiod_request_config *req_cfg_handle;
	struct gpiod_line_config *line_cfg_handle;
	struct gpiod_line_request *req;

	g_assert(self);

	if (gpiodglib_chip_is_closed(self)) {
		set_err_chip_closed(err);
		return NULL;
	}

	if (!line_cfg) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "line-config is required for request");
		return NULL;
	}

	req_cfg_handle = req_cfg ?
		_gpiodglib_request_config_get_handle(req_cfg) : NULL;
	line_cfg_handle = _gpiodglib_line_config_get_handle(line_cfg);

	req = gpiod_chip_request_lines(self->handle,
				       req_cfg_handle, line_cfg_handle);
	if (!req) {
		_gpiodglib_set_error_from_errno(err,
				"failed to request GPIO lines");
		return NULL;
	}

	return _gpiodglib_line_request_new(req);
}
