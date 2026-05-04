# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

cxx_inc = include_directories('.', '../../include')

libgpiodcxx_both = both_libraries('gpiodcxx',
  sources: [
    'chip.cpp',
    'chip-info.cpp',
    'edge-event-buffer.cpp',
    'edge-event.cpp',
    'exception.cpp',
    'info-event.cpp',
    'internal.cpp',
    'line.cpp',
    'line-config.cpp',
    'line-info.cpp',
    'line-request.cpp',
    'line-settings.cpp',
    'misc.cpp',
    'request-builder.cpp',
    'request-config.cpp',
  ],
  include_directories: cxx_inc,
  cpp_args: ['-fvisibility=hidden'] + profiling_c_args,
  link_args: profiling_link_args,
  dependencies: [libgpiod_dep],
  version: libgpiodcxx_version,
  soversion: libgpiodcxx_soversion,
  install: true,
)
libgpiodcxx_lib = libgpiodcxx_both.get_shared_lib()

install_headers('gpiod.hpp')

install_headers(
  'gpiodcxx/chip.hpp',
  'gpiodcxx/chip-info.hpp',
  'gpiodcxx/edge-event-buffer.hpp',
  'gpiodcxx/edge-event.hpp',
  'gpiodcxx/exception.hpp',
  'gpiodcxx/info-event.hpp',
  'gpiodcxx/line.hpp',
  'gpiodcxx/line-config.hpp',
  'gpiodcxx/line-info.hpp',
  'gpiodcxx/line-request.hpp',
  'gpiodcxx/line-settings.hpp',
  'gpiodcxx/misc.hpp',
  'gpiodcxx/request-builder.hpp',
  'gpiodcxx/request-config.hpp',
  'gpiodcxx/timestamp.hpp',
  subdir: 'gpiodcxx',
)

pkgconfig.generate(libgpiodcxx_lib,
  name: 'libgpiodcxx',
  description: 'C++ bindings for libgpiod',
  url: project_url,
  version: meson.project_version(),
  requires: ['libgpiod'],
)

libgpiodcxx_dep = declare_dependency(
  link_with: libgpiodcxx_lib,
  include_directories: cxx_inc,
)

if opt_examples.allowed()
  subdir('examples')
endif

if opt_tests.allowed()
  subdir('tests')
endif
