// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <glib/gprintf.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

static void print_err_msg(GError *err, const gchar *fmt, va_list va)
{
	g_printerr("%s: ", g_get_prgname());
	g_vfprintf(stderr, fmt, va);
	if (err)
		g_printerr(": %s", err->message);
	g_printerr("\n");
}

void die(const gchar *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	print_err_msg(NULL, fmt, va);
	va_end(va);

	exit(EXIT_FAILURE);
}

void die_gerror(GError *err, const gchar *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	print_err_msg(err, fmt, va);
	va_end(va);

	exit(EXIT_FAILURE);
}

void die_parsing_opts(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	print_err_msg(NULL, fmt, va);
	va_end(va);
	g_printerr("\nSee %s --help\n", g_get_prgname());

	exit(EXIT_FAILURE);
}

void parse_options(const GOptionEntry *opts, const gchar *summary,
		   const gchar *description, int *argc, char ***argv)
{
	g_autoptr(GOptionContext) ctx = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	ctx = g_option_context_new(NULL);
	g_option_context_set_summary(ctx, summary);
	g_option_context_set_description(ctx, description);
	g_option_context_add_main_entries(ctx, opts, NULL);
	g_option_context_set_strict_posix(ctx, TRUE);

	ret = g_option_context_parse(ctx, argc, argv, &err);
	if (!ret) {
		g_printerr("%s: Option parsing failed: %s\nSee %s --help\n",
			   g_get_prgname(), err->message, g_get_prgname());
		exit(EXIT_FAILURE);
	}
}

void check_manager(void)
{
	g_autoptr(GDBusProxy) proxy = NULL;
	g_autoptr(GVariant) result = NULL;
	g_autoptr(GError) err = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, NULL,
			"io.gpiod1", "/io/gpiod1", "org.freedesktop.DBus.Peer",
			NULL, &err);
	if (!proxy)
		die_gerror(err, "Unable to create a proxy to '/io/gpiod1'");

	result = g_dbus_proxy_call_sync(proxy, "Ping", NULL,
					G_DBUS_CALL_FLAGS_NONE, -1, NULL,
					&err);
	if (!result) {
		if (err->domain == G_DBUS_ERROR) {
			switch (err->code) {
			case G_DBUS_ERROR_ACCESS_DENIED:
				die("Access to gpio-manager denied, check your permissions");
			case G_DBUS_ERROR_SERVICE_UNKNOWN:
				die("gpio-manager not running");
			}
		}

		die_gerror(err, "Failed trying to contect the gpio manager");
	}
}

gboolean quit_main_loop_on_signal(gpointer user_data)
{
	GMainLoop *loop = user_data;

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

void die_on_name_vanished(GDBusConnection *con G_GNUC_UNUSED,
			  const gchar *name G_GNUC_UNUSED,
			  gpointer user_data G_GNUC_UNUSED)
{
	die("gpio-manager exited unexpectedly");
}

GList *strv_to_gstring_list(GStrv lines)
{
	gsize llen = g_strv_length(lines);
	GList *list = NULL;
	guint i;

	for (i = 0; i < llen; i++)
		list = g_list_append(list, g_string_new(lines[i]));

	return list;
}

gint output_value_from_str(const gchar *value_str)
{
	if ((g_strcmp0(value_str, "active") == 0) ||
	    (g_strcmp0(value_str, "1") == 0))
		return 1;
	else if ((g_strcmp0(value_str, "inactive") == 0) ||
		 (g_strcmp0(value_str, "0") == 0))
		return 0;

	die_parsing_opts("invalid output value: '%s'", value_str);
}

static gboolean str_is_all_digits(const gchar *str)
{
	for (; *str; str++) {
		if (!g_ascii_isdigit(*str))
			return FALSE;
	}

	return TRUE;
}

static gint compare_objs_by_path(GDBusObject *a, GDBusObject *b)
{
	return strverscmp(g_dbus_object_get_object_path(a),
			  g_dbus_object_get_object_path(b));
}

GDBusObjectManager *get_object_manager_client(const gchar *obj_path)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GError) err = NULL;

	manager = gpiodbus_object_manager_client_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"io.gpiod1", obj_path, NULL, &err);
	if (!manager)
		die_gerror(err,
			   "failed to create the object manager client for %s",
			   obj_path);

	return g_object_ref(manager);
}

static gchar *make_chip_obj_path(const gchar *chip)
{
	return g_strdup_printf(
		str_is_all_digits(chip) ?
			"/io/gpiod1/chips/gpiochip%s" :
			"/io/gpiod1/chips/%s",
		chip);
}

