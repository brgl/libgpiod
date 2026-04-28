# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

libgpiosim_both = both_libraries('gpiosim',
  sources: ['gpiosim.c'],
  c_args: ['-fvisibility=hidden'],
  dependencies: [libkmod_dep, mount_dep, threads_dep],
  version: libgpiosim_version,
  soversion: libgpiosim_soversion,
  install: true,
)
libgpiosim_lib = libgpiosim_both.get_shared_lib()
libgpiosim_static = libgpiosim_both.get_static_lib()
libgpiosim_inc = include_directories('.')

executable('gpiosim-selftest',
  sources: ['gpiosim-selftest.c'],
  link_with: [libgpiosim_lib],
)
