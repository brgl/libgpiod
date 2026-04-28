# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

executable('gpiod-cxx-test',
  sources: [
    'check-kernel.cpp',
    'gpiod-cxx-test-main.cpp',
    'gpiosim.cpp',
    'helpers.cpp',
    'tests-chip.cpp',
    'tests-chip-info.cpp',
    'tests-edge-event.cpp',
    'tests-info-event.cpp',
    'tests-line.cpp',
    'tests-line-config.cpp',
    'tests-line-info.cpp',
    'tests-line-request.cpp',
    'tests-line-settings.cpp',
    'tests-misc.cpp',
    'tests-request-config.cpp',
  ],
  include_directories: [
    cxx_inc,
    include_directories('../../..'),
    include_directories('../../../tests/gpiosim'),
  ],
  dependencies: [libgpiodcxx_dep, catch2_test_dep, threads_dep],
  link_with: [libgpiosim_lib],
)