GpiodbusObject *get_chip_obj_by_path(const gchar *obj_path)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GpiodbusObject) chip_obj = NULL;

	manager = get_object_manager_client("/io/gpiod1/chips");

	chip_obj = GPIODBUS_OBJECT(g_dbus_object_manager_get_object(manager,
								    obj_path));
	if (!chip_obj)
		die("No such chip object: '%s'", obj_path);

	return g_object_ref(chip_obj);
}

GpiodbusObject *get_chip_obj(const gchar *chip_name)
{
	g_autofree gchar *chip_path = make_chip_obj_path(chip_name);

	return get_chip_obj_by_path(chip_path);
}

GList *get_chip_objs(GStrv chip_names)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	GList *objs = NULL;
	gint i;

	manager = get_object_manager_client("/io/gpiod1/chips");

	if (!chip_names)
		return g_list_sort(g_dbus_object_manager_get_objects(manager),
				   (GCompareFunc)compare_objs_by_path);

	for (i = 0; chip_names[i]; i++) {
		g_autofree gchar *obj_path = make_chip_obj_path(chip_names[i]);
		g_autoptr(GpiodbusObject) obj = NULL;

		obj = GPIODBUS_OBJECT(
			g_dbus_object_manager_get_object(manager, obj_path));
		if (!obj)
			die("No such chip: '%s'", chip_names[i]);

		objs = g_list_insert_sorted(objs, g_object_ref(obj),
					    (GCompareFunc)compare_objs_by_path);
	}

	return objs;
}

gchar *make_request_obj_path(const gchar *request)
{
	return g_strdup_printf(
		str_is_all_digits(request) ?
			"/io/gpiod1/requests/request%s" :
			"/io/gpiod1/requests/%s",
		request);
}

GpiodbusObject *get_request_obj(const gchar *request_name)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GpiodbusObject) req_obj = NULL;
	g_autofree gchar *obj_path = NULL;

	manager = get_object_manager_client("/io/gpiod1/requests");
	obj_path = make_request_obj_path(request_name);

	req_obj = GPIODBUS_OBJECT(g_dbus_object_manager_get_object(manager,
								   obj_path));
	if (!req_obj)
		die("No such request: '%s'", request_name);

	return g_object_ref(req_obj);
}

GList *get_request_objs(void)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	GList *objs = NULL;

	manager = get_object_manager_client("/io/gpiod1/requests");
	objs = g_dbus_object_manager_get_objects(manager);

	return g_list_sort(objs, (GCompareFunc)compare_objs_by_path);
}

GArray *get_request_offsets(GpiodbusRequest *request)
{
	const gchar *chip_path, *line_path, *const *line_paths;
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autoptr(GArray) offsets = NULL;
	GpiodbusLine *line;
	guint i, offset;

	chip_path = gpiodbus_request_get_chip_path(request);
	line_paths = gpiodbus_request_get_line_paths(request);
	offsets = g_array_new(FALSE, TRUE, sizeof(guint));
	manager = get_object_manager_client(chip_path);

	for (i = 0, line_path = line_paths[i];
	     line_path;
	     line_path = line_paths[++i]) {
		g_autoptr(GDBusObject) line_obj = NULL;

		line_obj = g_dbus_object_manager_get_object(manager, line_path);
		line = gpiodbus_object_peek_line(GPIODBUS_OBJECT(line_obj));
		offset = gpiodbus_line_get_offset(line);
		g_array_append_val(offsets, offset);
	}

	return g_array_ref(offsets);
}

gboolean get_line_obj_by_name(const gchar *name, GpiodbusObject **line_obj,
			      GpiodbusObject **chip_obj)
{
	g_autolist(GpiodbusObject) chip_objs = NULL;
	GList *pos;

	if (str_is_all_digits(name))
		die("Refusing to use line offsets if chip is not specified");

	chip_objs = get_chip_objs(NULL);

	for (pos = g_list_first(chip_objs); pos; pos = g_list_next(pos)) {
		*line_obj = get_line_obj_by_name_for_chip(pos->data, name);
		if (*line_obj) {
			if (chip_obj)
				*chip_obj = g_object_ref(pos->data);
			return TRUE;
		}
	}

	return FALSE;
}

