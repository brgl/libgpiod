# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

py.extension_module('_ext',
  sources: ['ext.c'],
  c_args: ['-D_GNU_SOURCE=1', '-Wall', '-Wextra'],
  install: false,
)

fs = import('fs')

foreach f : ['__init__.py', '_ext.pyi']
  fs.copyfile(f)
endforeach
