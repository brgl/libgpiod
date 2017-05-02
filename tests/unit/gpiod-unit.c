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
#include <pthread.h>
#include <sys/time.h>
#include <sys/utsname.h>

#define NORETURN	__attribute__((noreturn))
#define MALLOC		__attribute__((malloc))

static const char mockup_devpath[] = "/devices/platform/gpio-mockup/gpiochip";

struct mockup_chip {
	char *path;
	char *name;
	unsigned int number;
};

struct event_thread {
	pthread_t thread_id;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	bool running;
	bool should_stop;
	unsigned int chip_index;
	unsigned int line_offset;
	unsigned int freq;
	int event_type;
};

struct test_context {
	struct mockup_chip **chips;
	size_t num_chips;
	bool test_failed;
	char *failed_msg;
	struct event_thread event;
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

enum {
	CNORM = 0,
	CGREEN,
	CRED,
	CREDBOLD,
	CYELLOW,
};

static const char *term_colors[] = {
	"\033[0m",
	"\033[32m",
	"\033[31m",
	"\033[1m\033[31m",
	"\033[33m",
};

static void set_color(int color)
{
	fprintf(stderr, "%s", term_colors[color]);
}

static void reset_color(void)
{
	fprintf(stderr, "%s", term_colors[CNORM]);
}

static void pr_raw_v(const char *fmt, va_list va)
{
	vfprintf(stderr, fmt, va);
}

static GU_PRINTF(1, 2) void pr_raw(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	pr_raw_v(fmt, va);
	va_end(va);
}

static void print_header(const char *hdr, int color)
{
	set_color(color);
	pr_raw("[%-5s] ", hdr);
	reset_color();
}

static void vmsgn(const char *hdr, int hdr_clr, const char *fmt, va_list va)
{
	print_header(hdr, hdr_clr);
	pr_raw_v(fmt, va);
}

static void vmsg(const char *hdr, int hdr_clr, const char *fmt, va_list va)
{
	vmsgn(hdr, hdr_clr, fmt, va);
	pr_raw("\n");
}

static GU_PRINTF(1, 2) void msg(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg("INFO", CGREEN, fmt, va);
	va_end(va);
}

static GU_PRINTF(1, 2) void err(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg("ERROR", CRED, fmt, va);
	va_end(va);
}

static GU_PRINTF(1, 2) NORETURN void die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsg("FATAL", CRED, fmt, va);
	va_end(va);

	exit(EXIT_FAILURE);
}

static GU_PRINTF(1, 2) NORETURN void die_perr(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vmsgn("FATAL", CRED, fmt, va);
	pr_raw(": %s\n", strerror(errno));
	va_end(va);

	exit(EXIT_FAILURE);
}

static MALLOC void * xzalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr)
		die("out of memory");

	memset(ptr, 0, size);

	return ptr;
}

static MALLOC char * xstrdup(const char *str)
{
	char *ret;

	ret = strdup(str);
	if (!ret)
		die("out of memory");

	return ret;
}

static MALLOC GU_PRINTF(2, 3) char * xappend(char *str, const char *fmt, ...)
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

static void event_lock(void)
{
	pthread_mutex_lock(&globals.test_ctx.event.lock);
}

static void event_unlock(void)
{
	pthread_mutex_unlock(&globals.test_ctx.event.lock);
}

static void * event_worker(void *data GU_UNUSED)
{
	struct event_thread *ev = &globals.test_ctx.event;
	struct timeval tv_now, tv_add, tv_res;
	struct timespec ts;
	int status, i, fd;
	char *path;
	ssize_t rd;
	char buf;

	for (i = 0;; i++) {
		event_lock();
		if (ev->should_stop) {
			event_unlock();
			break;
		}

		gettimeofday(&tv_now, NULL);
		tv_add.tv_sec = 0;
		tv_add.tv_usec = ev->freq * 1000;
		timeradd(&tv_now, &tv_add, &tv_res);
		ts.tv_sec = tv_res.tv_sec;
		ts.tv_nsec = tv_res.tv_usec * 1000;

		status = pthread_cond_timedwait(&ev->cond, &ev->lock, &ts);
		if (status != 0 && status != ETIMEDOUT)
			die("error waiting for conditional variable: %s",
			    strerror(status));

		path = xappend(NULL,
			       "/sys/kernel/debug/gpio-mockup-event/gpio-mockup-%c/%u",
			       'A' + ev->chip_index, ev->line_offset);

		fd = open(path, O_RDWR);
		free(path);
		if (fd < 0)
			die_perr("error opening gpio event file");

		if (ev->event_type == GU_EVENT_RISING)
			buf = '1';
		else if (ev->event_type == GU_EVENT_FALLING)
			buf = '0';
		else
			buf = i % 2 == 0 ? '1' : '0';

		rd = write(fd, &buf, 1);
		close(fd);
		if (rd < 0)
			die_perr("error writing to gpio event file");
		else if (rd != 1)
			die("invalid write size to gpio event file");

		event_unlock();
	}

	return NULL;
}

