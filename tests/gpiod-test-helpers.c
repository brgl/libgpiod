/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/*
 * Testing framework for the core library.
 *
 * This file contains functions and definitions extending the GLib unit testing
 * framework with functionalities necessary to test the libgpiod core C API as
 * well as the kernel-to-user-space interface.
 */

#include "gpiod-test-helpers.h"

GVariant *
gpiod_test_package_line_names(const struct gpiod_test_line_name *names)
{
	const struct gpiod_test_line_name *name;
	GVariantBuilder *builder;
	GVariant *ret;

	builder = g_variant_builder_new(G_VARIANT_TYPE("a(us)"));

	for (name = &names[0]; name->name; name++)
		g_variant_builder_add(builder, "(us)",
				      name->offset, name->name);

	ret = g_variant_ref_sink(g_variant_new("a(us)", builder));
	g_variant_builder_unref(builder);

	return ret;
}

GVariant *gpiod_test_package_hogs(const struct gpiod_test_hog *hogs)
{
	const struct gpiod_test_hog *hog;
	GVariantBuilder *builder;
	GVariant *ret;

	builder = g_variant_builder_new(G_VARIANT_TYPE("a(usi)"));

	for (hog = &hogs[0]; hog->name; hog++)
		g_variant_builder_add(builder, "(usi)",
				      hog->offset, hog->name, hog->direction);

	ret = g_variant_ref_sink(g_variant_new("a(usi)", builder));
	g_variant_builder_unref(builder);

	return ret;
}
