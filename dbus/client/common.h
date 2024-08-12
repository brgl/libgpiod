/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIOCLI_COMMON_H__
#define __GPIOCLI_COMMON_H__

#include <gio/gio.h>
#include <glib.h>
#include <gpiodbus.h>

void die(const gchar *fmt, ...) G_NORETURN G_GNUC_PRINTF(1, 2);
void
die_gerror(GError *err, const gchar *fmt, ...) G_NORETURN G_GNUC_PRINTF(2, 3);
void die_parsing_opts(const char *fmt, ...) G_NORETURN G_GNUC_PRINTF(1, 2);

void parse_options(const GOptionEntry *opts, const gchar *summary,
		   const gchar *description, int *argc, char ***argv);
void check_manager(void);

gboolean quit_main_loop_on_signal(gpointer user_data);
void die_on_name_vanished(GDBusConnection *con, const gchar *name,
			  gpointer user_data);

GList *strv_to_gstring_list(GStrv lines);
gint output_value_from_str(const gchar *value_str);

GDBusObjectManager *get_object_manager_client(const gchar *obj_path);
GpiodbusObject *get_chip_obj_by_path(const gchar *obj_path);
GpiodbusObject *get_chip_obj(const gchar *chip_name);
GList *get_chip_objs(GStrv chip_names);
gchar *make_request_obj_path(const gchar *request);
GpiodbusObject *get_request_obj(const gchar *request_name);
GList *get_request_objs(void);
GArray *get_request_offsets(GpiodbusRequest *request);
gboolean get_line_obj_by_name(const gchar *name, GpiodbusObject **line_obj,
			      GpiodbusObject **chip_obj);
GpiodbusObject *
get_line_obj_by_name_for_chip(GpiodbusObject *chip_obj, const gchar *name_line);
GList *get_all_line_objs_for_chip(GpiodbusObject *chip_obj);

gchar *sanitize_object_path(const gchar *path);

typedef struct {
	gchar *name;
	guint offset;
	gboolean used;
	gchar *consumer;
	const gchar *direction;
	const gchar *drive;
	const gchar *bias;
	gboolean active_low;
	const gchar *edge;
	gboolean debounced;
	guint64 debounce_period;
	const gchar *event_clock;
	gboolean managed;
	gchar *request_name;
} LineProperties;

LineProperties *get_line_properties(GpiodbusLine *line);
void free_line_properties(LineProperties *props);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(LineProperties, free_line_properties);

typedef struct {
	gboolean input;
	gboolean output;
	gboolean active_low;
	gboolean rising_edge;
	gboolean falling_edge;
	gboolean both_edges;
	gboolean push_pull;
	gboolean open_source;
	gboolean open_drain;
	gboolean pull_up;
	gboolean pull_down;
	gboolean bias_disabled;
	gboolean clock_monotonic;
	gboolean clock_realtime;
	gboolean clock_hte;
	GTimeSpan debounce_period;
	GArray *output_values;
} LineConfigOpts;

#define LINE_CONFIG_OPTIONS(opts) \
		{ \
			.long_name		= "input", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->input, \
			.description		= "Set direction to input.", \
		}, \
		{ \
			.long_name		= "output", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->output, \
			.description		= "Set direction to output.", \
		}, \
		{ \
			.long_name		= "rising-edge", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->rising_edge, \
			.description		= "Monitor rising edges." \
		}, \
		{ \
			.long_name		= "falling-edge", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->falling_edge, \
			.description		= "Monitor falling edges." \
		}, \
		{ \
			.long_name		= "both-edges", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->both_edges, \
			.description		= "Monitor rising and falling edges." \
		}, \
		{ \
			.long_name		= "push-pull", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->push_pull, \
			.description		= "Drive the line in push-pull mode.", \
		}, \
		{ \
			.long_name		= "open-drain", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->open_drain, \
			.description		= "Drive the line in open-drain mode.", \
		}, \
		{ \
			.long_name		= "open-source", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->open_source, \
			.description		= "Drive the line in open-source mode.", \
		}, \
		{ \
			.long_name		= "pull-up", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->pull_up, \
			.description		= "Enable internal pull-up bias.", \
		}, \
		{ \
			.long_name		= "pull-down", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->pull_down, \
			.description		= "Enable internal pull-down bias.", \
		}, \
		{ \
			.long_name		= "bias-disabled", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->bias_disabled, \
			.description		= "Disable internal pull-up/down bias.", \
		}, \
		{ \
			.long_name		= "active-low", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->active_low, \
			.description		= "Treat the lines as active low.", \
		}, \
		{ \
			.long_name		= "debounce-period", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_INT64, \
			.arg_data		= &(opts)->debounce_period, \
			.arg_description	= "<period in miliseconds>", \
			.description		= "Enable debouncing and set the period", \
		}, \
		{ \
			.long_name		= "clock-monotonic", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->clock_monotonic, \
			.description		= "Use monotonic clock for edge event timestamps", \
		}, \
		{ \
			.long_name		= "clock-realtime", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->clock_realtime, \
			.description		= "Use realtime clock for edge event timestamps", \
		}, \
		{ \
			.long_name		= "clock-hte", \
			.flags			= G_OPTION_FLAG_NONE, \
			.arg			= G_OPTION_ARG_NONE, \
			.arg_data		= &(opts)->clock_hte, \
			.description		= "Use HTE clock (if available) for edge event timestamps", \
		}

void validate_line_config_opts(LineConfigOpts *opts);
GVariant *make_line_config(GArray *offsets, LineConfigOpts *cfg_opts);

#endif /* __GPIOCLI_COMMON_H__ */
