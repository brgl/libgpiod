/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Misc code that didn't fit anywhere else. */

#include <gpiod.h>

const char *gpiod_version_string(void)
{
	return GPIOD_VERSION_STR;
}
