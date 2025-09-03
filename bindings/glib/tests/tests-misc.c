// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <glib.h>
#include <gpiod-glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiosim-glib.h>

#define GPIOD_TEST_GROUP "glib/misc"

GPIOD_TEST_CASE(is_gpiochip_bad)
{
	g_assert_false(gpiodglib_is_gpiochip_device("/dev/null"));
	g_assert_false(gpiodglib_is_gpiochip_device("/dev/nonexistent"));
}

GPIOD_TEST_CASE(is_gpiochip_good)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);

	g_assert_true(gpiodglib_is_gpiochip_device(
			g_gpiosim_chip_get_dev_path(sim)));
}

GPIOD_TEST_CASE(is_gpiochip_link_bad)
{
	g_autofree gchar *link = NULL;
	gint ret;

	link = g_strdup_printf("/tmp/gpiod-test-link.%u", getpid());
	ret = symlink("/dev/null", link);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_false(gpiodglib_is_gpiochip_device(link));
	ret = unlink(link);
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(is_gpiochip_link_good)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autofree gchar *link = NULL;
	gint ret;

	link = g_strdup_printf("/tmp/gpiod-test-link.%u", getpid());
	ret = symlink(g_gpiosim_chip_get_dev_path(sim), link);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiodglib_is_gpiochip_device(link));
	ret = unlink(link);
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(version_string)
{
	static const gchar *const pattern = "^\\d+\\.\\d+\\.\\d+";

	g_autoptr(GError) err = NULL;
	g_autoptr(GRegex) regex = NULL;
	g_autoptr(GMatchInfo) match = NULL;
	g_autofree gchar *res = NULL;
	const gchar *ver;
	gboolean ret;

	ver = gpiodglib_api_version();
	g_assert_nonnull(ver);
	gpiod_test_return_if_failed();

	regex = g_regex_new(pattern, 0, 0, &err);
	g_assert_nonnull(regex);
	g_assert_no_error(err);
	gpiod_test_return_if_failed();

	ret = g_regex_match(regex, ver, 0, &match);
	g_assert_true(ret);
	gpiod_test_return_if_failed();

	g_assert_true(g_match_info_matches(match));
	res = g_match_info_fetch(match, 0);
	g_assert_nonnull(res);
	g_assert_cmpstr(res, ==, ver);
	g_match_info_next(match, &err);
	g_assert_no_error(err);
	g_assert_false(g_match_info_matches(match));
}
