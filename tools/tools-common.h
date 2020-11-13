/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#ifndef __GPIOD_TOOLS_COMMON_H__
#define __GPIOD_TOOLS_COMMON_H__

/*
 * Various helpers for the GPIO tools.
 *
 * NOTE: This is not a stable interface - it's only to avoid duplicating
 * common code.
 */

#define NORETURN		__attribute__((noreturn))
#define UNUSED			__attribute__((unused))
#define PRINTF(fmt, arg)	__attribute__((format(printf, fmt, arg)))
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof(*(x)))

#define GETOPT_NULL_LONGOPT	NULL, 0, NULL, 0

const char *get_progname(void);
void die(const char *fmt, ...) NORETURN PRINTF(1, 2);
void die_perror(const char *fmt, ...) NORETURN PRINTF(1, 2);
void print_version(void);
int bias_flags(const char *option);
void print_bias_help(void);
int make_signalfd(void);

#endif /* __GPIOD_TOOLS_COMMON_H__ */
