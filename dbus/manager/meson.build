# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

gpio_manager_exe = executable('gpio-manager',
  sources: [
    'daemon.c',
    'daemon.h',
    'helpers.c',
    'helpers.h',
    'gpio-manager.c',
  ],
  include_directories: [
    libgpiod_inc,
    include_directories('../../bindings/glib'),
  ],
  c_args: ['-DG_LOG_DOMAIN="gpio-manager"'],
  dependencies: [
    libgpiod_glib_dep,
    libgpiodbus_dep,
    glib_dep,
    gio_dep,
    gio_unix_dep,
    gudev_dep,
  ],
  install: true,
)
