// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include "helpers.h"

GArray *gpiodglib_test_array_from_const(gconstpointer data, gsize len,
					gsize elem_size)
{
	GArray *arr = g_array_new(FALSE, TRUE, elem_size);

	return g_array_append_vals(arr, data, len);
}
