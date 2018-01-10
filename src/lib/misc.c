/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

/* Misc code that didn't fit anywhere else. */

#include <gpiod.h>

const char * gpiod_version_string(void)
{
	return GPIOD_VERSION_STR;
}
