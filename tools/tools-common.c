/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Common code for GPIO tools. */

#include <errno.h>
#include <gpiod.h>
#include <libgen.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>

#include "tools-common.h"

const char *get_progname(void)
{
	return program_invocation_name;
}

void die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", program_invocation_name);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);

	exit(EXIT_FAILURE);
}

void die_perror(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", program_invocation_name);
	vfprintf(stderr, fmt, va);
	fprintf(stderr, ": %s\n", strerror(errno));
	va_end(va);

	exit(EXIT_FAILURE);
}

void print_version(void)
{
	printf("%s (libgpiod) v%s\n",
	       program_invocation_short_name, gpiod_version_string());
	printf("Copyright (C) 2017-2018 Bartosz Golaszewski\n");
	printf("License: LGPLv2.1\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

int bias_flags(const char *option)
{
	if (strcmp(option, "pull-down") == 0)
		return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
	if (strcmp(option, "pull-up") == 0)
		return GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
	if (strcmp(option, "disable") == 0)
		return GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
	if (strcmp(option, "as-is") != 0)
		die("invalid bias: %s", option);
	return 0;
}

void print_bias_help(void)
{
	printf("Biases:\n");
	printf("  as-is:\tleave bias unchanged\n");
	printf("  disable:\tdisable bias\n");
	printf("  pull-up:\tenable pull-up\n");
	printf("  pull-down:\tenable pull-down\n");
}

int make_signalfd(void)
{
	sigset_t sigmask;
	int sigfd, rv;

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);

	rv = sigprocmask(SIG_BLOCK, &sigmask, NULL);
	if (rv < 0)
		die("error masking signals: %s", strerror(errno));

	sigfd = signalfd(-1, &sigmask, 0);
	if (sigfd < 0)
		die("error creating signalfd: %s", strerror(errno));

	return sigfd;
}
