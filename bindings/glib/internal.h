/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_INTERNAL_H__
#define __GPIODGLIB_INTERNAL_H__

#include <glib.h>
#include <glib-object.h>
#include <gpiod.h>

#include "gpiod-glib.h"

GpiodglibLineSettings *
_gpiodglib_line_settings_new(struct gpiod_line_settings *handle);
GpiodglibChipInfo *_gpiodglib_chip_info_new(struct gpiod_chip_info *handle);
GpiodglibLineInfo *_gpiodglib_line_info_new(struct gpiod_line_info *handle);
GpiodglibEdgeEvent *_gpiodglib_edge_event_new(struct gpiod_edge_event *handle);
GpiodglibInfoEvent *_gpiodglib_info_event_new(struct gpiod_info_event *handle);
GpiodglibLineRequest *
_gpiodglib_line_request_new(struct gpiod_line_request *handle);

struct gpiod_request_config *
_gpiodglib_request_config_get_handle(GpiodglibRequestConfig *req_cfg);
struct gpiod_line_config *
_gpiodglib_line_config_get_handle(GpiodglibLineConfig *line_cfg);
struct gpiod_line_settings *
_gpiodglib_line_settings_get_handle(GpiodglibLineSettings *settings);

void _gpiodglib_set_error_from_errno(GError **err,
				     const gchar *fmt, ...) G_GNUC_PRINTF(2, 3);

gchar *_gpiodglib_dup_prop_string(GObject *obj, const gchar *prop);
gboolean _gpiodglib_get_prop_bool(GObject *obj, const gchar *prop);
gint _gpiodglib_get_prop_enum(GObject *obj, const gchar *prop);
guint _gpiodglib_get_prop_uint(GObject *obj, const gchar *prop);
guint64 _gpiodglib_get_prop_uint64(GObject *obj, const gchar *prop);
gulong _gpiodglib_get_prop_ulong(GObject *obj, const gchar *prop);
GTimeSpan _gpiodglib_get_prop_timespan(GObject *obj, const gchar *prop);
GObject *_gpiodglib_get_prop_object(GObject *obj, const gchar *prop);
gpointer _gpiodglib_get_prop_pointer(GObject *obj, const gchar *prop);
gpointer _gpiodglib_get_prop_boxed_array(GObject *obj, const gchar *prop);

void _gpiodglib_set_prop_uint(GObject *obj, const gchar *prop, guint val);
void _gpiodglib_set_prop_string(GObject *obj, const gchar *prop,
				const gchar *val);
void _gpiodglib_set_prop_enum(GObject *obj, const gchar *prop, gint val);
void _gpiodglib_set_prop_bool(GObject *obj, const gchar *prop, gboolean val);
void _gpiodglib_set_prop_timespan(GObject *obj, const gchar *prop,
				  GTimeSpan val);

GpiodglibLineDirection
_gpiodglib_line_direction_from_library(enum gpiod_line_direction direction,
				       gboolean allow_as_is);
GpiodglibLineEdge _gpiodglib_line_edge_from_library(enum gpiod_line_edge edge);
GpiodglibLineBias _gpiodglib_line_bias_from_library(enum gpiod_line_bias bias,
						    gboolean allow_as_is);
GpiodglibLineDrive
_gpiodglib_line_drive_from_library(enum gpiod_line_drive drive);
GpiodglibLineClock
_gpiodglib_line_clock_from_library(enum gpiod_line_clock event_clock);
GpiodglibLineValue
_gpiodglib_line_value_from_library(enum gpiod_line_value value);
GpiodglibInfoEventType
_gpiodglib_info_event_type_from_library(enum gpiod_info_event_type type);
GpiodglibEdgeEventType
_gpiodglib_edge_event_type_from_library(enum gpiod_edge_event_type type);

enum gpiod_line_direction
_gpiodglib_line_direction_to_library(GpiodglibLineDirection direction);
enum gpiod_line_edge _gpiodglib_line_edge_to_library(GpiodglibLineEdge edge);
enum gpiod_line_bias _gpiodglib_line_bias_to_library(GpiodglibLineBias bias);
enum gpiod_line_drive
_gpiodglib_line_drive_to_library(GpiodglibLineDrive drive);
enum gpiod_line_clock
_gpiodglib_line_clock_to_library(GpiodglibLineClock event_clock);
enum gpiod_line_value
_gpiodglib_line_value_to_library(GpiodglibLineValue value);

#endif /* __GPIODGLIB_INTERNAL_H__ */
