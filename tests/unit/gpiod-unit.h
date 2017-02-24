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

struct gu_chip_descr {
	unsigned int num_chips;
	unsigned int *num_lines;
	bool named_lines;
};

struct gu_test {
	struct gu_test *_next;

	const char *name;
	gu_test_func func;

	struct gu_chip_descr chip_descr;
};

void _gu_register_test(struct gu_test *test);
void _gu_set_test_failed(void);

#define GU_REGISTER_TEST(test)						\
	static GU_INIT void _gu_register_##test(void)			\
	{								\
		_gu_register_test(&test);				\
	}								\
	static int _gu_##test##_sentinel GU_UNUSED

#define GU_DEFINE_TEST(_a_func, _a_name, _a_named_lines, ...)		\
	static unsigned int _##_a_func##_lines[] = __VA_ARGS__;		\
	static struct gu_test _##_a_func##_descr = {			\
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

void GU_PRINTF(1, 2) gu_msg(const char *fmt, ...);
void GU_PRINTF(1, 2) gu_err(const char *fmt, ...);

const char * gu_chip_path(unsigned int index);
const char * gu_chip_name(unsigned int index);
unsigned int gu_chip_num(unsigned int index);

void gu_close_chip(struct gpiod_chip **chip);
void gu_free_str(char **str);
void gu_free_chip_iter(struct gpiod_chip_iter **iter);
void gu_free_chip_iter_noclose(struct gpiod_chip_iter **iter);
void gu_release_line(struct gpiod_line **line);

#define GU_ASSERT(statement)						\
	do {								\
		if (!(statement)) {					\
			gu_err("assertion failed (%s:%d): '%s'",	\
			       __FILE__, __LINE__, #statement);		\
			_gu_set_test_failed();				\
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
