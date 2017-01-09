/*
 * Common code for GPIO tools.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#ifndef __GPIOD_TOOLS_COMMON_H__
#define __GPIOD_TOOLS_COMMON_H__

/*
 * Various helpers for the GPIO tools.
 *
 * NOTE: This is not a stable interface - it's only to avoid duplicating
 * common code.
 */

#define UNUSED			__attribute__((unused))
#define PRINTF(fmt, arg)	__attribute__((format(printf, fmt, arg)))

void set_progname(char *name);
const char * get_progname(void);
void die(const char *fmt, ...);
void die_perror(const char *fmt, ...);

#endif /* __GPIOD_TOOLS_COMMON_H__ */
