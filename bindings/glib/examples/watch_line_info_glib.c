// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

/* Minimal example of watching for requests on particular lines. */

#include <glib.h>
#include <gpiod-glib.h>
#include <stdlib.h>

static void on_info_event(GpiodglibChip *chip G_GNUC_UNUSED,
			  GpiodglibInfoEvent *event,
			  gpointer data G_GNUC_UNUSED)
{
	g_autoptr(GpiodglibLineInfo) info = NULL;
	g_autoptr(GString) event_name = NULL;
	guint offset;

	event_name = g_string_new(
			g_enum_to_string(GPIODGLIB_INFO_EVENT_TYPE_TYPE,
				gpiodglib_info_event_get_event_type(event)));
	g_string_replace(event_name, "GPIODGLIB_INFO_EVENT_LINE_", "", 0);
	info = gpiodglib_info_event_get_line_info(event);
	offset = gpiodglib_line_info_get_offset(info);

	g_print("%s %u\n", event_name->str, offset);
}

int main(void)
{
	static const gchar *const chip_path = "/dev/gpiochip1";
	static const guint line_offsets[] = { 5, 3, 7 };
	static const gsize num_lines = 3;

	g_autoptr(GMainLoop) loop = NULL;
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	guint i;

	chip = gpiodglib_chip_new(chip_path, &err);
	if (!chip) {
		g_printerr("unable to open %s: %s\n", chip_path, err->message);
		return EXIT_FAILURE;
	}

	for (i = 0; i < num_lines; i++) {
		g_autoptr(GpiodglibLineInfo) info =
			gpiodglib_chip_watch_line_info(chip, line_offsets[i],
						       &err);
		if (!info) {
			g_printerr("unable to watch line info for offset %u: %s",
				   line_offsets[1], err->message);
			return EXIT_FAILURE;
		}
	}

	loop = g_main_loop_new(NULL, FALSE);

	g_signal_connect(chip, "info-event", G_CALLBACK(on_info_event), NULL);

	g_main_loop_run(loop);

	return EXIT_SUCCESS;
}
