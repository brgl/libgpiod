# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

libgpiod_both = both_libraries('gpiod',
  sources: [
    'chip.c',
    'chip-info.c',
    'edge-event.c',
    'info-event.c',
    'internal.c',
    'line-config.c',
    'line-info.c',
    'line-request.c',
    'line-settings.c',
    'misc.c',
    'request-config.c',
  ],
  include_directories: [libgpiod_inc, include_directories('.')],
  c_args: ['-fvisibility=hidden'] + profiling_c_args,
  link_args: profiling_link_args,
  version: libgpiod_version,
  soversion: libgpiod_soversion,
  install: true,
)
libgpiod_lib = libgpiod_both.get_shared_lib()

pkgconfig = import('pkgconfig')
pkgconfig.generate(libgpiod_lib,
  name: 'libgpiod',
  description: 'Library and tools for the Linux GPIO chardev',
  url: project_url,
  version: meson.project_version(),
)

libgpiod_dep = declare_dependency(
  link_with: libgpiod_lib,
  include_directories: libgpiod_inc,
)
