/*
 * List all lines of a GPIO chip.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include <gpiod.h>

#include <stdio.h>
#include <string.h>

/* REVISIT This program needs a nicer, better formatted output. */

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*(x)))

struct flag {
	const char *name;
	bool (*is_set)(struct gpiod_line *);
};

static const struct flag flags[] = {
	{
		.name = "kernel",
		.is_set = gpiod_line_is_used_by_kernel,
	},
	{
		.name = "open-drain",
		.is_set = gpiod_line_is_open_drain,
	},
	{
		.name = "open-source",
		.is_set = gpiod_line_is_open_source,
	},
};

int main(int argc, char **argv)
{
	int i, direction, flag_printed, polarity;
	struct gpiod_line_iter iter;
	const char *name, *consumer;
	struct gpiod_line *line;
	struct gpiod_chip *chip;
	unsigned int j;

	for (i = 1; i < argc; i++) {
		chip = gpiod_chip_open_lookup(argv[i]);
		if (!chip) {
			fprintf(stderr,
				"%s: unable to access %s: %s\n",
				argv[0], argv[i],
				gpiod_strerror(gpiod_errno()));
			continue;
		}

		printf("%s - %u lines:\n",
		       gpiod_chip_name(chip), gpiod_chip_num_lines(chip));

		gpiod_line_iter_init(&iter);
		gpiod_chip_foreach_line(&iter, chip, line) {
			name = gpiod_line_name(line);
			consumer = gpiod_line_consumer(line);
			direction = gpiod_line_direction(line);
			polarity = gpiod_line_polarity(line);

			printf("\tline %2u: ", gpiod_line_offset(line));

			if (name)
				printf("\"%s\"", name);
			else
				printf("unnamed");
			printf(" ");

			if (consumer)
				printf("\"%s\"", consumer);
			else
				printf("unused");
			printf(" ");

			printf("%s ", direction == GPIOD_DIRECTION_INPUT
							? "input" : "output");
			printf("%s ", polarity == GPIOD_POLARITY_ACTIVE_LOW
							? "active-low"
							: "active-high");

			flag_printed = false;
			for (j = 0; j < ARRAY_SIZE(flags); j++) {
				if (flags[j].is_set(line)) {
					if (flag_printed)
						printf(" ");
					else
						printf("[");
					printf("%s", flags[j].name);
					flag_printed = 1;
				}
			}
			if (flag_printed)
				printf("]");

			printf("\n");
		}

		gpiod_chip_close(chip);
	}

	return EXIT_SUCCESS;
}
