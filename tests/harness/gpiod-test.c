// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <errno.h>
#include <linux/version.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "gpiod-test.h"

#define MIN_KERNEL_MAJOR	6
#define MIN_KERNEL_MINOR	17
#define MIN_KERNEL_RELEASE	0
#define MIN_KERNEL_VERSION	KERNEL_VERSION(MIN_KERNEL_MAJOR, \
					       MIN_KERNEL_MINOR, \
					       MIN_KERNEL_RELEASE)

static GList *tests;

static gboolean check_kernel(void)
{
	guint major, minor, release;
	struct utsname un;
	gint ret;

	g_debug("checking linux kernel version");

	ret = uname(&un);
	if (ret) {
		g_critical("unable to read the kernel release version: %s",
			   g_strerror(errno));
		return FALSE;
	}

	ret = sscanf(un.release, "%u.%u.%u", &major, &minor, &release);
	if (ret != 3) {
		g_critical("error reading kernel release version");
		return FALSE;
	}

	if (KERNEL_VERSION(major, minor, release) < MIN_KERNEL_VERSION) {
		g_critical("linux kernel version must be at least v%u.%u.%u - got v%u.%u.%u",
			   MIN_KERNEL_MAJOR, MIN_KERNEL_MINOR, MIN_KERNEL_RELEASE,
			   major, minor, release);
		return FALSE;
	}

	g_debug("kernel release is v%u.%u.%u - ok to run tests",
		major, minor, release);

	return TRUE;
}

static void test_func_wrapper(gconstpointer data)
{
	const struct _gpiod_test_case *test = data;

	test->func();
}

static void add_test_from_list(gpointer element, gpointer data G_GNUC_UNUSED)
{
	struct _gpiod_test_case *test = element;

	g_test_add_data_func(test->path, test, test_func_wrapper);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_set_nonfatal_assertions();

	g_debug("running libgpiod test suite");
	g_debug("%u tests registered", g_list_length(tests));

	if (!check_kernel())
		return EXIT_FAILURE;

	g_list_foreach(tests, add_test_from_list, NULL);
	g_list_free(tests);

	return g_test_run();
}

void _gpiod_test_register(struct _gpiod_test_case *test)
{
	tests = g_list_append(tests, test);
}
