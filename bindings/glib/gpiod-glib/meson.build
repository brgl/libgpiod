# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

gen_enum_h = custom_target('generated-enums-h',
  input: [
    files('line.h', 'edge-event.h', 'info-event.h'),
    files('../generated-enums.h.template'),
  ],
  output:  'generated-enums.h',
  command: [
    glib_mkenums,
    '--template', '@INPUT3@',
    '--output', '@OUTPUT@',
    '@INPUT0@', '@INPUT1@', '@INPUT2@',
  ],
  install: true,
  install_dir: get_option('includedir') / 'gpiod-glib',
)
