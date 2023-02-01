/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#include <errno.h>
#include <gpiosim.h>
#include <stdlib.h>
#include <unistd.h>

#include "gpiod-test-sim.h"

struct _GPIOSimChip {
	GObject parent_instance;
	struct gpiosim_bank *bank;
};

enum {
	G_GPIOSIM_CHIP_PROP_DEV_PATH = 1,
	G_GPIOSIM_CHIP_PROP_NAME,
	G_GPIOSIM_CHIP_PROP_NUM_LINES,
	G_GPIOSIM_CHIP_PROP_LABEL,
	G_GPIOSIM_CHIP_PROP_LINE_NAMES,
	G_GPIOSIM_CHIP_PROP_HOGS,
};

static struct gpiosim_ctx *sim_ctx;

G_DEFINE_TYPE(GPIOSimChip, g_gpiosim_chip, G_TYPE_OBJECT);

static void g_gpiosim_ctx_unref(void)
{
	gpiosim_ctx_unref(sim_ctx);
}

static void g_gpiosim_ctx_init(void)
{
	sim_ctx = gpiosim_ctx_new();
	if (!sim_ctx)
		g_error("Unable to initialize libgpiosim: %s",
			g_strerror(errno));

	atexit(g_gpiosim_ctx_unref);
}

static void g_gpiosim_chip_constructed(GObject *obj)
{
	GPIOSimChip *self = G_GPIOSIM_CHIP(obj);
	struct gpiosim_dev *dev;
	gint ret;

	dev = gpiosim_bank_get_dev(self->bank);
	ret = gpiosim_dev_enable(dev);
	gpiosim_dev_unref(dev);
	if (ret)
		g_error("Error while trying to enable the simulated GPIO device: %s",
			g_strerror(errno));

	G_OBJECT_CLASS(g_gpiosim_chip_parent_class)->constructed(obj);
}