static void check_kernel(void)
{
	int status, version, patchlevel;
	struct utsname un;

	msg("checking the linux kernel version");

	status = uname(&un);
	if (status)
		die_perr("uname");

	status = sscanf(un.release, "%d.%d", &version, &patchlevel);
	if (status != 2)
		die("error reading kernel release version");

	if (version < 4 || patchlevel < 11)
		die("linux kernel version must be at least v4.11 - got v%d.%d",
		    version, patchlevel);

	msg("kernel release is v%d.%d - ok to run tests", version, patchlevel);
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

	if (descr->named_lines)
		modarg = xappend(modarg, " gpio_mockup_named_lines");

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

	return chip1->number > chip2->number;
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
	pthread_mutex_init(&ctx->event.lock, NULL);
	pthread_cond_init(&ctx->event.cond, NULL);

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
	 * Once all gpiochips are there, sort them by chip number.
	 */
	qsort(ctx->chips, ctx->num_chips, sizeof(*ctx->chips), chipcmp);
}

static void test_teardown(void)
{
	struct mockup_chip *chip;
	struct event_thread *ev;
	unsigned int i;
	int status;

	event_lock();
	ev = &globals.test_ctx.event;

	if (ev->running) {
		ev->should_stop = true;
		pthread_cond_broadcast(&ev->cond);
		event_unlock();

		status = pthread_join(globals.test_ctx.event.thread_id, NULL);
		if (status != 0)
			die("error joining event thread: %s",
			    strerror(status));

		pthread_mutex_destroy(&globals.test_ctx.event.lock);
		pthread_cond_destroy(&globals.test_ctx.event.cond);
	} else {
		event_unlock();
	}

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

	check_kernel();
	check_gpio_mockup();

	msg("running tests");

	for (test = globals.test_list_head; test; test = test->_next) {
		test_prepare(&test->chip_descr);

		print_header("TEST", CYELLOW);
		pr_raw("'%s': ", test->name);

		test->func();

		if (globals.test_ctx.test_failed) {
			globals.tests_failed++;
			set_color(CREDBOLD);
			pr_raw("FAILED:");
			reset_color();
			set_color(CRED);
			pr_raw("\n\t\t'%s': %s\n",
			       test->name, globals.test_ctx.failed_msg);
			reset_color();
			free(globals.test_ctx.failed_msg);
		} else {
			set_color(CGREEN);
			pr_raw("OK\n");
			reset_color();
		}

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

GU_PRINTF(1, 2) void _gu_test_failed(const char *fmt, ...)
{
	int status;
	va_list va;

	va_start(va, fmt);
	status = vasprintf(&globals.test_ctx.failed_msg, fmt, va);
	va_end(va);
	if (status < 0)
		die_perr("vasprintf");

	globals.test_ctx.test_failed = true;
}

void gu_set_event(unsigned int chip_index,
		  unsigned int line_offset, int event_type, unsigned int freq)
{
	struct event_thread *ev = &globals.test_ctx.event;
	int status;

	event_lock();

	if (!ev->running) {
		status = pthread_create(&ev->thread_id, NULL,
					event_worker, NULL);
		if (status != 0)
			die("error creating event thread: %s",
			    strerror(status));

		ev->running = true;
	}

	ev->chip_index = chip_index;
	ev->line_offset = line_offset;
	ev->event_type = event_type;
	ev->freq = freq;

	pthread_cond_broadcast(&ev->cond);

	event_unlock();
}
