# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

c_examples = [
  'async_watch_line_value',
  'find_line_by_name',
  'get_chip_info',
  'get_line_info',
  'get_line_value',
  'get_multiple_line_values',
  'reconfigure_input_to_output',
  'toggle_line_value',
  'toggle_multiple_line_values',
  'watch_line_info',
  'watch_line_rising',
  'watch_line_value',
  'watch_multiple_line_values',
]

foreach ex : c_examples
  executable(ex,
    sources: [ex + '.c'],
    dependencies: [libgpiod_dep],
  )
endforeach
