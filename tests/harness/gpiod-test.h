/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

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

/* These are private definitions and should not be used directly. */

struct _gpiod_test_case {
	const gchar *path;
	void (*func)(void);
};

void _gpiod_test_register(struct _gpiod_test_case *test);

#define _GPIOD_TEST_PATH(_name) \
		"/gpiod/" GPIOD_TEST_GROUP "/" G_STRINGIFY(_name)

/*
 * Register a test case function.
 */
#define GPIOD_TEST_CASE(_name) \
	static void _gpiod_test_func_##_name(void); \
	static struct _gpiod_test_case _##_name##_test_case = { \
		.path = _GPIOD_TEST_PATH(_name), \
		.func = _gpiod_test_func_##_name, \
	}; \
	static __attribute__((constructor)) void \
	_##_name##_test_register(void) \
	{ \
		_gpiod_test_register(&_##_name##_test_case); \
	} \
	static void _gpiod_test_func_##_name(void)

#endif /* __GPIOD_TEST_H__ */
