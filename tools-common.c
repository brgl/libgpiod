/*
 * Common code for GPIO tools.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include "gpiod.h"
#include "tools-common.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define NORETURN		__attribute__((noreturn))
#define PRINTF(fmt, arg)	__attribute__((format(printf, fmt, arg)))

static char *progname = "unknown";

void set_progname(char *name)
{
	progname = name;
}

const char * get_progname(void)
{
	return progname;
}

void NORETURN PRINTF(1, 2) die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);

	exit(EXIT_FAILURE);
}

void NORETURN PRINTF(1, 2) die_perror(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, ": %s\n", gpiod_strerror(gpiod_errno()));
	va_end(va);

	exit(EXIT_FAILURE);
}
