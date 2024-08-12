// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include "helpers.h"

gint gpiodbus_id_cmp(gconstpointer a, gconstpointer b,
		     gpointer user_data G_GNUC_UNUSED)
{
	const gint *id_a = a;
	const gint *id_b = b;

	if (*id_a < *id_b)
		return -1;
	else if (*id_a > *id_b)
		return 1;

	return 0;
}

static gboolean find_lowest(gpointer key, gpointer value G_GNUC_UNUSED,
			    gpointer data)
{
	gint *lowest = data, *curr = key;

	if (*lowest == *curr)
		(*lowest)++;

	return FALSE;
}

gint gpiodbus_id_alloc(GTree *id_root)
{
	gint lowest = 0, *key;

	g_tree_foreach(id_root, find_lowest, &lowest);

	key = g_malloc(sizeof(*key));
	*key = lowest;
	g_tree_insert(id_root, key, NULL);

	return lowest;
}

void gpiodbus_id_free(GTree *id_root, gint id)
{
	g_assert(g_tree_remove(id_root, &id));
}

gboolean
gpiodbus_chip_set_props(GpiodbusChip *skeleton, GpiodglibChip *chip,
			GError **err)
{
	g_autoptr(GpiodglibChipInfo) info = NULL;
	g_autofree gchar *label = NULL;
	g_autofree gchar *path = NULL;
	g_autofree gchar *name = NULL;

	info = gpiodglib_chip_get_info(chip, err);
	if (!info)
		return FALSE;

	name = gpiodglib_chip_info_dup_name(info);
	label = gpiodglib_chip_info_dup_label(info);

	gpiodbus_chip_set_name(skeleton, name);
	gpiodbus_chip_set_label(skeleton, label);
	gpiodbus_chip_set_num_lines(skeleton,
				    gpiodglib_chip_info_get_num_lines(info));
	path = gpiodglib_chip_dup_path(chip);
	gpiodbus_chip_set_path(skeleton, path);
	g_dbus_interface_skeleton_flush(G_DBUS_INTERFACE_SKELETON(skeleton));

	return TRUE;
}

static const gchar *map_direction(GpiodglibLineDirection direction)
{
	switch (direction) {
	case GPIODGLIB_LINE_DIRECTION_INPUT:
		return "input";
	case GPIODGLIB_LINE_DIRECTION_OUTPUT:
		return "output";
	default:
		g_error("invalid direction value returned by libgpiod-glib");
	}
}

static const gchar *map_edge(GpiodglibLineEdge edge)
{
	switch (edge) {
	case GPIODGLIB_LINE_EDGE_NONE:
		return "none";
	case GPIODGLIB_LINE_EDGE_FALLING:
		return "falling";
	case GPIODGLIB_LINE_EDGE_RISING:
		return "rising";
	case GPIODGLIB_LINE_EDGE_BOTH:
		return "both";
	default:
		g_error("invalid edge value returned by libgpiod-glib");
	}
}

static const gchar *map_bias(GpiodglibLineBias bias)
{
	switch (bias) {
	case GPIODGLIB_LINE_BIAS_UNKNOWN:
		return "unknown";
	case GPIODGLIB_LINE_BIAS_DISABLED:
		return "disabled";
	case GPIODGLIB_LINE_BIAS_PULL_UP:
		return "pull-up";
	case GPIODGLIB_LINE_BIAS_PULL_DOWN:
		return "pull-down";
	default:
		g_error("invalid bias value returned by libgpiod-glib");
	}
}

static const gchar *map_drive(GpiodglibLineDrive drive)
{
	switch (drive) {
	case GPIODGLIB_LINE_DRIVE_PUSH_PULL:
		return "push-pull";
	case GPIODGLIB_LINE_DRIVE_OPEN_DRAIN:
		return "open-drain";
	case GPIODGLIB_LINE_DRIVE_OPEN_SOURCE:
		return "open-source";
	default:
		g_error("invalid drive value returned by libgpiod-glib");
	}
}

