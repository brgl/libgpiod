/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Testing framework - functions and definitions used by test cases. */

#ifndef __GPIOD_TEST_H__
#define __GPIOD_TEST_H__

#include <gpiod.h>
#include <string.h>

#define TEST_CONSUMER		"gpiod-test"

#define TEST_INIT		__attribute__((constructor))
#define TEST_UNUSED		__attribute__((unused))
#define TEST_PRINTF(fmt, arg)	__attribute__((format(printf, fmt, arg)))
#define TEST_CLEANUP(func)	__attribute__((cleanup(func)))
#define TEST_BIT(nr)		(1UL << (nr))

#define TEST_ARRAY_SIZE(x)	(sizeof(x) / sizeof(*(x)))

typedef void (*_test_func)(void);

struct _test_chip_descr {
	unsigned int num_chips;
	unsigned int *num_lines;
	int flags;
};

struct _test_case {
	struct _test_case *_next;

	const char *name;
	_test_func func;

	struct _test_chip_descr chip_descr;
};

void _test_register(struct _test_case *test);
void _test_print_failed(const char *fmt, ...) TEST_PRINTF(1, 2);

enum {
	TEST_FLAG_NAMED_LINES = TEST_BIT(0),
};

/*
 * This macro should be used for code brevity instead of manually declaring
 * the _test_case structure.
 *
 * The macro accepts the following arguments:
 *   _a_func: name of the test function
 *   _a_name: name of the test case (will be shown to user)
 *   _a_flags: various switches for the test case
 *
 * The last argument must be an array of unsigned integers specifying the
 * number of GPIO lines in each subsequent mockup chip. The size of this
 * array will at the same time specify the number of gpiochips to create.
 */
#define TEST_DEFINE(_a_func, _a_name, _a_flags, ...)			\
	static unsigned int _##_a_func##_lines[] = __VA_ARGS__;		\
	static struct _test_case _##_a_func##_descr = {			\
		.name = _a_name,					\
		.func = _a_func,					\
		.chip_descr = {						\
			.num_chips = TEST_ARRAY_SIZE(			\
						_##_a_func##_lines),	\
			.num_lines = _##_a_func##_lines,		\
			.flags = _a_flags,				\
		},							\
	};								\
	static TEST_INIT void _test_register_##_a_func##_test(void)	\
	{								\
		_test_register(&_##_a_func##_descr);			\
	}								\
	static int _test_##_a_func##_sentinel TEST_UNUSED

/*
 * We want libgpiod tests to co-exist with gpiochips created by other GPIO
 * drivers. For that reason we can't just use hardcoded device file paths or
 * gpiochip names.
 *
 * The test suite detects the chips that were exported by the gpio-mockup
 * module and stores them in the internal test context structure. Test cases
 * should use the routines declared below to access the gpiochip path, name
 * or number by index corresponding with the order in which the mockup chips
 * were requested in the TEST_DEFINE() macro.
 */
const char *test_chip_path(unsigned int index);
const char *test_chip_name(unsigned int index);
unsigned int test_chip_num(unsigned int index);

void test_set_event(unsigned int chip_index,
		    unsigned int line_offset, unsigned int freq);

void test_tool_run(char *tool, ...);
void test_tool_wait(void);
const char *test_tool_stdout(void);
const char *test_tool_stderr(void);
bool test_tool_exited(void);
int test_tool_exit_status(void);
void test_tool_signal(int signum);
void test_tool_stdin_write(const char *fmt, ...) TEST_PRINTF(1, 2);

/*
 * Every TEST_ASSERT_*() macro expansion can make a test function return, so
 * it would be quite difficult to keep track of every resource allocation. At
 * the same time we don't want any deliberate memory leaks as we also use this
 * test suite to find actual memory leaks in the library with valgrind.
 *
 * For this reason, the tests should generally always use the TEST_CLEANUP()
 * macro for dynamically allocated variables and objects that need closing.
 *
 * The functions below can be reused by different tests for common use cases.
 */
void test_close_chip(struct gpiod_chip **chip);
void test_line_close_chip(struct gpiod_line **line);
void test_free_chip_iter(struct gpiod_chip_iter **iter);
void test_free_chip_iter_noclose(struct gpiod_chip_iter **iter);
void test_free_line_iter(struct gpiod_line_iter **iter);

#define TEST_CLEANUP_CHIP TEST_CLEANUP(test_close_chip)

bool test_regex_match(const char *str, const char *pattern);

/*
 * Return a custom string built according to printf() formatting rules. The
 * returned string is valid until the next call to this routine.
 */
const char *test_build_str(const char *fmt, ...) TEST_PRINTF(1, 2);

#define TEST_ASSERT(statement)						\
	do {								\
		if (!(statement)) {					\
			_test_print_failed(				\
				"assertion failed (%s:%d): '%s'",	\
				__FILE__, __LINE__, #statement);	\
			return;						\
		}							\
	} while (0)

#define TEST_ASSERT_FALSE(statement)	TEST_ASSERT(!(statement))
#define TEST_ASSERT_NOT_NULL(ptr)	TEST_ASSERT((ptr) != NULL)
#define TEST_ASSERT_RET_OK(status)	TEST_ASSERT((status) == 0)
#define TEST_ASSERT_NULL(ptr)		TEST_ASSERT((ptr) == NULL)
#define TEST_ASSERT_ERRNO_IS(errnum)	TEST_ASSERT(errno == (errnum))
#define TEST_ASSERT_EQ(a1, a2)		TEST_ASSERT((a1) == (a2))
#define TEST_ASSERT_NOTEQ(a1, a2)	TEST_ASSERT((a1) != (a2))
#define TEST_ASSERT_STR_EQ(s1, s2)	TEST_ASSERT(strcmp(s1, s2) == 0)
#define TEST_ASSERT_STR_CONTAINS(s, p)	TEST_ASSERT(strstr(s, p) != NULL)
#define TEST_ASSERT_STR_NOTCONT(s, p)	TEST_ASSERT(strstr(s, p) == NULL)
#define TEST_ASSERT_REGEX_MATCH(s, p)	TEST_ASSERT(test_regex_match(s, p))
#define TEST_ASSERT_REGEX_NOMATCH(s, p)	TEST_ASSERT(!test_regex_match(s, p))

#endif /* __GPIOD_TEST_H__ */