static void g_gpiosim_chip_get_property(GObject *obj, guint prop_id,
					GValue *val, GParamSpec *pspec)
{
	GPIOSimChip *self = G_GPIOSIM_CHIP(obj);

	switch (prop_id) {
	case G_GPIOSIM_CHIP_PROP_DEV_PATH:
		g_value_set_static_string(val,
				gpiosim_bank_get_dev_path(self->bank));
		break;
	case G_GPIOSIM_CHIP_PROP_NAME:
		g_value_set_static_string(val,
				gpiosim_bank_get_chip_name(self->bank));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiosim_chip_set_property(GObject *obj, guint prop_id,
					const GValue *val, GParamSpec *pspec)
{
	GPIOSimChip *self = G_GPIOSIM_CHIP(obj);
	gint ret, vdir, dir;
	GVariantIter *iter;
	GVariant *variant;
	guint offset;
	gchar *name;

	switch (prop_id) {
	case G_GPIOSIM_CHIP_PROP_NUM_LINES:
		ret = gpiosim_bank_set_num_lines(self->bank,
						 g_value_get_uint(val));
		if (ret)
			g_error("Unable to set the number of lines exposed by the simulated chip: %s",
				g_strerror(errno));
		break;
	case G_GPIOSIM_CHIP_PROP_LABEL:
		ret = gpiosim_bank_set_label(self->bank,
					     g_value_get_string(val));
		if (ret)
			g_error("Unable to set the label of the simulated chip: %s",
				g_strerror(errno));
		break;
	case G_GPIOSIM_CHIP_PROP_LINE_NAMES:
		variant = g_value_get_variant(val);
		if (!variant)
			break;

		iter = g_variant_iter_new(variant);

		while (g_variant_iter_loop(iter, "(us)", &offset, &name)) {
			ret = gpiosim_bank_set_line_name(self->bank,
							 offset, name);
			if (ret)
				g_error("Unable to set the name of the simulated GPIO line: %s",
					g_strerror(errno));
		}

		g_variant_iter_free(iter);
		break;
	case G_GPIOSIM_CHIP_PROP_HOGS:
		variant = g_value_get_variant(val);
		if (!variant)
			break;

		iter = g_variant_iter_new(variant);

		while (g_variant_iter_loop(iter, "(usi)",
					   &offset, &name, &vdir)) {
			switch (vdir) {
			case G_GPIOSIM_DIRECTION_INPUT:
				dir = GPIOSIM_DIRECTION_INPUT;
				break;
			case G_GPIOSIM_DIRECTION_OUTPUT_HIGH:
				dir = GPIOSIM_DIRECTION_OUTPUT_HIGH;
				break;
			case G_GPIOSIM_DIRECTION_OUTPUT_LOW:
				dir = GPIOSIM_DIRECTION_OUTPUT_LOW;
				break;
			default:
				g_error("Invalid hog direction value: %d",
					vdir);
			}

			ret = gpiosim_bank_hog_line(self->bank,
						    offset, name, dir);
			if (ret)
				g_error("Unable to hog the simulated GPIO line: %s",
					g_strerror(errno));
		}

		g_variant_iter_free(iter);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
		break;
	}
}

static void g_gpiosim_chip_dispose(GObject *obj)
{
	GPIOSimChip *self = G_GPIOSIM_CHIP(obj);
	struct gpiosim_dev *dev;
	gint ret;

	dev = gpiosim_bank_get_dev(self->bank);

	if (gpiosim_dev_is_live(dev)) {
		ret = gpiosim_dev_disable(dev);
		if (ret)
			g_error("Error while trying to disable the simulated GPIO device: %s",
				g_strerror(errno));
	}

	gpiosim_dev_unref(dev);
}

static void g_gpiosim_chip_finalize(GObject *obj)
{
	GPIOSimChip *self = G_GPIOSIM_CHIP(obj);

	gpiosim_bank_unref(self->bank);

	G_OBJECT_CLASS(g_gpiosim_chip_parent_class)->finalize(obj);
}

static void g_gpiosim_chip_class_init(GPIOSimChipClass *chip_class)
{
	GObjectClass *class = G_OBJECT_CLASS(chip_class);

	class->constructed = g_gpiosim_chip_constructed;
	class->get_property = g_gpiosim_chip_get_property;
	class->set_property = g_gpiosim_chip_set_property;
	class->dispose = g_gpiosim_chip_dispose;
	class->finalize = g_gpiosim_chip_finalize;

	g_object_class_install_property(
		class, G_GPIOSIM_CHIP_PROP_DEV_PATH,
		g_param_spec_string("dev-path", "Device path",
				    "Character device filesystem path.", NULL,
				    G_PARAM_READABLE));

	g_object_class_install_property(
		class, G_GPIOSIM_CHIP_PROP_NAME,
		g_param_spec_string(
			"name", "Chip name",
			"Name of this chip device as set by the kernel.", NULL,
			G_PARAM_READABLE));

	g_object_class_install_property(
		class, G_GPIOSIM_CHIP_PROP_NUM_LINES,
		g_param_spec_uint("num-lines", "Number of lines",
				  "Number of lines this simulated chip exposes.",
				  1, G_MAXUINT, 1,
				  G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(
		class, G_GPIOSIM_CHIP_PROP_LABEL,
		g_param_spec_string("label", "Chip label",
				    "Label of this simulated chip.", NULL,
				    G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(
		class, G_GPIOSIM_CHIP_PROP_LINE_NAMES,
		g_param_spec_variant(
			"line-names", "Line names",
			"List of names of the lines exposed by this chip",
			(GVariantType *)"a(us)", NULL,
			G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property(
		class, G_GPIOSIM_CHIP_PROP_HOGS,
		g_param_spec_variant(
			"hogs", "Line hogs",
			"List of hogged lines and their directions.",
			(GVariantType *)"a(usi)", NULL,
			G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}

static void g_gpiosim_chip_init(GPIOSimChip *self)
{
	g_autofree gchar *dev_name = NULL;
	struct gpiosim_dev *dev;

	if (!sim_ctx)
		g_gpiosim_ctx_init();

	dev = gpiosim_dev_new(sim_ctx);
	if (!dev)
		g_error("Unable to instantiate new GPIO device: %s",
			g_strerror(errno));

	self->bank = gpiosim_bank_new(dev);
	gpiosim_dev_unref(dev);
	if (!self->bank)
		g_error("Unable to instantiate new GPIO bank: %s",
			g_strerror(errno));
}

static const gchar *
g_gpiosim_chip_get_string_prop(GPIOSimChip *self, const gchar *prop)
{
	GValue val = G_VALUE_INIT;
	const gchar *str;

	g_object_get_property(G_OBJECT(self), prop, &val);
	str = g_value_get_string(&val);
	g_value_unset(&val);

	return str;
}

const gchar *g_gpiosim_chip_get_dev_path(GPIOSimChip *self)
{
	return g_gpiosim_chip_get_string_prop(self, "dev-path");
}

const gchar *g_gpiosim_chip_get_name(GPIOSimChip *self)
{
	return g_gpiosim_chip_get_string_prop(self, "name");
}

gint g_gpiosim_chip_get_value(GPIOSimChip *chip, guint offset)
{
	gint val;

	val = gpiosim_bank_get_value(chip->bank, offset);
	if (val < 0)
		g_error("Unable to read the line value: %s", g_strerror(errno));

	return val;
}

void g_gpiosim_chip_set_pull(GPIOSimChip *chip, guint offset, GPIOSimPull pull)
{
	gint ret, sim_pull;

	switch (pull) {
	case G_GPIOSIM_PULL_DOWN:
		sim_pull = GPIOSIM_PULL_DOWN;
		break;
	case G_GPIOSIM_PULL_UP:
		sim_pull = GPIOSIM_PULL_UP;
		break;
	default:
		g_error("invalid pull value");
	}

	ret = gpiosim_bank_set_pull(chip->bank, offset, sim_pull);
	if (ret)
		g_error("Unable to set the pull setting for simulated line: %s",
			g_strerror(errno));
}
