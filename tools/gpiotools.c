// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>
// SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>

#include <ctype.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpiotools.h"

#define GT_API	__attribute__((visibility("default")))

static bool isuint(const char *str)
{
	for (; *str && isdigit(*str); str++)
		;

	return *str == '\0';
}

GT_API bool gpiotools_chip_path_lookup(const char *id, char **path_ptr)
{
	char *path;

	if (isuint(id)) {
		/* by number */
		if (asprintf(&path, "/dev/gpiochip%s", id) < 0)
			return false;
	} else if (strchr(id, '/')) {
		/* by path */
		if (asprintf(&path, "%s", id) < 0)
			return false;
	} else {
		/* by device name */
		if (asprintf(&path, "/dev/%s", id) < 0)
			return false;
	}

	if (!gpiod_is_gpiochip_device(path)) {
		free(path);
		return false;
	}

	*path_ptr = path;

	return true;
}