static const gchar *map_clock(GpiodglibLineClock event_clock)
{
	switch (event_clock) {
	case GPIODGLIB_LINE_CLOCK_MONOTONIC:
		return "monotonic";
	case GPIODGLIB_LINE_CLOCK_REALTIME:
		return "realtime";
	case GPIODGLIB_LINE_CLOCK_HTE:
		return "hte";
	default:
		g_error("invalid event clock value returned by libgpiod-glib");
	}
}

void gpiodbus_line_set_props(GpiodbusLine *skeleton, GpiodglibLineInfo *info)
{
	g_autofree gchar *consumer = gpiodglib_line_info_dup_consumer(info);
	g_autofree gchar *name = gpiodglib_line_info_dup_name(info);

	gpiodbus_line_set_offset(skeleton,
				 gpiodglib_line_info_get_offset(info));
	gpiodbus_line_set_name(skeleton, name);
	gpiodbus_line_set_used(skeleton, gpiodglib_line_info_is_used(info));
	gpiodbus_line_set_consumer(skeleton, consumer);
	gpiodbus_line_set_direction(skeleton,
			map_direction(gpiodglib_line_info_get_direction(info)));
	gpiodbus_line_set_edge_detection(skeleton,
			map_edge(gpiodglib_line_info_get_edge_detection(info)));
	gpiodbus_line_set_bias(skeleton,
			       map_bias(gpiodglib_line_info_get_bias(info)));
	gpiodbus_line_set_drive(skeleton,
				map_drive(gpiodglib_line_info_get_drive(info)));
	gpiodbus_line_set_active_low(skeleton,
				     gpiodglib_line_info_is_active_low(info));
	gpiodbus_line_set_debounced(skeleton,
				    gpiodglib_line_info_is_debounced(info));
	gpiodbus_line_set_debounce_period_us(skeleton,
			gpiodglib_line_info_get_debounce_period_us(info));
	gpiodbus_line_set_event_clock(skeleton,
			map_clock(gpiodglib_line_info_get_event_clock(info)));
	g_dbus_interface_skeleton_flush(G_DBUS_INTERFACE_SKELETON(skeleton));
}

static gint line_offset_cmp(gconstpointer a, gconstpointer b)
{
	GpiodbusObject *line_obj = (GpiodbusObject *)a;
	GpiodbusLine *line;
	const guint *offset = b;

	line = gpiodbus_object_peek_line(line_obj);

	return gpiodbus_line_get_offset(line) != *offset;
}

void gpiodbus_request_set_props(GpiodbusRequest *skeleton,
				GpiodglibLineRequest *request, GpiodbusChip *chip,
				GDBusObjectManager *line_manager)
{
	g_autolist(GpiodbusObject) line_objs = NULL;
	g_autoptr(GStrvBuilder) builder = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_auto(GStrv) paths = NULL;
	GList *found;
	guint i;

	offsets = gpiodglib_line_request_get_requested_offsets(request);
	line_objs = g_dbus_object_manager_get_objects(line_manager);
	builder = g_strv_builder_new();

	for (i = 0; i < offsets->len; i++) {
		found = g_list_find_custom(line_objs,
					   &g_array_index(offsets, guint, i),
					   line_offset_cmp);
		if (found)
			g_strv_builder_add(builder,
					   g_dbus_object_get_object_path(
						G_DBUS_OBJECT(found->data)));
	}

	paths = g_strv_builder_end(builder);

	gpiodbus_request_set_chip_path(skeleton,
			g_dbus_interface_skeleton_get_object_path(
					G_DBUS_INTERFACE_SKELETON(chip)));
	gpiodbus_request_set_line_paths(skeleton, (const gchar *const *)paths);
	g_dbus_interface_skeleton_flush(G_DBUS_INTERFACE_SKELETON(skeleton));
}

