# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

subdir('gpiod-glib')

project_headers = files(
  'gpiod-glib/line.h',
  'gpiod-glib/edge-event.h',
  'gpiod-glib/info-event.h',
)

gen_enum_c = custom_target('generated-enums-c',
  input: project_headers + [files('generated-enums.c.template')],
  output: 'generated-enums.c',
  command: [
    glib_mkenums,
    '--template', '@INPUT3@',
    '--output', '@OUTPUT@',
    '@INPUT0@', '@INPUT1@', '@INPUT2@',
  ],
)

glib_sources = [
  'chip.c',
  'chip-info.c',
  'edge-event.c',
  'error.c',
  'info-event.c',
  'internal.c',
  'line-config.c',
  'line-info.c',
  'line-request.c',
  'line-settings.c',
  'misc.c',
  'request-config.c',
]

glib_public_headers = files(
  'gpiod-glib/chip.h',
  'gpiod-glib/chip-info.h',
  'gpiod-glib/edge-event.h',
  'gpiod-glib/error.h',
  'gpiod-glib/info-event.h',
  'gpiod-glib/line.h',
  'gpiod-glib/line-config.h',
  'gpiod-glib/line-info.h',
  'gpiod-glib/line-request.h',
  'gpiod-glib/line-settings.h',
  'gpiod-glib/misc.h',
  'gpiod-glib/request-config.h',
)

glib_inc = include_directories('.', '../../include')

libgpiod_glib_both = both_libraries('gpiod-glib',
  sources: glib_sources + [gen_enum_c, gen_enum_h],
  include_directories: glib_inc,
  c_args: ['-DG_LOG_DOMAIN="gpiod-glib"', '-DGPIODGLIB_COMPILATION'] + profiling_c_args,
  link_args: profiling_link_args,
  dependencies: [libgpiod_dep, glib_dep, gobject_dep, gio_dep, gio_unix_dep],
  version: libgpiod_glib_version,
  soversion: libgpiod_glib_soversion,
  install: true,
)
libgpiod_glib_lib = libgpiod_glib_both.get_shared_lib()

install_headers('gpiod-glib.h')

install_headers(glib_public_headers, subdir: 'gpiod-glib')

pkgconfig.generate(libgpiod_glib_lib,
  name: 'gpiod-glib',
  description: 'GObject bindings for libgpiod',
  url: 'https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/',
  version: meson.project_version(),
  requires_private: ['libgpiod >= 2.1'],
)

libgpiod_glib_dep = declare_dependency(
  link_with: libgpiod_glib_lib,
  include_directories: glib_inc,
)

if opt_introspection.allowed() and gir_dep.found()
  gnome = import('gnome')

  glib_gir = gnome.generate_gir(libgpiod_glib_lib,
    sources: glib_sources + glib_public_headers + [gen_enum_c, gen_enum_h],
    namespace: 'Gpiodglib',
    nsversion: '1.0',
    identifier_prefix: 'Gpiodglib',
    symbol_prefix: 'gpiodglib',
    includes: ['Gio-2.0'],
    header: 'gpiod-glib.h',
    export_packages: 'gpiod-glib',
    extra_args: [
      '--c-include=gpiod-glib.h',
      '--warn-all',
      '-DGPIODGLIB_COMPILATION',
    ],
    include_directories: glib_inc,
    install: true,
  )
endif

if opt_examples.allowed()
  subdir('examples')
endif

if opt_tests.allowed()
  subdir('tests')
endif
