// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gio/gio.h>

#include "internal.h"

/**
 * GpiodglibChipInfo:
 *
 * Represents an immutable snapshot of GPIO chip information.
 */
struct _GpiodglibChipInfo {
	GObject parent_instance;
	struct gpiod_chip_info *handle;
};

typedef enum {
	GPIODGLIB_CHIP_INFO_PROP_NAME = 1,
	GPIODGLIB_CHIP_INFO_PROP_LABEL,
	GPIODGLIB_CHIP_INFO_PROP_NUM_LINES,
} GpiodglibChipInfoProp;

G_DEFINE_TYPE(GpiodglibChipInfo, gpiodglib_chip_info, G_TYPE_OBJECT);

static void gpiodglib_chip_info_get_property(GObject *obj, guint prop_id,
					     GValue *val, GParamSpec *pspec)
{
	GpiodglibChipInfo *self = GPIODGLIB_CHIP_INFO_OBJ(obj);

	g_assert(self->handle);

	switch ((GpiodglibChipInfoProp)prop_id) {
	case GPIODGLIB_CHIP_INFO_PROP_NAME:
		g_value_set_string(val,
				   gpiod_chip_info_get_name(self->handle));
		break;
	case GPIODGLIB_CHIP_INFO_PROP_LABEL:
		g_value_set_string(val,
				   gpiod_chip_info_get_label(self->handle));
		break;
	case GPIODGLIB_CHIP_INFO_PROP_NUM_LINES:
		g_value_set_uint(val,
			gpiod_chip_info_get_num_lines(self->handle));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
	}
}

static void gpiodglib_chip_info_finalize(GObject *obj)
{
	GpiodglibChipInfo *self = GPIODGLIB_CHIP_INFO_OBJ(obj);

	g_clear_pointer(&self->handle, gpiod_chip_info_free);

	G_OBJECT_CLASS(gpiodglib_chip_info_parent_class)->finalize(obj);
}

static void
gpiodglib_chip_info_class_init(GpiodglibChipInfoClass *chip_info_class)
{
	GObjectClass *class = G_OBJECT_CLASS(chip_info_class);

	class->get_property = gpiodglib_chip_info_get_property;
	class->finalize = gpiodglib_chip_info_finalize;

	/**
	 * GpiodglibChipInfo:name:
	 *
	 * Name of this GPIO chip device.
	 */
	g_object_class_install_property(class, GPIODGLIB_CHIP_INFO_PROP_NAME,
		g_param_spec_string("name", "Name",
			"Name of this GPIO chip device.", NULL,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibChipInfo:label:
	 *
	 * Label of this GPIO chip device.
	 */
	g_object_class_install_property(class, GPIODGLIB_CHIP_INFO_PROP_LABEL,
		g_param_spec_string("label", "Label",
			"Label of this GPIO chip device.", NULL,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GpiodglibChipInfo:num-lines:
	 *
	 * Number of GPIO lines exposed by this chip.
	 */
	g_object_class_install_property(class, GPIODGLIB_CHIP_INFO_PROP_NUM_LINES,
		g_param_spec_uint("num-lines", "NumLines",
			"Number of GPIO lines exposed by this chip.",
			1, G_MAXUINT, 1,
			G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void gpiodglib_chip_info_init(GpiodglibChipInfo *self)
{
	self->handle = NULL;
}

gchar *gpiodglib_chip_info_dup_name(GpiodglibChipInfo *self)
{
	return _gpiodglib_dup_prop_string(G_OBJECT(self), "name");
}

gchar *gpiodglib_chip_info_dup_label(GpiodglibChipInfo *self)
{
	return _gpiodglib_dup_prop_string(G_OBJECT(self), "label");
}

guint gpiodglib_chip_info_get_num_lines(GpiodglibChipInfo *self)
{
	return _gpiodglib_get_prop_uint(G_OBJECT(self), "num-lines");
}

GpiodglibChipInfo *_gpiodglib_chip_info_new(struct gpiod_chip_info *handle)
{
	GpiodglibChipInfo *info;

	info = GPIODGLIB_CHIP_INFO_OBJ(g_object_new(GPIODGLIB_CHIP_INFO_TYPE,
						    NULL));
	info->handle = handle;

	return info;
}