static gboolean
set_settings_from_variant(GpiodglibLineSettings *settings, const gchar *key,
			  GVariant *val)
{
	GpiodglibLineDirection direction;
	GpiodglibLineClock event_clock;
	GpiodglibLineDrive drive;
	GpiodglibLineEdge edge;
	GpiodglibLineBias bias;
	const gchar *str;

	/* FIXME: Make it into a nice set of hashmaps and callbacks. */
	if (g_strcmp0(key, "direction") == 0) {
		str = g_variant_get_string(val, NULL);

		if (g_strcmp0(str, "input") == 0) {
			direction = GPIODGLIB_LINE_DIRECTION_INPUT;
		} else if (g_strcmp0(str, "output") == 0) {
			direction = GPIODGLIB_LINE_DIRECTION_OUTPUT;
		} else if (g_strcmp0(str, "as-is") == 0) {
			direction = GPIODGLIB_LINE_DIRECTION_AS_IS;
		} else {
			g_critical("invalid direction value received: '%s'",
				   str);
			return FALSE;
		}

		gpiodglib_line_settings_set_direction(settings, direction);
	} else if (g_strcmp0(key, "edge") == 0) {
		str = g_variant_get_string(val, NULL);

		if (g_strcmp0(str, "falling") == 0) {
			edge = GPIODGLIB_LINE_EDGE_FALLING;
		} else if (g_strcmp0(str, "rising") == 0) {
			edge = GPIODGLIB_LINE_EDGE_RISING;
		} else if (g_strcmp0(str, "both") == 0) {
			edge = GPIODGLIB_LINE_EDGE_BOTH;
		} else {
			g_critical("invalid edge value received: '%s'", str);
			return FALSE;
		}

		gpiodglib_line_settings_set_edge_detection(settings, edge);
	} else if (g_strcmp0(key, "active-low") == 0) {
		if (g_variant_get_boolean(val))
			gpiodglib_line_settings_set_active_low(settings, TRUE);
	} else if (g_strcmp0(key, "bias") == 0) {
		str = g_variant_get_string(val, NULL);

		if (g_strcmp0(str, "as-is") == 0) {
			bias = GPIODGLIB_LINE_BIAS_AS_IS;
		} else if (g_strcmp0(str, "pull-up") == 0) {
			bias = GPIODGLIB_LINE_BIAS_PULL_UP;
		} else if (g_strcmp0(str, "pull-down") == 0) {
			bias = GPIODGLIB_LINE_BIAS_PULL_DOWN;
		} else if (g_strcmp0(str, "disabled") == 0) {
			bias = GPIODGLIB_LINE_BIAS_DISABLED;
		} else {
			g_critical("invalid bias value received: '%s'", str);
			return FALSE;
		}

		gpiodglib_line_settings_set_bias(settings, bias);
	} else if (g_strcmp0(key, "drive") == 0) {
		str = g_variant_get_string(val, NULL);

		if (g_strcmp0(str, "push-pull") == 0) {
			drive = GPIODGLIB_LINE_DRIVE_PUSH_PULL;
		} else if (g_strcmp0(str, "open-drain") == 0) {
			drive = GPIODGLIB_LINE_DRIVE_OPEN_DRAIN;
		} else if (g_strcmp0(str, "open-source") == 0) {
			drive = GPIODGLIB_LINE_DRIVE_OPEN_SOURCE;
		} else {
			g_critical("invalid drive value received: '%s'", str);
			return FALSE;
		}

		gpiodglib_line_settings_set_drive(settings, drive);
	} else if (g_strcmp0(key, "debounce-period") == 0) {
		gpiodglib_line_settings_set_debounce_period_us(settings,
						g_variant_get_int64(val));
	} else if (g_strcmp0(key, "event-clock") == 0) {
		str = g_variant_get_string(val, NULL);

		if (g_strcmp0(str, "monotonic") == 0) {
			event_clock = GPIODGLIB_LINE_CLOCK_MONOTONIC;
		} else if (g_strcmp0(str, "realtime") == 0) {
			event_clock = GPIODGLIB_LINE_CLOCK_REALTIME;
		} else if (g_strcmp0(str, "hte") == 0) {
			event_clock = GPIODGLIB_LINE_CLOCK_HTE;
		} else {
			g_critical("invalid event clock value received: '%s'",
				   str);
			return FALSE;
		}

		gpiodglib_line_settings_set_event_clock(settings, event_clock);
	} else {
		g_critical("invalid config option received: '%s'", key);
		return FALSE;
	}

	return TRUE;
}

