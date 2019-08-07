// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <glib/gstdio.h>
#include <gpio-mockup.h>
#include <linux/version.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "gpiod-test.h"

#define MIN_KERNEL_MAJOR	5
#define MIN_KERNEL_MINOR	2
#define MIN_KERNEL_RELEASE	7
#define MIN_KERNEL_VERSION	KERNEL_VERSION(MIN_KERNEL_MAJOR, \
					       MIN_KERNEL_MINOR, \
					       MIN_KERNEL_RELEASE)

struct gpiod_test_event_thread {
	GThread *id;
	GMutex lock;
	GCond cond;
	gboolean should_stop;
	guint chip_index;
	guint line_offset;
	guint freq;
};

static struct {
	GList *tests;
	struct gpio_mockup *mockup;
} globals;

static void check_kernel(void)
{
	guint major, minor, release;
	struct utsname un;
	gint ret;

	g_debug("checking linux kernel version");

	ret = uname(&un);
	if (ret)
		g_error("unable to read the kernel release version: %s",
			g_strerror(errno));

	ret = sscanf(un.release, "%u.%u.%u", &major, &minor, &release);
	if (ret != 3)
		g_error("error reading kernel release version");

	if (KERNEL_VERSION(major, minor, release) < MIN_KERNEL_VERSION)
		g_error("linux kernel version must be at least v%u.%u.%u - got v%u.%u.%u",
			MIN_KERNEL_MAJOR, MIN_KERNEL_MINOR, MIN_KERNEL_RELEASE,
			major, minor, release);

	g_debug("kernel release is v%u.%u.%u - ok to run tests",
		major, minor, release);

	return;
}

static void test_func_wrapper(gconstpointer data)
{
	const _GpiodTestCase *test = data;
	gint ret, flags = 0;

	if (test->flags & GPIOD_TEST_FLAG_NAMED_LINES)
		flags |= GPIO_MOCKUP_FLAG_NAMED_LINES;

	ret = gpio_mockup_probe(globals.mockup, test->num_chips,
				test->chip_sizes, flags);
	if (ret)
		g_error("unable to probe gpio-mockup: %s", g_strerror(errno));

	test->func();

	ret = gpio_mockup_remove(globals.mockup);
	if (ret)
		g_error("unable to remove gpio_mockup: %s", g_strerror(errno));
}

static void unref_mockup(void)
{
	gpio_mockup_unref(globals.mockup);
}

static void add_test_from_list(gpointer element, gpointer data G_GNUC_UNUSED)
{
	_GpiodTestCase *test = element;

	g_test_add_data_func(test->path, test, test_func_wrapper);
}

int main(gint argc, gchar **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_debug("running libgpiod test suite");
	g_debug("%u tests registered", g_list_length(globals.tests));

	/*
	 * Setup libgpiomockup first so that it runs its own kernel version
	 * check before we tell the user our local requirements are met as
	 * well.
	 */
	globals.mockup = gpio_mockup_new();
	if (!globals.mockup)
		g_error("unable to initialize gpio-mockup library: %s",
			g_strerror(errno));
	atexit(unref_mockup);

	check_kernel();

	g_list_foreach(globals.tests, add_test_from_list, NULL);
	g_list_free(globals.tests);

	return g_test_run();
}

void _gpiod_test_register(_GpiodTestCase *test)
{
	globals.tests = g_list_append(globals.tests, test);
}

const gchar *gpiod_test_chip_path(guint index)
{
	const gchar *path;

	path = gpio_mockup_chip_path(globals.mockup, index);
	if (!path)
		g_error("unable to retrieve the chip path: %s",
			g_strerror(errno));

	return path;
}

const gchar *gpiod_test_chip_name(guint index)
{
	const gchar *name;

	name = gpio_mockup_chip_name(globals.mockup, index);
	if (!name)
		g_error("unable to retrieve the chip name: %s",
			g_strerror(errno));

	return name;
}

gint gpiod_test_chip_num(unsigned int index)
{
	gint num;

	num = gpio_mockup_chip_num(globals.mockup, index);
	if (num < 0)
		g_error("unable to retrieve the chip number: %s",
			g_strerror(errno));

	return num;
}

gint gpiod_test_chip_get_value(guint chip_index, guint line_offset)
{
	gint ret;

	ret = gpio_mockup_get_value(globals.mockup, chip_index, line_offset);
	if (ret < 0)
		g_error("unable to read line value from gpio-mockup: %s",
			g_strerror(errno));

	return ret;
}

void gpiod_test_chip_set_pull(guint chip_index, guint line_offset, gint pull)
{
	gint ret;

	ret = gpio_mockup_set_pull(globals.mockup, chip_index,
				   line_offset, pull);
	if (ret)
		g_error("unable to set line pull in gpio-mockup: %s",
			g_strerror(errno));
}

static gpointer event_worker_func(gpointer data)
{
	GpiodTestEventThread *thread = data;
	gboolean signalled;
	gint64 end_time;
	gint i;

	for (i = 0;; i++) {
		g_mutex_lock(&thread->lock);
		if (thread->should_stop) {
			g_mutex_unlock(&thread->lock);
			break;
		}

		end_time = g_get_monotonic_time() + thread->freq * 1000;

		signalled = g_cond_wait_until(&thread->cond,
					      &thread->lock, end_time);
		if (!signalled)
			gpiod_test_chip_set_pull(thread->chip_index,
						 thread->line_offset, i % 2);

		g_mutex_unlock(&thread->lock);
	}

	return NULL;
}

GpiodTestEventThread *
gpiod_test_start_event_thread(guint chip_index, guint line_offset, guint freq)
{
	GpiodTestEventThread *thread = g_malloc0(sizeof(*thread));

	g_mutex_init(&thread->lock);
	g_cond_init(&thread->cond);

	thread->chip_index = chip_index;
	thread->line_offset = line_offset;
	thread->freq = freq;

	thread->id = g_thread_new("event-worker", event_worker_func, thread);

	return thread;
}

void gpiod_test_stop_event_thread(GpiodTestEventThread *thread)
{
	g_mutex_lock(&thread->lock);
	thread->should_stop = TRUE;
	g_cond_broadcast(&thread->cond);
	g_mutex_unlock(&thread->lock);

	(void)g_thread_join(thread->id);

	g_mutex_clear(&thread->lock);
	g_cond_clear(&thread->cond);
	g_free(thread);
}
