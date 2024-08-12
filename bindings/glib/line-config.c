// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

/**
 * GpiodglibLineConfig:
 *
 * The line-config object contains the configuration for lines that can be
 * used in two cases:
 *  - when making a line request
 *  - when reconfiguring a set of already requested lines.
 */
struct _GpiodglibLineConfig {
	GObject parent_instance;
	struct gpiod_line_config *handle;
};

typedef enum {
	GPIODGLIB_LINE_CONFIG_PROP_CONFIGURED_OFFSETS = 1,
} GpiodglibLineConfigProp;

G_DEFINE_TYPE(GpiodglibLineConfig, gpiodglib_line_config, G_TYPE_OBJECT);

static void gpiodglib_line_config_get_property(GObject *obj, guint prop_id,
					       GValue *val, GParamSpec *pspec)
{
	GpiodglibLineConfig *self = GPIODGLIB_LINE_CONFIG_OBJ(obj);
	g_autofree guint *offsets = NULL;
	g_autoptr(GArray) boxed = NULL;
	gsize num_offsets, i;

	switch ((GpiodglibLineConfigProp)prop_id) {
	case GPIODGLIB_LINE_CONFIG_PROP_CONFIGURED_OFFSETS:
		num_offsets = gpiod_line_config_get_num_configured_offsets(
								self->handle);
		offsets = g_malloc0(num_offsets * sizeof(guint));
		gpiod_line_config_get_configured_offsets(self->handle, offsets,
							 num_offsets);

		boxed = g_array_new(FALSE, TRUE, sizeof(guint));
		for (i = 0; i < num_offsets; i++)
			g_array_append_val(boxed, offsets[i]);

		g_value_set_boxed(val, boxed);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_line_config_finalize(GObject *obj)
{
	GpiodglibLineConfig *self = GPIODGLIB_LINE_CONFIG_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_line_config_free);

	G_OBJECT_CLASS(gpiodglib_line_config_parent_class)->finalize(obj);
}

static void
gpiodglib_line_config_class_init(GpiodglibLineConfigClass *line_config_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_config_class);

	class->get_property = gpiodglib_line_config_get_property;
	class->finalize = gpiodglib_line_config_finalize;

	/**
	 * GpiodglibLineConfig:configured-offsets:
	 *
	 * Array of offsets for which line settings have been set.
	 */
	g_object_class_install_property(class,
				GPIODGLIB_LINE_CONFIG_PROP_CONFIGURED_OFFSETS,
		g_param_spec_boxed("configured-offsets", "Configured Offsets",
			"Array of offsets for which line settings have been set.",
			G_TYPE_ARRAY,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_line_config_init(GpiodglibLineConfig *self)
{
	self->handle = gpiod_line_config_new();
	if (!self->handle)
		/* The only possible error is ENOMEM. */
		g_error("Failed to allocate memory for the request-config object.");
}

GpiodglibLineConfig *gpiodglib_line_config_new(void)
{
	return GPIODGLIB_LINE_CONFIG_OBJ(
			g_object_new(GPIODGLIB_LINE_CONFIG_TYPE, NULL));
}

void gpiodglib_line_config_reset(GpiodglibLineConfig *self)
{
	g_assert(self);

	gpiod_line_config_reset(self->handle);
}

gboolean
gpiodglib_line_config_add_line_settings(GpiodglibLineConfig *self,
					const GArray *offsets,
					GpiodglibLineSettings *settings,
					GError **err)
{
	struct gpiod_line_settings *settings_handle;
	int ret;

	g_assert(self);

	if (!offsets || !offsets->len) {
		g_set_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL,
			    "at least one offset must be specified when adding line settings");
		return FALSE;
	}

	settings_handle = settings ?
		_gpiodglib_line_settings_get_handle(settings) : NULL;
	ret = gpiod_line_config_add_line_settings(self->handle,
						  (unsigned int *)offsets->data,
						  offsets->len,
						  settings_handle);
	if (ret) {
		_gpiodglib_set_error_from_errno(err,
			"failed to add line settings to line config");
		return FALSE;
	}

	return TRUE;
}

GpiodglibLineSettings *
gpiodglib_line_config_get_line_settings(GpiodglibLineConfig *self, guint offset)
{
	struct gpiod_line_settings *settings;

	g_assert(self);

	settings = gpiod_line_config_get_line_settings(self->handle, offset);
	if (!settings) {
		if (errno == ENOENT)
			return NULL;

		/* Let's bail-out on ENOMEM/ */
		g_error("failed to retrieve line settings for offset %u: %s",
			offset, g_strerror(errno));
	}

	return _gpiodglib_line_settings_new(settings);
}

gboolean gpiodglib_line_config_set_output_values(GpiodglibLineConfig *self,
						 const GArray *values,
						 GError **err)
{
	g_autofree enum gpiod_line_value *vals = NULL;
	gint ret;
	guint i;

	g_assert(self);

	vals = g_malloc0(sizeof(*vals) * values->len);
	for (i = 0; i < values->len; i++)
		vals[i] = _gpiodglib_line_value_to_library(
				g_array_index(values, GpiodglibLineValue, i));

	ret = gpiod_line_config_set_output_values(self->handle, vals,
						  values->len);
	if (ret) {
		_gpiodglib_set_error_from_errno(err,
				"unable to set output values");
		return FALSE;
	}

	return TRUE;
}

GArray *gpiodglib_line_config_get_configured_offsets(GpiodglibLineConfig *self)
{
	return _gpiodglib_get_prop_boxed_array(G_OBJECT(self),
					       "configured-offsets");
}

struct gpiod_line_config *
_gpiodglib_line_config_get_handle(GpiodglibLineConfig *line_cfg)
{
	return line_cfg->handle;
}
