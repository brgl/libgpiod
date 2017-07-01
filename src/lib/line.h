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

struct line_chip_ctx;

struct line_chip_ctx * line_chip_ctx_new(struct gpiod_chip *chip, int fd);
void line_chip_ctx_free(struct line_chip_ctx *chip_ctx);

struct gpiod_line * line_new(unsigned int offset,
			     struct line_chip_ctx *chip_ctx);
void line_free(struct gpiod_line *line);

#endif /* __GPIOD_INTERNAL_LINE_H__ */
