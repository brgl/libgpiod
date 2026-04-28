# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

subdir('gpiosim')
subdir('gpiosim-glib')
subdir('harness')

executable('gpiod-test',
  sources: [
    'helpers.h',
    'tests-chip.c',
    'tests-chip-info.c',
    'tests-edge-event.c',
    'tests-info-event.c',
    'tests-kernel-uapi.c',
    'tests-line-config.c',
    'tests-line-info.c',
    'tests-line-request.c',
    'tests-line-settings.c',
    'tests-misc.c',
    'tests-request-config.c',
  ],
  include_directories: [
    libgpiod_inc,
    include_directories('gpiosim'),
    include_directories('gpiosim-glib'),
    include_directories('harness'),
  ],
  c_args: ['-DG_LOG_DOMAIN="gpiod-test"'],
  dependencies: [
    libgpiod_dep,
    glib_test_dep,
    gio_test_dep,
    threads_dep,
  ],
  link_with: [libgpiosim_lib, libgpiosim_glib_lib, libgpiod_test_harness_lib],
)
