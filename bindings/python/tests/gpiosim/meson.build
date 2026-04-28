# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

py.extension_module('_ext',
  sources: ['ext.c'],
  include_directories: [libgpiosim_inc],
  link_with: [libgpiosim_lib],
  c_args: ['-D_GNU_SOURCE=1', '-Wall', '-Wextra'],
  install: false,
)

fs = import('fs')

foreach f : ['__init__.py', 'chip.py', '_ext.pyi']
  fs.copyfile(f)
endforeach
