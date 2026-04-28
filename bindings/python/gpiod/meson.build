# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

py.extension_module('_ext',
  sources: [
    'ext/chip.c',
    'ext/common.c',
    'ext/line-config.c',
    'ext/line-settings.c',
    'ext/module.c',
    'ext/request.c',
  ],
  include_directories: [libgpiod_inc],
  dependencies: [libgpiod_dep],
  c_args: ['-D_GNU_SOURCE=1', '-Wall', '-Wextra'],
  subdir: 'gpiod',
  install: true,
)

py_sources = [
  '__init__.py',
  'chip.py',
  'chip_info.py',
  'edge_event.py',
  'exception.py',
  'info_event.py',
  '_internal.py',
  'line_info.py',
  'line.py',
  'line_request.py',
  'line_settings.py',
  'version.py',
  '_ext.pyi',
  'py.typed',
]

py.install_sources(files(py_sources), subdir: 'gpiod')

fs = import('fs')

foreach f : py_sources
  fs.copyfile(f)
endforeach
