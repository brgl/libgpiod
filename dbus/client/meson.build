# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

gpiocli_exe = executable('gpiocli',
  sources: [
    'common.c',
    'common.h',
    'detect.c',
    'find.c',
    'get.c',
    'gpiocli.c',
    'info.c',
    'monitor.c',
    'notify.c',
    'reconfigure.c',
    'release.c',
    'request.c',
    'requests.c',
    'set.c',
    'wait.c',
  ],
  c_args: ['-DG_LOG_DOMAIN="gpiocli"'],
  dependencies: [
    libgpiodbus_dep,
    glib_dep,
    gio_dep,
    gio_unix_dep,
  ],
  install: true,
)
