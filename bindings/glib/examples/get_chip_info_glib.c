// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

/* Minimal example of reading the info for a chip. */

#include <glib.h>
#include <gpiod-glib.h>
#include <stdlib.h>

int main(void)
{
	/* Example configuration - customize to suit your situation. */
	static const gchar *const chip_path = "/dev/gpiochip0";

	g_autoptr(GpiodglibChipInfo) info = NULL;
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autofree gchar *label = NULL;
	g_autofree gchar *name = NULL;
	g_autoptr(GError) err = NULL;

	chip = gpiodglib_chip_new(chip_path, &err);
	if (err) {
		g_printerr("Failed to open the GPIO chip at '%s': %s\n",
			   chip_path, err->message);
		return EXIT_FAILURE;
	}

	info = gpiodglib_chip_get_info(chip, &err);
	if (err) {
		g_printerr("Failed to retrieve GPIO chip info: %s\n",
			   err->message);
		return EXIT_FAILURE;
	}

	name = gpiodglib_chip_info_dup_name(info);
	label = gpiodglib_chip_info_dup_label(info);

	g_print("%s [%s] (%u lines)\n",
		name, label, gpiodglib_chip_info_get_num_lines(info));

	return EXIT_SUCCESS;
}
