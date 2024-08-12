/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __GPIOD_TEST_COMMON_H__
#define __GPIOD_TEST_COMMON_H__

#include <glib.h>

#define gpiod_test_return_if_failed() \
	do { \
		if (g_test_failed()) \
			return; \
	} while (0)

#define gpiod_test_join_thread_and_return_if_failed(_thread) \
	do { \
		if (g_test_failed()) { \
			g_thread_join(_thread); \
			return; \
		} \
	} while (0)

#endif /* __GPIOD_TEST_COMMON_H__ */
