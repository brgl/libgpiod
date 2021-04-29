/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com> */

/*
 * Testing framework for the core library.
 *
 * This file contains functions and definitions extending the GLib unit testing
 * framework with functionalities necessary to test the libgpiod core C API as
 * well as the kernel-to-user-space interface.
 */

#ifndef __GPIOD_TEST_H__
#define __GPIOD_TEST_H__

#include <glib.h>
#include <gpiod.h>

/*
 * These typedefs are needed to make g_autoptr work - it doesn't accept
 * regular 'struct typename' syntax.
 */
typedef struct gpiod_chip gpiod_chip_struct;
typedef struct gpiod_line_bulk gpiod_line_bulk_struct;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(gpiod_chip_struct, gpiod_chip_unref);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(gpiod_line_bulk_struct, gpiod_line_bulk_free);

/* These are private definitions and should not be used directly. */
typedef void (*_gpiod_test_func)(void);

typedef struct _gpiod_test_case _GpiodTestCase;
struct _gpiod_test_case {
	const gchar *path;
	_gpiod_test_func func;

	guint num_chips;
	guint *chip_sizes;
	gint flags;
};

void _gpiod_test_register(_GpiodTestCase *test);

#define _GPIOD_TEST_PATH(_name) \
		"/gpiod/" GPIOD_TEST_GROUP "/" G_STRINGIFY(_name)

enum {
	/* Dummy lines for this test case should have names assigned. */
	GPIOD_TEST_FLAG_NAMED_LINES = (1 << 0),
};

/*
 * Register a test case function. The last argument is the array of numbers
 * of lines per mockup chip.
 */
#define GPIOD_TEST_CASE(_name, _flags, ...)				\
	static void _name(void);					\
	static guint _##_name##_chip_sizes[] = __VA_ARGS__;		\
	static _GpiodTestCase _##_name##_test_case = {			\
		.path = _GPIOD_TEST_PATH(_name),			\
		.func = _name,						\
		.num_chips = G_N_ELEMENTS(_##_name##_chip_sizes),	\
		.chip_sizes = _##_name##_chip_sizes,			\
		.flags = _flags,					\
	};								\
	static __attribute__((constructor)) void			\
	_##_name##_test_register(void)					\
	{								\
		_gpiod_test_register(&_##_name##_test_case);		\
	}								\
	static void _name(void)

#define GPIOD_TEST_CONSUMER "gpiod-test"

#define gpiod_test_return_if_failed()					\
	do {								\
		if (g_test_failed())					\
			return;						\
	} while (0)

/* Wrappers around libgpiomockup helpers. */
const gchar *gpiod_test_chip_path(guint idx);
const gchar *gpiod_test_chip_name(guint idx);
gint gpiod_test_chip_get_value(guint chip_index, guint line_offset);
void gpiod_test_chip_set_pull(guint chip_index, guint line_offset, gint pull);

/* Helpers for triggering line events in a separate thread. */
struct gpiod_test_event_thread;
typedef struct gpiod_test_event_thread GpiodTestEventThread;

GpiodTestEventThread *
gpiod_test_start_event_thread(guint chip_index,
			      guint line_offset, guint period_ms);
void gpiod_test_stop_event_thread(GpiodTestEventThread *thread);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GpiodTestEventThread,
			      gpiod_test_stop_event_thread);

#endif /* __GPIOD_TEST_H__ */
