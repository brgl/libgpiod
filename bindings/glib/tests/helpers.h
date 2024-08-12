/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_TEST_HELPERS_H__
#define __GPIODGLIB_TEST_HELPERS_H__

#include <glib.h>
#include <gpiod-test-common.h>

#define gpiodglib_test_new_chip_or_fail(_path) \
	({ \
		g_autoptr(GError) _err = NULL; \
		GpiodglibChip *_chip = gpiodglib_chip_new(_path, &_err); \
		g_assert_nonnull(_chip); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
		_chip; \
	})

#define gpiodglib_test_chip_get_info_or_fail(_chip) \
	({ \
		g_autoptr(GError) _err = NULL; \
		GpiodglibChipInfo *_info = gpiodglib_chip_get_info(_chip, \
								   &_err); \
		g_assert_nonnull(_info); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
		_info; \
	})

#define gpiodglib_test_chip_get_line_info_or_fail(_chip, _offset) \
	({ \
		g_autoptr(GError) _err = NULL; \
		GpiodglibLineInfo *_info = \
			gpiodglib_chip_get_line_info(_chip, _offset, &_err); \
		g_assert_nonnull(_info); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
		_info; \
	})

#define gpiodglib_test_chip_watch_line_info_or_fail(_chip, _offset) \
	({ \
		g_autoptr(GError) _err = NULL; \
		GpiodglibLineInfo *_info = \
			gpiodglib_chip_watch_line_info(_chip, _offset, \
						       &_err); \
		g_assert_nonnull(_info); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
		_info; \
	})

#define gpiodglib_test_chip_unwatch_line_info_or_fail(_chip, _offset) \
	do { \
		g_autoptr(GError) _err = NULL; \
		gboolean ret = gpiodglib_chip_unwatch_line_info(_chip, \
								_offset, \
								&_err); \
		g_assert_true(ret); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
	} while (0)

#define gpiodglib_test_line_config_add_line_settings_or_fail(_config, \
							     _offsets, \
							     _settings) \
	do { \
		g_autoptr(GError) _err = NULL; \
		gboolean _ret = \
			gpiodglib_line_config_add_line_settings(_config, \
								_offsets,\
								_settings, \
								&_err); \
		g_assert_true(_ret); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
	} while (0)

#define gpiodglib_test_line_config_get_line_settings_or_fail(_config, \
							     _offset) \
	({ \
		GpiodglibLineSettings *_settings = \
			gpiodglib_line_config_get_line_settings(_config, \
								_offset); \
		g_assert_nonnull(_settings); \
		gpiod_test_return_if_failed(); \
		_settings; \
	})

#define gpiodglib_test_line_config_set_output_values_or_fail(_config, \
							     _values) \
	do { \
		g_autoptr(GError) _err = NULL; \
		gboolean _ret = \
			gpiodglib_line_config_set_output_values(_config, \
								_values, \
								&_err); \
		g_assert_true(_ret); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
	} while (0)

#define gpiodglib_test_chip_request_lines_or_fail(_chip, _req_cfg, _line_cfg) \
	({ \
		g_autoptr(GError) _err = NULL; \
		GpiodglibLineRequest *_req = \
			gpiodglib_chip_request_lines(_chip, _req_cfg, \
						     _line_cfg, &_err); \
		g_assert_nonnull(_req); \
		g_assert_no_error(_err); \
		gpiod_test_return_if_failed(); \
		_req; \
	})

#define gpiodglib_test_request_lines_or_fail(_path, _req_cfg, _line_cfg) \
	({ \
		g_autoptr(GpiodglibChip) _chip = \
			gpiodglib_test_new_chip_or_fail(_path); \
		GpiodglibLineRequest *_req = \
			gpiodglib_test_chip_request_lines_or_fail(_chip, \
								  _req_cfg, \
								  _line_cfg); \
		_req; \
	})

#define gpiodglib_test_check_error_or_fail(_err, _domain, _code) \
	do { \
		g_assert_nonnull(_err); \
		gpiod_test_return_if_failed(); \
		g_assert_cmpint(_domain, ==, (_err)->domain); \
		g_assert_cmpint(_code, ==, (_err)->code); \
		gpiod_test_return_if_failed(); \
	} while (0)

GArray *gpiodglib_test_array_from_const(const gconstpointer data, gsize len,
					 gsize elem_size);

#endif /* __GPIODGLIB_TEST_HELPERS_H__ */

