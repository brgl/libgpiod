/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __GPIOD_TEST_SIM_H__
#define __GPIOD_TEST_SIM_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	G_GPIOSIM_PULL_UP = 1,
	G_GPIOSIM_PULL_DOWN,
} GPIOSimPull;

typedef enum {
	G_GPIOSIM_DIRECTION_INPUT = 1,
	G_GPIOSIM_DIRECTION_OUTPUT_HIGH,
	G_GPIOSIM_DIRECTION_OUTPUT_LOW,
} GPIOSimDirection;

G_DECLARE_FINAL_TYPE(GPIOSimChip, g_gpiosim_chip, G_GPIOSIM, CHIP, GObject);

#define G_GPIOSIM_TYPE_CHIP (g_gpiosim_chip_get_type())
#define G_GPIOSIM_CHIP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), G_GPIOSIM_TYPE_CHIP, GPIOSimChip))

#define g_gpiosim_chip_new(...) \
	G_GPIOSIM_CHIP(g_object_new(G_GPIOSIM_TYPE_CHIP, __VA_ARGS__))

const gchar *g_gpiosim_chip_get_dev_path(GPIOSimChip *self);
const gchar *g_gpiosim_chip_get_name(GPIOSimChip *self);

gint g_gpiosim_chip_get_value(GPIOSimChip *self, guint offset);
void g_gpiosim_chip_set_pull(GPIOSimChip *self, guint offset, GPIOSimPull pull);

G_END_DECLS

#endif /* __GPIOD_TEST_SIM_H__ */
