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
#include <unistd.h>
#include <poll.h>
#include <libkmod.h>
#include <libudev.h>

#define NORETURN __attribute__((noreturn))

static const char mockup_devpath[] = "/devices/platform/gpio-mockup/gpiochip";

struct mockup_chip {
	char *path;
	char *name;
	unsigned int number;
};

struct test_context {
	struct mockup_chip **chips;
	size_t num_chips;
	bool test_failed;
};

static struct {
	struct _gu_test *test_list_head;
	struct _gu_test *test_list_tail;
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

static void GU_PRINTF(1, 2) msg(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg(stderr, " INFO", fmt, va);
	va_end(va);
}

static void GU_PRINTF(1, 2) err(const char *fmt, ...)
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

static GU_PRINTF(2, 3) char * xappend(char *str, const char *fmt, ...)
{
	char *new, *ret;
	va_list va;
	int status;

	va_start(va, fmt);
	status = vasprintf(&new, fmt, va);
	va_end(va);
	if (status < 0)
		die_perr("vasprintf");

	if (!str)
		return new;

	ret = realloc(str, strlen(str) + strlen(new) + 1);
	if (!ret)
		die("out of memory");

	strcat(ret, new);
	free(new);

	return ret;
}

static void check_chip_index(unsigned int index)
{
	if (index >= globals.test_ctx.num_chips)
		die("invalid chip number requested from test code");
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
	msg("cleaning up");

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

	msg("checking gpio-mockup availability");

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

	msg("gpio-mockup ok");
}

static void test_load_module(struct _gu_chip_descr *descr)
{
	unsigned int i;
	char *modarg;
	int status;

	modarg = xappend(NULL, "gpio_mockup_ranges=");
	for (i = 0; i < descr->num_chips; i++)
		modarg = xappend(modarg, "-1,%u,", descr->num_lines[i]);
	modarg[strlen(modarg) - 1] = '\0'; /* Remove the last comma. */

	/*
	 * TODO Once the support for named lines in the gpio-mockup module
	 * is merged upstream, implement checking the named_lines field of
	 * the test description and setting the corresponding module param.
	 */

	status = kmod_module_probe_insert_module(globals.module, 0,
						 modarg, NULL, NULL, NULL);
	if (status)
		die_perr("unable to load gpio-mockup");

	free(modarg);
}

static int chipcmp(const void *c1, const void *c2)
{
	const struct mockup_chip *chip1 = *(const struct mockup_chip **)c1;
	const struct mockup_chip *chip2 = *(const struct mockup_chip **)c2;

	return strcmp(chip1->name, chip2->name);
}

static bool devpath_is_mockup(const char *devpath)
{
	return !strncmp(devpath, mockup_devpath, sizeof(mockup_devpath) - 1);
}

static void test_prepare(struct _gu_chip_descr *descr)
{
	const char *devpath, *devnode, *sysname;
	struct udev_monitor *monitor;
	unsigned int detected = 0;
	struct test_context *ctx;
	struct mockup_chip *chip;
	struct udev_device *dev;
	struct udev *udev_ctx;
	struct pollfd pfd;
	int status;

	ctx = &globals.test_ctx;
	memset(ctx, 0, sizeof(*ctx));
	ctx->num_chips = descr->num_chips;
	ctx->chips = xzalloc(sizeof(*ctx->chips) * ctx->num_chips);

	/*
	 * We'll setup the udev monitor, insert the module and wait for the
	 * mockup gpiochips to appear.
	 */

	udev_ctx = udev_new();
	if (!udev_ctx)
		die_perr("error creating udev context");

	monitor = udev_monitor_new_from_netlink(udev_ctx, "udev");
	if (!monitor)
		die_perr("error creating udev monitor");

	status = udev_monitor_filter_add_match_subsystem_devtype(monitor,
								 "gpio", NULL);
	if (status < 0)
		die_perr("error adding udev filters");

	status = udev_monitor_enable_receiving(monitor);
	if (status < 0)
		die_perr("error enabling udev event receiving");

	test_load_module(descr);

	pfd.fd = udev_monitor_get_fd(monitor);
	pfd.events = POLLIN | POLLPRI;

	while (ctx->num_chips > detected) {
		status = poll(&pfd, 1, 5000);
		if (status < 0)
			die_perr("error polling for uevents");
		else if (status == 0)
			die("timeout waiting for gpiochips");

		dev = udev_monitor_receive_device(monitor);
		if (!dev)
			die_perr("error reading device info");

		devpath = udev_device_get_devpath(dev);
		devnode = udev_device_get_devnode(dev);
		sysname = udev_device_get_sysname(dev);

		if (!devpath || !devnode || !sysname ||
		    !devpath_is_mockup(devpath)) {
			udev_device_unref(dev);
			continue;
		}

		chip = xzalloc(sizeof(*chip));
		chip->name = xstrdup(sysname);
		chip->path = xstrdup(devnode);
		status = sscanf(sysname, "gpiochip%u", &chip->number);
		if (status != 1)
			die("unable to determine chip number");

		ctx->chips[detected++] = chip;
		udev_device_unref(dev);
	}

	udev_monitor_unref(monitor);
	udev_unref(udev_ctx);

	/*
	 * We can't assume that the order in which the mockup gpiochip
	 * devices are created will be deterministic, yet we want the
	 * index passed to the gu_chip_*() functions to correspond with the
	 * order in which the chips were defined in the GU_DEFINE_TEST()
	 * macro.
	 *
	 * Once all gpiochips are there, sort them by name.
	 */
	qsort(ctx->chips, ctx->num_chips, sizeof(*ctx->chips), chipcmp);
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
	struct _gu_test *test;

	atexit(module_cleanup);

	msg("libgpiod unit-test suite");
	msg("%u tests registered", globals.num_tests);

	check_gpio_mockup();

	msg("running tests");

	for (test = globals.test_list_head; test; test = test->_next) {
		test_prepare(&test->chip_descr);

		test->func();
		msg("test '%s': %s", test->name,
		       globals.test_ctx.test_failed ? "FAILED" : "OK");
		if (globals.test_ctx.test_failed)
			globals.tests_failed++;

		test_teardown();
	}

	if (!globals.tests_failed)
		msg("all tests passed");
	else
		err("%u out of %u tests failed",
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

void gu_free_chip_iter_noclose(struct gpiod_chip_iter **iter)
{
	if (*iter)
		gpiod_chip_iter_free_noclose(*iter);
}

void gu_release_line(struct gpiod_line **line)
{
	if (*line)
		gpiod_line_release(*line);
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

void _gu_register_test(struct _gu_test *test)
{
	struct _gu_test *tmp;

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

void _gu_test_failed(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg(stderr, "ERROR", fmt, va);
	va_end(va);

	globals.test_ctx.test_failed = true;
}
