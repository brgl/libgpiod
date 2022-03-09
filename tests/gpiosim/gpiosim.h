/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __GPIOD_GPIOSIM_H__
#define __GPIOD_GPIOSIM_H__

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpiosim_ctx;
struct gpiosim_dev;
struct gpiosim_bank;

enum {
	GPIOSIM_PULL_DOWN = 1,
	GPIOSIM_PULL_UP,
};

enum {
	GPIOSIM_HOG_DIR_INPUT = 1,
	GPIOSIM_HOG_DIR_OUTPUT_HIGH,
	GPIOSIM_HOG_DIR_OUTPUT_LOW,
};

struct gpiosim_ctx *gpiosim_ctx_new(void);
struct gpiosim_ctx *gpiosim_ctx_ref(struct gpiosim_ctx *ctx);
void gpiosim_ctx_unref(struct gpiosim_ctx *ctx);

struct gpiosim_dev *
gpiosim_dev_new(struct gpiosim_ctx *ctx, const char *name);
struct gpiosim_dev *gpiosim_dev_ref(struct gpiosim_dev *dev);
void gpiosim_dev_unref(struct gpiosim_dev *dev);
struct gpiosim_ctx *gpiosim_dev_get_ctx(struct gpiosim_dev *dev);
const char *gpiosim_dev_get_name(struct gpiosim_dev *dev);

int gpiosim_dev_enable(struct gpiosim_dev *dev);
int gpiosim_dev_disable(struct gpiosim_dev *dev);
bool gpiosim_dev_is_live(struct gpiosim_dev *dev);

struct gpiosim_bank*
gpiosim_bank_new(struct gpiosim_dev *dev, const char *name);
struct gpiosim_bank *gpiosim_bank_ref(struct gpiosim_bank *bank);
void gpiosim_bank_unref(struct gpiosim_bank *bank);
struct gpiosim_dev *gpiosim_bank_get_dev(struct gpiosim_bank *bank);
const char *gpiosim_bank_get_chip_name(struct gpiosim_bank *bank);
const char *gpiosim_bank_get_dev_path(struct gpiosim_bank *bank);

int gpiosim_bank_set_label(struct gpiosim_bank *bank, const char *label);
int gpiosim_bank_set_num_lines(struct gpiosim_bank *bank, size_t num_lines);
int gpiosim_bank_set_line_name(struct gpiosim_bank *bank,
			       unsigned int offset, const char *name);
int gpiosim_bank_hog_line(struct gpiosim_bank *bank, unsigned int offset,
			  const char *name, int direction);
int gpiosim_bank_clear_hog(struct gpiosim_bank *bank, unsigned int offset);

int gpiosim_bank_get_value(struct gpiosim_bank *bank, unsigned int offset);
int gpiosim_bank_get_pull(struct gpiosim_bank *bank, unsigned int offset);
int gpiosim_bank_set_pull(struct gpiosim_bank *bank,
			  unsigned int offset, int pull);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GPIOD_GPIOSIM_H__ */
