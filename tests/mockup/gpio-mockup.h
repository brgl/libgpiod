/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com> */

#ifndef __GPIO_MOCKUP_H__
#define __GPIO_MOCKUP_H__

#ifdef __cplusplus
extern "C" {
#endif

struct gpio_mockup;

#define GPIO_MOCKUP_FLAG_NAMED_LINES	(1 << 0)

struct gpio_mockup *gpio_mockup_new(void);
void gpio_mockup_ref(struct gpio_mockup *ctx);
void gpio_mockup_unref(struct gpio_mockup *ctx);

int gpio_mockup_probe(struct gpio_mockup *ctx, unsigned int num_chips,
		      const unsigned int *chip_sizes, int flags);
int gpio_mockup_remove(struct gpio_mockup *ctx);

const char *gpio_mockup_chip_name(struct gpio_mockup *ctx, unsigned int idx);
const char *gpio_mockup_chip_path(struct gpio_mockup *ctx, unsigned int idx);
int gpio_mockup_chip_num(struct gpio_mockup *ctx, unsigned int idx);

int gpio_mockup_get_value(struct gpio_mockup *ctx,
			  unsigned int chip_idx, unsigned int line_offset);
int gpio_mockup_set_pull(struct gpio_mockup *ctx, unsigned int chip_idx,
			 unsigned int line_offset, int pull);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GPIO_MOCKUP_H__ */
