# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

executable('gpiod-glib-test',
  sources: [
    'helpers.c',
    'tests-chip.c',
    'tests-chip-info.c',
    'tests-edge-event.c',
    'tests-info-event.c',
    'tests-line-config.c',
    'tests-line-info.c',
    'tests-line-request.c',
    'tests-line-settings.c',
    'tests-misc.c',
    'tests-request-config.c',
  ],
  include_directories: [
    glib_inc,
    include_directories('../../..'),
    include_directories('../../../tests/gpiosim-glib'),
    include_directories('../../../tests/harness'),
  ],
  c_args: ['-DG_LOG_DOMAIN="gpiod-glib-test"'],
  dependencies: [libgpiod_glib_dep, glib_test_dep, gio_test_dep],
  link_with: [libgpiosim_lib, libgpiosim_glib_lib, libgpiod_test_harness_lib],
)
