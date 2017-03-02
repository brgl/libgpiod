/*
 * GPIO chip test cases for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-unit.h"

#include <stdio.h>
#include <errno.h>

static void chip_open_good(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);
}
GU_DEFINE_TEST(chip_open_good,
	       "gpiod_chip_open() - good",
	       GU_LINES_UNNAMED, { 8 });

static void chip_open_nonexistent(void)
{
	struct gpiod_chip *chip;

	chip = gpiod_chip_open("/dev/nonexistent_gpiochip");
	GU_ASSERT_NULL(chip);
	GU_ASSERT_EQ(gpiod_errno(), ENOENT);
}
GU_DEFINE_TEST(chip_open_nonexistent,
	       "gpiod_chip_open() - nonexistent chip",
	       GU_LINES_UNNAMED, { 8 });

static void chip_open_notty(void)
{
	struct gpiod_chip *chip;

	chip = gpiod_chip_open("/dev/null");
	GU_ASSERT_NULL(chip);
	GU_ASSERT_EQ(gpiod_errno(), ENOTTY);
}
GU_DEFINE_TEST(chip_open_notty,
	       "gpiod_chip_open() - notty",
	       GU_LINES_UNNAMED, { 8 });

static void chip_open_by_name_good(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;

	chip = gpiod_chip_open_by_name(gu_chip_name(0));
	GU_ASSERT_NOT_NULL(chip);
}
GU_DEFINE_TEST(chip_open_by_name_good,
	       "gpiod_chip_open_by_name() - good",
	       GU_LINES_UNNAMED, { 8 });

static void chip_open_by_number_good(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;

	chip = gpiod_chip_open_by_number(gu_chip_num(0));
	GU_ASSERT_NOT_NULL(chip);
}
GU_DEFINE_TEST(chip_open_by_number_good,
	       "gpiod_chip_open_by_number() - good",
	       GU_LINES_UNNAMED, { 8 });

static void chip_open_lookup(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip_by_label = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip_by_name = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip_by_path = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip_by_num = NULL;
	GU_CLEANUP(gu_free_str) char *chip_num;

	GU_ASSERT(asprintf(&chip_num, "%u", gu_chip_num(1)) > 0);

	chip_by_name = gpiod_chip_open_lookup(gu_chip_name(1));
	chip_by_path = gpiod_chip_open_lookup(gu_chip_path(1));
	chip_by_num = gpiod_chip_open_lookup(chip_num);
	chip_by_label = gpiod_chip_open_lookup("gpio-mockup-B");

	GU_ASSERT_NOT_NULL(chip_by_name);
	GU_ASSERT_NOT_NULL(chip_by_path);
	GU_ASSERT_NOT_NULL(chip_by_num);
	GU_ASSERT_NOT_NULL(chip_by_label);

	GU_ASSERT_STR_EQ(gpiod_chip_name(chip_by_name), gu_chip_name(1));
	GU_ASSERT_STR_EQ(gpiod_chip_name(chip_by_path), gu_chip_name(1));
	GU_ASSERT_STR_EQ(gpiod_chip_name(chip_by_num), gu_chip_name(1));
	GU_ASSERT_STR_EQ(gpiod_chip_name(chip_by_label), gu_chip_name(1));
}
GU_DEFINE_TEST(chip_open_lookup,
	       "gpiod_chip_open_lookup() - good",
	       GU_LINES_UNNAMED, { 8, 8, 8 });

static void chip_open_by_label_good(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;

	chip = gpiod_chip_open_by_label("gpio-mockup-D");
	GU_ASSERT_NOT_NULL(chip);
	GU_ASSERT_STR_EQ(gpiod_chip_name(chip), gu_chip_name(3));
}
GU_DEFINE_TEST(chip_open_by_label_good,
	       "gpiod_chip_open_by_label() - good",
	       GU_LINES_UNNAMED, { 4, 4, 4, 4, 4 });

static void chip_open_by_label_bad(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;

	chip = gpiod_chip_open_by_label("nonexistent_gpio_chip");
	GU_ASSERT_NULL(chip);
}
GU_DEFINE_TEST(chip_open_by_label_bad,
	       "gpiod_chip_open_by_label() - bad",
	       GU_LINES_UNNAMED, { 4, 4, 4, 4, 4 });

static void chip_name(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip0 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip1 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip2 = NULL;

	chip0 = gpiod_chip_open(gu_chip_path(0));
	chip1 = gpiod_chip_open(gu_chip_path(1));
	chip2 = gpiod_chip_open(gu_chip_path(2));
	GU_ASSERT_NOT_NULL(chip0);
	GU_ASSERT_NOT_NULL(chip1);
	GU_ASSERT_NOT_NULL(chip2);

	GU_ASSERT_STR_EQ(gpiod_chip_name(chip0), gu_chip_name(0));
	GU_ASSERT_STR_EQ(gpiod_chip_name(chip1), gu_chip_name(1));
	GU_ASSERT_STR_EQ(gpiod_chip_name(chip2), gu_chip_name(2));
}
GU_DEFINE_TEST(chip_name,
	       "gpiod_chip_name()",
	       GU_LINES_UNNAMED, { 8, 8, 8 });

static void chip_label(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip0 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip1 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip2 = NULL;

	chip0 = gpiod_chip_open(gu_chip_path(0));
	chip1 = gpiod_chip_open(gu_chip_path(1));
	chip2 = gpiod_chip_open(gu_chip_path(2));
	GU_ASSERT_NOT_NULL(chip0);
	GU_ASSERT_NOT_NULL(chip1);
	GU_ASSERT_NOT_NULL(chip2);

	GU_ASSERT_STR_EQ(gpiod_chip_label(chip0), "gpio-mockup-A");
	GU_ASSERT_STR_EQ(gpiod_chip_label(chip1), "gpio-mockup-B");
	GU_ASSERT_STR_EQ(gpiod_chip_label(chip2), "gpio-mockup-C");
}
GU_DEFINE_TEST(chip_label,
	       "gpiod_chip_label()",
	       GU_LINES_UNNAMED, { 8, 8, 8 });

static void chip_num_lines(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip0 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip1 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip2 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip3 = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip4 = NULL;

	chip0 = gpiod_chip_open(gu_chip_path(0));
	chip1 = gpiod_chip_open(gu_chip_path(1));
	chip2 = gpiod_chip_open(gu_chip_path(2));
	chip3 = gpiod_chip_open(gu_chip_path(3));
	chip4 = gpiod_chip_open(gu_chip_path(4));
	GU_ASSERT_NOT_NULL(chip0);
	GU_ASSERT_NOT_NULL(chip1);
	GU_ASSERT_NOT_NULL(chip2);
	GU_ASSERT_NOT_NULL(chip3);
	GU_ASSERT_NOT_NULL(chip4);

	GU_ASSERT_EQ(gpiod_chip_num_lines(chip0), 1);
	GU_ASSERT_EQ(gpiod_chip_num_lines(chip1), 4);
	GU_ASSERT_EQ(gpiod_chip_num_lines(chip2), 8);
	GU_ASSERT_EQ(gpiod_chip_num_lines(chip3), 16);
	GU_ASSERT_EQ(gpiod_chip_num_lines(chip4), 32);
}
GU_DEFINE_TEST(chip_num_lines,
	       "gpiod_chip_num_lines()",
	       GU_LINES_UNNAMED, { 1, 4, 8, 16, 32 });
