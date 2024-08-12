// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

/* Minimal example of finding a line with the given name. */

#include <glib.h>
#include <gpiod-glib.h>
#include <stdlib.h>

int main(void)
{
	/* Example configuration - customize to suit your situation. */
	static const gchar *const line_name = "GPIO0";

	g_autoptr(GpiodglibChipInfo) info = NULL;
	g_autoptr(GError) err = NULL;
	g_autoptr(GDir) dir = NULL;
	const gchar *filename;
	gboolean ret;
	guint offset;

	dir = g_dir_open("/dev", 0, &err);
	if (err) {
		g_printerr("Unable to open /dev: %s\n", err->message);
		return EXIT_FAILURE;
	}

	/*
	 * Names are not guaranteed unique, so this finds the first line with
	 * the given name.
	 */
	while ((filename = g_dir_read_name(dir))) {
		g_autoptr(GpiodglibChip) chip = NULL;
		g_autofree gchar *path = NULL;
		g_autofree gchar *name = NULL;

		path = g_build_filename("/dev", filename, NULL);
		if (!gpiodglib_is_gpiochip_device(path))
			continue;

		chip = gpiodglib_chip_new(path, &err);
		if (err) {
			g_printerr("Failed to open the GPIO chip at '%s': %s\n",
				   path, err->message);
			return EXIT_FAILURE;
		}

		ret = gpiodglib_chip_get_line_offset_from_name(chip, line_name,
							       &offset, &err);
		if (!ret) {
			g_printerr("Failed to map the line name '%s' to offset: %s\n",
				   line_name, err->message);
			return EXIT_FAILURE;
		}

		info = gpiodglib_chip_get_info(chip, &err);
		if (!info) {
			g_printerr("Failed to get chip info: %s\n",
				   err->message);
			return EXIT_FAILURE;
		}

		name = gpiodglib_chip_info_dup_name(info);

		g_print("%s %u\n", name, offset);
	}

	g_print("line '%s' not found\n", line_name);

	return EXIT_SUCCESS;
}