GpiodglibLineConfig *gpiodbus_line_config_from_variant(GVariant *variant)
{
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) config = NULL;
	g_autoptr(GVariant) output_values_v = NULL;
	g_autoptr(GVariant) line_configs_v = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	GVariantIter iter0, iter1;
	guint offset;
	gboolean ret;
	GVariant *v;
	gchar *k;
	gint val;

	line_configs_v = g_variant_get_child_value(variant, 0);
	output_values_v = g_variant_get_child_value(variant, 1);

	config = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(NULL);

	g_variant_iter_init(&iter0, line_configs_v);
	while ((v = g_variant_iter_next_value(&iter0))) {
		g_autoptr(GVariant) line_settings_v = NULL;
		g_autoptr(GVariant) line_config_v = v;
		g_autoptr(GVariant) offsets_v = NULL;
		g_autoptr(GArray) offsets = NULL;

		offsets_v = g_variant_get_child_value(line_config_v, 0);
		line_settings_v = g_variant_get_child_value(line_config_v, 1);

		gpiodglib_line_settings_reset(settings);
		g_variant_iter_init(&iter1, line_settings_v);
		while (g_variant_iter_next(&iter1, "{sv}", &k, &v)) {
			g_autoptr(GVariant) val = v;
			g_autofree gchar *key = k;

			ret = set_settings_from_variant(settings, key, val);
			if (!ret)
				return NULL;
		}

		offsets = g_array_sized_new(FALSE, TRUE, sizeof(guint),
					    g_variant_n_children(offsets_v));
		g_variant_iter_init(&iter1, offsets_v);
		while (g_variant_iter_next(&iter1, "u", &offset))
			g_array_append_val(offsets, offset);

		ret = gpiodglib_line_config_add_line_settings(config, offsets,
							      settings, &err);
		if (!ret) {
			g_critical("failed to add line settings: %s",
				   err->message);
			return NULL;
		}
	}

	values = g_array_sized_new(FALSE, TRUE, sizeof(gint),
				   g_variant_n_children(output_values_v));
	g_variant_iter_init(&iter0, output_values_v);
	while (g_variant_iter_next(&iter0, "i", &val))
		g_array_append_val(values, val);

	if (values->len > 0) {
		ret = gpiodglib_line_config_set_output_values(config, values,
							      &err);
		if (!ret) {
			g_critical("failed to set output values: %s",
				   err->message);
			return NULL;
		}
	}

	return g_object_ref(config);
}

GpiodglibRequestConfig *gpiodbus_request_config_from_variant(GVariant *variant)
{
	g_autoptr(GpiodglibRequestConfig) config = NULL;
	GVariantIter iter;
	GVariant *v;
	gchar *k;

	config = gpiodglib_request_config_new(NULL);

	g_variant_iter_init(&iter, variant);
	while (g_variant_iter_next(&iter, "{sv}", &k, &v)) {
		g_autoptr(GVariant) val = v;
		g_autofree gchar *key = k;

		if (g_strcmp0(key, "consumer") == 0) {
			gpiodglib_request_config_set_consumer(config,
					g_variant_get_string(val, NULL));
		} else if (g_strcmp0(key, "event-buffer-size") == 0) {
			gpiodglib_request_config_set_event_buffer_size(config,
						g_variant_get_uint32(val));
		} else {
			g_critical("invalid request config option received: '%s'",
				   key);
			return NULL;
		}
	}

	return g_object_ref(config);
}
