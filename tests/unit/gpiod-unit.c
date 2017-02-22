/*
 * Unit testing framework for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-unit.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <libkmod.h>

#define NORETURN __attribute__((noreturn))

struct mockup_chip {
	char *path;
	char *name;
	unsigned int number;
};

struct test_context {
	struct mockup_chip **chips;
	size_t num_chips;
	bool test_failed;
	struct timeval mod_loaded_ts;
};

static struct {
	struct gu_test *test_list_head;
	struct gu_test *test_list_tail;
	unsigned int num_tests;
	unsigned int tests_failed;
	struct kmod_ctx *module_ctx;
	struct kmod_module *module;
	struct test_context test_ctx;
} globals;

static void vmsg(FILE *stream, const char *hdr, const char *fmt, va_list va)
{
	fprintf(stream, "[%.5s] ", hdr);
	vfprintf(stream, fmt, va);
	fputc('\n', stream);
}

void gu_msg(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg(stderr, " INFO", fmt, va);
	va_end(va);
}

void gu_err(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg(stderr, "ERROR", fmt, va);
	va_end(va);
}

static void GU_PRINTF(1, 2) NORETURN die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg(stderr, "FATAL", fmt, va);
	va_end(va);

	exit(EXIT_FAILURE);
}

static void GU_PRINTF(1, 2) NORETURN die_perr(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "[FATAL] ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, ": %s\n", strerror(errno));
	va_end(va);

	exit(EXIT_FAILURE);
}

static void * xzalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr)
		die("out of memory");

	memset(ptr, 0, size);

	return ptr;
}

static char * xstrdup(const char *str)
{
	char *ret;

	ret = strdup(str);
	if (!ret)
		die("out of memory");

	return ret;
}

static GU_PRINTF(1, 2) char * xasprintf(const char *fmt, ...)
{
	int status;
	va_list va;
	char *ret;

	va_start(va, fmt);
	status = vasprintf(&ret, fmt, va);
	if (status < 0)
		die_perr("asprintf");
	va_end(va);

	return ret;
}

static GU_PRINTF(2, 3) char * xcasprintf(size_t *count, const char *fmt, ...)
{
	int status;
	va_list va;
	char *ret;

	va_start(va, fmt);
	status = vasprintf(&ret, fmt, va);
	if (status < 0)
		die_perr("asprintf");
	va_end(va);

	*count = status;
	return ret;
}

static void  check_chip_index(unsigned int index)
{
	if (index >= globals.test_ctx.num_chips)
		die("invalid chip number requested from test code");
}

const char * gu_chip_path(unsigned int index)
{
	check_chip_index(index);

	return globals.test_ctx.chips[index]->path;
}

const char * gu_chip_name(unsigned int index)
{
	check_chip_index(index);

	return globals.test_ctx.chips[index]->name;
}

unsigned int gu_chip_num(unsigned int index)
{
	check_chip_index(index);

	return globals.test_ctx.chips[index]->number;
}

void _gu_register_test(struct gu_test *test)
{
	struct gu_test *tmp;

	if (!globals.test_list_head) {
		globals.test_list_head = globals.test_list_tail = test;
		test->_next = NULL;
	} else {
		tmp = globals.test_list_tail;
		globals.test_list_tail = test;
		test->_next = NULL;
		tmp->_next = test;
	}

	globals.num_tests++;
}

void _gu_set_test_failed(void)
{
	globals.test_ctx.test_failed = true;
}

static bool mockup_loaded(void)
{
	int state;

	if (!globals.module_ctx || !globals.module)
		return false;

	state = kmod_module_get_initstate(globals.module);

	return state == KMOD_MODULE_LIVE;
}

static void module_cleanup(void)
{
	gu_msg("cleaning up");

	if (mockup_loaded())
		kmod_module_remove_module(globals.module, 0);

	if (globals.module)
		kmod_module_unref(globals.module);

	if (globals.module_ctx)
		kmod_unref(globals.module_ctx);
}

static void check_gpio_mockup(void)
{
	const char *modpath;
	int status;

	gu_msg("checking gpio-mockup availability");

	globals.module_ctx = kmod_new(NULL, NULL);
	if (!globals.module_ctx)
		die_perr("error creating kernel module context");

	status = kmod_module_new_from_name(globals.module_ctx,
					   "gpio-mockup", &globals.module);
	if (status)
		die_perr("error allocating module info");

	/* First see if we can find the module. */
	modpath = kmod_module_get_path(globals.module);
	if (!modpath)
		die("the gpio-mockup module does not exist in the system or is built into the kernel");

	/* Then see if we can freely load and unload it. */
	status = kmod_module_probe_insert_module(globals.module, 0,
						 NULL, NULL, NULL, NULL);
	if (status)
		die_perr("unable to load gpio-mockup");

	status = kmod_module_remove_module(globals.module, 0);
	if (status)
		die_perr("unable to remove gpio-mockup");

	gu_msg("gpio-mockup ok");
}