GpiodbusObject *
get_line_obj_by_name_for_chip(GpiodbusObject *chip_obj, const gchar *line_name)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	g_autolist(GpiodbusObject) line_objs = NULL;
	const gchar *chip_path;
	GpiodbusLine *line;
	guint64 offset;
	GList *pos;

	chip_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(chip_obj));
	manager = get_object_manager_client(chip_path);
	line_objs = g_dbus_object_manager_get_objects(manager);

	for (pos = g_list_first(line_objs); pos; pos = g_list_next(pos)) {
		line = gpiodbus_object_peek_line(pos->data);

		if (g_strcmp0(gpiodbus_line_get_name(line), line_name) == 0)
			return g_object_ref(pos->data);

		if (str_is_all_digits(line_name)) {
			offset = g_ascii_strtoull(line_name, NULL, 10);
			if (offset == gpiodbus_line_get_offset(line))
				return g_object_ref(pos->data);
		}
	}

	return NULL;
}

GList *get_all_line_objs_for_chip(GpiodbusObject *chip_obj)
{
	g_autoptr(GDBusObjectManager) manager = NULL;
	const gchar *chip_path;

	chip_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(chip_obj));
	manager = get_object_manager_client(chip_path);

	return g_list_sort(g_dbus_object_manager_get_objects(manager),
			   (GCompareFunc)compare_objs_by_path);
}

static gchar *sanitize_str(const gchar *str)
{
	if (!strlen(str))
		return NULL;

	return g_strdup(str);
}

static const gchar *sanitize_direction(const gchar *direction)
{
	if ((g_strcmp0(direction, "input") == 0) ||
	    (g_strcmp0(direction, "output") == 0))
		return direction;

	die("invalid direction value received from manager: '%s'", direction);
}

static const gchar *sanitize_drive(const gchar *drive)
{
	if ((g_strcmp0(drive, "push-pull") == 0) ||
	    (g_strcmp0(drive, "open-source") == 0) ||
	    (g_strcmp0(drive, "open-drain") == 0))
		return drive;

	die("invalid drive value received from manager: '%s'", drive);
}

static const gchar *sanitize_bias(const gchar *bias)
{
	if ((g_strcmp0(bias, "pull-up") == 0) ||
	    (g_strcmp0(bias, "pull-down") == 0) ||
	    (g_strcmp0(bias, "disabled") == 0))
		return bias;

	if (g_strcmp0(bias, "unknown") == 0)
		return NULL;

	die("invalid bias value received from manager: '%s'", bias);
}

static const gchar *sanitize_edge(const gchar *edge)
{
	if ((g_strcmp0(edge, "rising") == 0) ||
	    (g_strcmp0(edge, "falling") == 0) ||
	    (g_strcmp0(edge, "both") == 0))
		return edge;

	if (g_strcmp0(edge, "none") == 0)
		return NULL;

	die("invalid edge value received from manager: '%s'", edge);
}

static const gchar *sanitize_clock(const gchar *event_clock)
{
	if ((g_strcmp0(event_clock, "monotonic") == 0) ||
	    (g_strcmp0(event_clock, "realtime") == 0) ||
	    (g_strcmp0(event_clock, "hte") == 0))
		return event_clock;

	die("invalid clock value received from manager: '%s'", event_clock);
}

gchar *sanitize_object_path(const gchar *path)
{
	if (g_strcmp0(path, "/") == 0)
		return g_strdup("N/A");

	return g_path_get_basename(path);
}

LineProperties *get_line_properties(GpiodbusLine *line)
{
	LineProperties *props;

	props = g_malloc0(sizeof(*props));
	props->name = sanitize_str(gpiodbus_line_get_name(line));
	props->offset = gpiodbus_line_get_offset(line);
	props->used = gpiodbus_line_get_used(line);
	props->consumer = sanitize_str(gpiodbus_line_get_consumer(line));
	props->direction = sanitize_direction(
				gpiodbus_line_get_direction(line));
	props->drive = sanitize_drive(gpiodbus_line_get_drive(line));
	props->bias = sanitize_bias(gpiodbus_line_get_bias(line));
	props->active_low = gpiodbus_line_get_active_low(line);
	props->edge = sanitize_edge(gpiodbus_line_get_edge_detection(line));
	props->debounced = gpiodbus_line_get_debounced(line);
	props->debounce_period = gpiodbus_line_get_debounce_period_us(line);
	props->event_clock = sanitize_clock(
				gpiodbus_line_get_event_clock(line));
	props->managed = gpiodbus_line_get_managed(line);
	props->request_name = sanitize_object_path(
			gpiodbus_line_get_request_path(line));

	return props;
}

void free_line_properties(LineProperties *props)
{
	g_free(props->name);
	g_free(props->consumer);
	g_free(props->request_name);
	g_free(props);
}

