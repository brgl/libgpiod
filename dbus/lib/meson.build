# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

gnome = import('gnome')

generated_dbus = gnome.gdbus_codegen('generated-gpiodbus',
  sources: 'io.gpiod1.xml',
  interface_prefix: 'io.gpiod1',
  namespace: 'Gpiodbus',
  object_manager: true,
  autocleanup: 'all',
  extra_args: ['--glib-min-required', '2.74.0'],
)

dbus_lib_inc = include_directories('.', '../../include')

libgpiodbus_both = both_libraries('gpiodbus',
  sources: ['gpiodbus.h', generated_dbus],
  include_directories: dbus_lib_inc,
  c_args: ['-DG_LOG_DOMAIN="gpiodbus"'],
  dependencies: [glib_dep, gio_dep],
  soversion: 1,
  install: true,
)
libgpiodbus_lib = libgpiodbus_both.get_shared_lib()

install_data('io.gpiod1.xml',
  install_dir: get_option('datadir') / 'dbus-1' / 'interfaces',
)

libgpiodbus_dep = declare_dependency(
  link_with: libgpiodbus_lib,
  include_directories: dbus_lib_inc,
  sources: generated_dbus,
)
