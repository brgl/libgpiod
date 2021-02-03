// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* Misc code that didn't fit anywhere else. */

#include <gpiod.h>

const char *gpiod_version_string(void)
{
	return GPIOD_VERSION_STR;
}
