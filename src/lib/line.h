/*
 * Internal GPIO line-related prototypes.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#ifndef __GPIOD_INTERNAL_LINE_H__
#define __GPIOD_INTERNAL_LINE_H__

#include <gpiod.h>

struct gpiod_line *
line_new(unsigned int offset, struct gpiod_chip *chip, int info_fd);
void line_free(struct gpiod_line *line);

#endif /* __GPIOD_INTERNAL_LINE_H__ */
