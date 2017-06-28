/*
 * Internal routines for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#ifndef __GPIOD_INTERNAL_H__
#define __GPIOD_INTERNAL_H__

#include <gpiod.h>

struct gpiod_line * line_array_alloc(size_t numlines);
void line_array_free(struct gpiod_line *lines);
struct gpiod_line * line_array_member(struct gpiod_line *lines, size_t index);

void line_set_chip(struct gpiod_line *line, struct gpiod_chip *chip);
void line_set_offset(struct gpiod_line *line, unsigned int offset);

int chip_get_fd(struct gpiod_chip *chip);

#endif /* __GPIOD_INTERNAL_H__ */
