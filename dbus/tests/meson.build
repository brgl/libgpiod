# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

executable('gpiodbus-test',
  sources: [
    'daemon-process.c',
    'daemon-process.h',
    'helpers.c',
    'helpers.h',
    'tests-chip.c',
    'tests-line.c',
    'tests-request.c',
  ],
  include_directories: [
    include_directories('../../tests/gpiosim-glib'),
    include_directories('../../tests/harness'),
  ],
  c_args: ['-DG_LOG_DOMAIN="gpiodbus-test"'],
  dependencies: [
    libgpiodbus_dep,
    glib_test_dep,
    gio_test_dep,
  ],
  link_with: [libgpiosim_lib, libgpiosim_glib_lib, libgpiod_test_harness_lib],
)