static void test_load_module(struct gu_chip_descr *descr)
{
	char *modarg, *tmp_modarg;
	size_t modarg_len, count;
	char **line_sizes;
	unsigned int i;
	int status;

	line_sizes = xzalloc(sizeof(char *) * descr->num_chips);

	modarg_len = strlen("gpio_mockup_ranges=");
	for (i = 0; i < descr->num_chips; i++) {
		line_sizes[i] = xcasprintf(&count, "-1,%u,",
					   descr->num_lines[i]);
		modarg_len += count;
	}

	modarg = xzalloc(modarg_len + 1);
	tmp_modarg = modarg;

	status = sprintf(tmp_modarg, "gpio_mockup_ranges=");
	tmp_modarg += status;

	for (i = 0; i < descr->num_chips; i++) {
		status = sprintf(tmp_modarg, "%s", line_sizes[i]);
		tmp_modarg += status;
	}

	/*
	 * TODO Once the support for named lines in the gpio-mockup module
	 * is merged upstream, implement checking the named_lines field of
	 * the test description and setting the corresponding module param.
	 */

	modarg[modarg_len - 1] = '\0'; /* Remove the last comma. */

	gettimeofday(&globals.test_ctx.mod_loaded_ts, NULL);
	status = kmod_module_probe_insert_module(globals.module, 0,
						 modarg, NULL, NULL, NULL);
	if (status)
		die_perr("unable to load gpio-mockup");

	free(modarg);
	for (i = 0; i < descr->num_chips; i++)
		free(line_sizes[i]);
	free(line_sizes);
}

/*
 * To see if given chip is a mockup chip, check if it was created after
 * inserting the gpio-mockup module. It's not too clever, but works well
 * enough...
 */
static bool is_mockup_chip(const char *name)
{
	struct timeval gdev_created_ts;
	struct stat gdev_stat;
	char *path;
	int status;

	path = xasprintf("/dev/%s", name);

	status = stat(path, &gdev_stat);
	free(path);
	if (status < 0)
		die_perr("stat");

	gdev_created_ts.tv_sec = gdev_stat.st_ctim.tv_sec;
	gdev_created_ts.tv_usec = gdev_stat.st_ctim.tv_nsec / 1000;

	return timercmp(&globals.test_ctx.mod_loaded_ts, &gdev_created_ts, >=);
}

static int chipcmp(const void *c1, const void *c2)
{
	const struct mockup_chip *chip1 = *(const struct mockup_chip **)c1;
	const struct mockup_chip *chip2 = *(const struct mockup_chip **)c2;

	return strcmp(chip1->name, chip2->name);
}

static void test_prepare(struct gu_chip_descr *descr)
{
	struct test_context *ctx;
	struct mockup_chip *chip;
	unsigned int current = 0;
	struct dirent *dentry;
	int status;
	DIR *dir;

	ctx = &globals.test_ctx;
	memset(ctx, 0, sizeof(*ctx));

	test_load_module(descr);

	ctx->num_chips = descr->num_chips;
	ctx->chips = xzalloc(sizeof(*ctx->chips) * ctx->num_chips);

	dir = opendir("/dev");
	if (!dir)
		die_perr("error opening /dev");

	for (dentry = readdir(dir); dentry; dentry = readdir(dir)) {
		if (strncmp(dentry->d_name, "gpiochip", 8) == 0) {
			if (!is_mockup_chip(dentry->d_name))
				continue;

			chip = xzalloc(sizeof(*chip));
			ctx->chips[current++] = chip;

			chip->name = xstrdup(dentry->d_name);
			chip->path = xasprintf("/dev/%s", dentry->d_name);

			status = sscanf(dentry->d_name,
					"gpiochip%u", &chip->number);
			if (status != 1)
				die("unable to determine the chip number");
		}
	}

	qsort(ctx->chips, ctx->num_chips, sizeof(*ctx->chips), chipcmp);

	if (descr->num_chips != current)
		die("number of requested and detected mockup gpiochips is not the same");

	closedir(dir);
}

static void test_teardown(void)
{
	struct mockup_chip *chip;
	unsigned int i;
	int status;

	for (i = 0; i < globals.test_ctx.num_chips; i++) {
		chip = globals.test_ctx.chips[i];

		free(chip->path);
		free(chip->name);
		free(chip);
	}

	free(globals.test_ctx.chips);

	status = kmod_module_remove_module(globals.module, 0);
	if (status)
		die_perr("unable to remove gpio-mockup");
}

int main(int argc GU_UNUSED, char **argv GU_UNUSED)
{
	struct gu_test *test;

	atexit(module_cleanup);

	gu_msg("libgpiod unit-test suite");
	gu_msg("%u tests registered", globals.num_tests);

	check_gpio_mockup();

	gu_msg("running tests");

	for (test = globals.test_list_head; test; test = test->_next) {
		test_prepare(&test->chip_descr);

		test->func();
		gu_msg("test '%s': %s", test->name,
		       globals.test_ctx.test_failed ? "FAILED" : "OK");
		if (globals.test_ctx.test_failed)
			globals.tests_failed++;

		test_teardown();
	}

	if (!globals.tests_failed)
		gu_msg("all tests passed");
	else
		gu_err("%u out of %u tests failed",
		       globals.tests_failed, globals.num_tests);

	return globals.tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

void gu_close_chip(struct gpiod_chip **chip)
{
	if (*chip)
		gpiod_chip_close(*chip);
}

void gu_free_str(char **str)
{
	if (*str)
		free(*str);
}

void gu_free_chip_iter(struct gpiod_chip_iter **iter)
{
	if (*iter)
		gpiod_chip_iter_free(*iter);
}

void gu_release_line(struct gpiod_line **line)
{
	if (*line)
		gpiod_line_release(*line);
}
