# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

glib_examples = [
  'find_line_by_name_glib',
  'get_chip_info_glib',
  'get_line_info_glib',
  'get_line_value_glib',
  'get_multiple_line_values_glib',
  'reconfigure_input_to_output_glib',
  'toggle_line_value_glib',
  'toggle_multiple_line_values_glib',
  'watch_line_info_glib',
  'watch_line_value_glib',
  'watch_multiple_edge_rising_glib',
]

foreach ex : glib_examples
  executable(ex,
    sources: [ex + '.c'],
    include_directories: glib_inc,
    c_args: ['-DG_LOG_DOMAIN="gpiotools-glib"'],
    dependencies: [libgpiod_glib_dep, glib_dep, gobject_dep],
  )
endforeach
