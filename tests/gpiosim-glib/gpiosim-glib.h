/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __GPIOD_TEST_SIM_H__
#define __GPIOD_TEST_SIM_H__

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	G_GPIOSIM_VALUE_ERROR = -1,
	G_GPIOSIM_VALUE_INACTIVE = 0,
	G_GPIOSIM_VALUE_ACTIVE = 1,
} GPIOSimValue;

typedef enum {
	G_GPIOSIM_PULL_UP = 1,
	G_GPIOSIM_PULL_DOWN,
} GPIOSimPull;

typedef enum {
	G_GPIOSIM_DIRECTION_INPUT = 1,
	G_GPIOSIM_DIRECTION_OUTPUT_HIGH,
	G_GPIOSIM_DIRECTION_OUTPUT_LOW,
} GPIOSimDirection;

#define G_GPIOSIM_ERROR g_gpiosim_error_quark()

typedef enum {
	G_GPIOSIM_ERR_CTX_INIT_FAILED = 1,
	G_GPIOSIM_ERR_CHIP_INIT_FAILED,
	G_GPIOSIM_ERR_CHIP_ENABLE_FAILED,
	G_GPIOSIM_ERR_GET_VALUE_FAILED,
} GPIOSimError;

GQuark g_gpiosim_error_quark(void);

G_DECLARE_FINAL_TYPE(GPIOSimChip, g_gpiosim_chip, G_GPIOSIM, CHIP, GObject);

#define G_GPIOSIM_CHIP_TYPE (g_gpiosim_chip_get_type())
#define G_GPIOSIM_CHIP_OBJ(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), G_GPIOSIM_CHIP_TYPE, GPIOSimChip))

#define g_gpiosim_chip_new(...) \
	({ \
		g_autoptr(GError) _err = NULL; \
		GPIOSimChip *_chip = G_GPIOSIM_CHIP_OBJ( \
					g_initable_new(G_GPIOSIM_CHIP_TYPE, \
						       NULL, &_err, \
						       __VA_ARGS__)); \
		g_assert_no_error(_err); \
		if (g_test_failed()) \
			return; \
		_chip; \
	})

const gchar *g_gpiosim_chip_get_dev_path(GPIOSimChip *self);
const gchar *g_gpiosim_chip_get_name(GPIOSimChip *self);

GPIOSimValue
_g_gpiosim_chip_get_value(GPIOSimChip *self, guint offset, GError **err);
void g_gpiosim_chip_set_pull(GPIOSimChip *self, guint offset, GPIOSimPull pull);

#define g_gpiosim_chip_get_value(self, offset) \
	({ \
		g_autoptr(GError) _err = NULL; \
		gint _val = _g_gpiosim_chip_get_value(self, offset, &_err); \
		g_assert_no_error(_err); \
		if (g_test_failed()) \
			return; \
		_val; \
	})

typedef struct {
	guint offset;
	const gchar *name;
} GPIOSimLineName;

typedef struct {
	guint offset;
	const gchar *name;
	GPIOSimDirection direction;
} GPIOSimHog;

GVariant *g_gpiosim_package_line_names(const GPIOSimLineName *names);
GVariant *g_gpiosim_package_hogs(const GPIOSimHog *hogs);

G_END_DECLS

#endif /* __GPIOD_TEST_SIM_H__ */