void validate_line_config_opts(LineConfigOpts *opts)
{
	gint counter;

	if (opts->input && opts->output)
		die_parsing_opts("--input and --output are mutually exclusive");

	if (opts->both_edges)
		opts->rising_edge = opts->falling_edge = TRUE;

	if (!opts->input && (opts->rising_edge || opts->falling_edge))
		die_parsing_opts("monitoring edges is only possible in input mode");

	counter = 0;
	if (opts->push_pull)
		counter++;
	if (opts->open_drain)
		counter++;
	if (opts->open_source)
		counter++;

	if (counter > 1)
		die_parsing_opts("--push-pull, --open-drain and --open-source are mutually exclusive");

	if (!opts->output && (counter > 0))
		die_parsing_opts("--push-pull, --open-drain and --open-source are only available in output mode");

	counter = 0;
	if (opts->pull_up)
		counter++;
	if (opts->pull_down)
		counter++;
	if (opts->bias_disabled)
		counter++;

	if (counter > 1)
		die_parsing_opts("--pull-up, --pull-down and --bias-disabled are mutually exclusive");

	counter = 0;
	if (opts->clock_monotonic)
		counter++;
	if (opts->clock_realtime)
		counter++;
	if (opts->clock_hte)
		counter++;

	if (counter > 1)
		die_parsing_opts("--clock-monotonic, --clock-realtime and --clock-hte are mutually exclusive");

	if (counter > 0 && (!opts->rising_edge && !opts->falling_edge))
		die_parsing_opts("--clock-monotonic, --clock-realtime and --clock-hte can only be used with edge detection enabled");

	if (opts->debounce_period && (!opts->rising_edge && !opts->falling_edge))
		die_parsing_opts("--debounce-period can only be used with edge-detection enabled");
}

GVariant *make_line_config(GArray *offsets, LineConfigOpts *opts)
{
	const char *direction, *edge = NULL, *bias = NULL, *drive = NULL,
		   *clock = NULL;
	g_autoptr(GVariant) output_values = NULL;
	g_autoptr(GVariant) line_settings = NULL;
	g_autoptr(GVariant) line_offsets = NULL;
	g_autoptr(GVariant) line_configs = NULL;
	g_autoptr(GVariant) line_config = NULL;
	GVariantBuilder builder;
	guint i;

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	for (i = 0; i < offsets->len; i++)
		g_variant_builder_add_value(&builder,
			g_variant_new_uint32(g_array_index(offsets, guint, i)));
	line_offsets = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

	if (opts->input)
		direction = "input";
	else if (opts->output)
		direction = "output";
	else
		direction = "as-is";

	if (direction)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "direction",
				      g_variant_new_string(direction)));

	if (opts->rising_edge && opts->falling_edge)
		edge = "both";
	else if (opts->falling_edge)
		edge = "falling";
	else if (opts->rising_edge)
		edge = "rising";

	if (edge)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "edge",
				      g_variant_new_string(edge)));

	if (opts->pull_up)
		bias = "pull-up";
	else if (opts->pull_down)
		bias = "pull-down";
	else if (opts->bias_disabled)
		bias = "disabled";

	if (bias)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "bias",
				      g_variant_new_string(bias)));

	if (opts->push_pull)
		drive = "push-pull";
	else if (opts->open_drain)
		drive = "open-drain";
	else if (opts->open_source)
		drive = "open-source";

	if (drive)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "drive",
				      g_variant_new_string(drive)));

	if (opts->active_low)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "active-low",
				      g_variant_new_boolean(TRUE)));

	if (opts->debounce_period)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "debounce-period",
				g_variant_new_int64(opts->debounce_period)));

	if (opts->clock_monotonic)
		clock = "monotonic";
	else if (opts->clock_realtime)
		clock = "realtime";
	else if (opts->clock_hte)
		clock = "hte";

	if (clock)
		g_variant_builder_add_value(&builder,
			g_variant_new("{sv}", "event-clock",
				      g_variant_new_string(clock)));

	line_settings = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&builder, g_variant_ref(line_offsets));
	g_variant_builder_add_value(&builder, g_variant_ref(line_settings));
	line_config = g_variant_builder_end(&builder);

	g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add_value(&builder, g_variant_ref(line_config));
	line_configs = g_variant_builder_end(&builder);

	if (opts->output_values) {
		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
		for (i = 0; i < opts->output_values->len; i++) {
			g_variant_builder_add(&builder, "i",
					g_array_index(opts->output_values,
						      gint, i));
		}
		output_values = g_variant_builder_end(&builder);
	} else {
		output_values = g_variant_new("ai", opts->output_values);
	}

	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&builder, g_variant_ref(line_configs));
	g_variant_builder_add_value(&builder, g_variant_ref(output_values));

	return g_variant_ref_sink(g_variant_builder_end(&builder));
}
