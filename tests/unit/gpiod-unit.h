/*
 * Unit testing framework for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#ifndef __GPIOD_UNIT_H__
#define __GPIOD_UNIT_H__

#include <gpiod.h>
#include <string.h>

#define GU_INIT			__attribute__((constructor))
#define GU_UNUSED		__attribute__((unused))
#define GU_PRINTF(fmt, arg)	__attribute__((format(printf, fmt, arg)))
#define GU_CLEANUP(func)	__attribute__((cleanup(func)))

#define GU_ARRAY_SIZE(x)	(sizeof(x) / sizeof(*(x)))

typedef void (*gu_test_func)(void);

struct _gu_chip_descr {
	unsigned int num_chips;
	unsigned int *num_lines;
	bool named_lines;
};

struct _gu_test {
	struct _gu_test *_next;

	const char *name;
	gu_test_func func;

	struct _gu_chip_descr chip_descr;
};

void _gu_register_test(struct _gu_test *test);
void _gu_test_failed(const char *fmt, ...) GU_PRINTF(1, 2);

#define GU_REGISTER_TEST(test)						\
	static GU_INIT void _gu_register_##test(void)			\
	{								\
		_gu_register_test(&test);				\
	}								\
	static int _gu_##test##_sentinel GU_UNUSED

/*
 * This macro should be used for code brevity instead of manually declaring
 * the gu_test structure.
 *
 * The macro accepts the following arguments:
 *   _a_func: name of the test function
 *   _a_name: name of the test case (will be shown to user)
 *   _a_named_lines: indicate whether we want the GPIO lines to be named
 *
 * The last argument must be an array of unsigned integers specifying the
 * number of GPIO lines in each subsequent mockup chip. The size of this
 * array will at the same time specify the number of gpiochips to create.
 */
#define GU_DEFINE_TEST(_a_func, _a_name, _a_named_lines, ...)		\
	static unsigned int _##_a_func##_lines[] = __VA_ARGS__;		\
	static struct _gu_test _##_a_func##_descr = {			\
		.name = _a_name,					\
		.func = _a_func,					\
		.chip_descr = {						\
			.num_chips = GU_ARRAY_SIZE(_##_a_func##_lines),	\
			.num_lines = _##_a_func##_lines,		\
			.named_lines = _a_named_lines,			\
		},							\
	};								\
	GU_REGISTER_TEST(_##_a_func##_descr)

enum {
	GU_LINES_UNNAMED = false,
	GU_LINES_NAMED = true,
};

/*
 * We want libgpiod tests to co-exist with gpiochips created by other GPIO
 * drivers. For that reason we can't just use hardcoded device file paths or
 * gpiochip names.
 *
 * The test suite detects the chips that were exported by the gpio-mockup
 * module and stores them in the internal test context structure. Test cases
 * should use the routines declared below to access the gpiochip path, name
 * or number by index corresponding with the order in which the mockup chips
 * were requested in the GU_DEFINE_TEST() macro.
 */
const char * gu_chip_path(unsigned int index);
const char * gu_chip_name(unsigned int index);
unsigned int gu_chip_num(unsigned int index);

/*
 * Every GU_ASSERT_*() macro expansion can make a test function return, so it
 * would be quite difficult to keep track of every resource allocation. At
 * the same time we don't want any deliberate memory leaks as we also use this
 * test suite to find actual memory leaks in the library with valgrind.
 *
 * For this reason, the tests should generally always use the GU_CLEANUP()
 * macro for dynamically allocated variables and objects that need closing.
 *
 * The functions below can be reused by different tests for common use cases.
 */
void gu_close_chip(struct gpiod_chip **chip);
void gu_free_str(char **str);
void gu_free_chip_iter(struct gpiod_chip_iter **iter);
void gu_free_chip_iter_noclose(struct gpiod_chip_iter **iter);
void gu_release_line(struct gpiod_line **line);

#define GU_ASSERT(statement)						\
	do {								\
		if (!(statement)) {					\
			_gu_test_failed(				\
				"assertion failed (%s:%d): '%s'",	\
				__FILE__, __LINE__, #statement);	\
			return;						\
		}							\
	} while (0)

#define GU_ASSERT_NOT_NULL(ptr)		GU_ASSERT(ptr != NULL)
#define GU_ASSERT_RET_OK(status)	GU_ASSERT(status == 0)
#define GU_ASSERT_NULL(ptr)		GU_ASSERT(ptr == NULL)
#define GU_ASSERT_EQ(a1, a2)		GU_ASSERT(a1 == a2)
#define GU_ASSERT_NOTEQ(a1, a2)		GU_ASSERT(a1 != a2)
#define GU_ASSERT_STR_EQ(s1, s2)	GU_ASSERT(strcmp(s1, s2) == 0)

#endif /* __GPIOD_UNIT_H__ */
