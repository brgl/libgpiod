// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <errno.h>
#include <glib/gstdio.h>
#include <gpiosim.h>
#include <linux/version.h>
#include <stdio.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "gpiod-test.h"

#define MIN_KERNEL_MAJOR	5
#define MIN_KERNEL_MINOR	10
#define MIN_KERNEL_RELEASE	0
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
	guint period_ms;
};

static struct {
	GList *tests;
	struct gpiosim_ctx *gpiosim;
	GPtrArray *sim_chips;
	GPtrArray *sim_banks;
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

static void remove_gpiosim_chip(gpointer data)
{
	struct gpiosim_dev *dev = data;
	gint ret;

	ret = gpiosim_dev_disable(dev);
	if (ret)
		g_error("unable to uncommit a simulated GPIO device: %s",
			g_strerror(errno));

	gpiosim_dev_unref(dev);
}

static void remove_gpiosim_bank(gpointer data)
{
	struct gpiosim_bank *bank = data;

	gpiosim_bank_unref(bank);
}

static void test_func_wrapper(gconstpointer data)
{
	const _GpiodTestCase *test = data;
	struct gpiosim_bank *sim_bank;
	struct gpiosim_dev *sim_dev;
	gchar *line_name, *label;
	gchar chip_idx;
	guint i, j;
	gint ret;

	globals.sim_chips = g_ptr_array_new_full(test->num_chips,
						 remove_gpiosim_chip);
	globals.sim_banks = g_ptr_array_new_full(test->num_chips,
						 remove_gpiosim_bank);

	for (i = 0; i < test->num_chips; i++) {
		chip_idx = i + 65;

		sim_dev = gpiosim_dev_new(globals.gpiosim, NULL);
		if (!sim_dev)
			g_error("unable to create a simulated GPIO chip: %s",
				g_strerror(errno));

		sim_bank = gpiosim_bank_new(sim_dev, NULL);
		if (!sim_bank)
			g_error("unable to create a simulated GPIO bank: %s",
				g_strerror(errno));

		label = g_strdup_printf("gpio-mockup-%c", chip_idx);
		ret = gpiosim_bank_set_label(sim_bank, label);
		g_free(label);
		if (ret)
			g_error("unable to set simulated chip label: %s",
				g_strerror(errno));

		ret = gpiosim_bank_set_num_lines(sim_bank, test->chip_sizes[i]);
		if (ret)
			g_error("unable to set the number of lines for a simulated chip: %s",
				g_strerror(errno));

		if (test->flags & GPIOD_TEST_FLAG_NAMED_LINES) {
			for (j = 0; j < test->chip_sizes[i]; j++) {
				line_name = g_strdup_printf("gpio-mockup-%c-%u",
							    chip_idx, j);

				ret = gpiosim_bank_set_line_name(sim_bank, j,
								 line_name);
				g_free(line_name);
				if (ret)
					g_error("unable to set the line names for a simulated bank: %s",
						g_strerror(errno));
			}
		}

		ret = gpiosim_dev_enable(sim_dev);
		if (ret)
			g_error("unable to commit the simulated GPIO device: %s",
				g_strerror(errno));

		g_ptr_array_add(globals.sim_chips, sim_dev);
		g_ptr_array_add(globals.sim_banks, sim_bank);
	}

	test->func();

	g_ptr_array_unref(globals.sim_banks);
	g_ptr_array_unref(globals.sim_chips);
}

static void unref_gpiosim(void)
{
	gpiosim_ctx_unref(globals.gpiosim);
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
	 * Setup libpiosim first so that it runs its own kernel version
	 * check before we tell the user our local requirements are met as
	 * well.
	 */
	globals.gpiosim = gpiosim_ctx_new();
	if (!globals.gpiosim)
		g_error("unable to initialize gpiosim library: %s",
			g_strerror(errno));
	atexit(unref_gpiosim);

	check_kernel();

	g_list_foreach(globals.tests, add_test_from_list, NULL);
	g_list_free(globals.tests);

	return g_test_run();
}

void _gpiod_test_register(_GpiodTestCase *test)
{
	globals.tests = g_list_append(globals.tests, test);
}

const gchar *gpiod_test_chip_path(guint idx)
{
	struct gpiosim_bank *bank = g_ptr_array_index(globals.sim_banks, idx);

	return gpiosim_bank_get_dev_path(bank);
}

const gchar *gpiod_test_chip_name(guint idx)
{
	struct gpiosim_bank *bank = g_ptr_array_index(globals.sim_banks, idx);

	return gpiosim_bank_get_chip_name(bank);
}

gint gpiod_test_chip_get_value(guint chip_index, guint line_offset)
{
	struct gpiosim_bank *bank = g_ptr_array_index(globals.sim_banks,
						      chip_index);
	gint ret;

	ret = gpiosim_bank_get_value(bank, line_offset);
	if (ret < 0)
		g_error("unable to read line value from gpiosim: %s",
			g_strerror(errno));

	return ret;
}

void gpiod_test_chip_set_pull(guint chip_index, guint line_offset, gint pull)
{
	struct gpiosim_bank *bank = g_ptr_array_index(globals.sim_banks,
						      chip_index);
	gint ret;

	ret = gpiosim_bank_set_pull(bank, line_offset,
				    pull ? GPIOSIM_PULL_UP : GPIOSIM_PULL_DOWN);
	if (ret)
		g_error("unable to set line pull in gpiosim: %s",
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

		end_time = g_get_monotonic_time() + thread->period_ms * 1000;

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
gpiod_test_start_event_thread(guint chip_index, guint line_offset, guint period_ms)
{
	GpiodTestEventThread *thread = g_malloc0(sizeof(*thread));

	g_mutex_init(&thread->lock);
	g_cond_init(&thread->cond);

	thread->chip_index = chip_index;
	thread->line_offset = line_offset;
	thread->period_ms = period_ms;

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
