# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

libgpiosim_glib_lib = static_library('gpiosim-glib',
  sources: ['gpiosim-glib.c'],
  include_directories: [include_directories('../gpiosim')],
  c_args: ['-DG_LOG_DOMAIN="gpiosim-glib"'],
  dependencies: [glib_test_dep, gio_test_dep],
  link_with: [libgpiosim_lib],
  pic: true,
)
