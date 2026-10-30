# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

libgpiosim_lib = library('gpiosim',
  sources: ['gpiosim.c'],
  c_args: ['-fvisibility=hidden'],
  dependencies: [libkmod_dep, mount_dep, threads_dep],
  version: libgpiosim_version,
  soversion: libgpiosim_soversion,
  install: true,
)

# The Rust bindings always link against the static libgpiosim.
if get_option('default_library') == 'both'
  libgpiosim_static = libgpiosim_lib.get_static_lib()
elif get_option('default_library') == 'static'
  libgpiosim_static = libgpiosim_lib
else
  libgpiosim_static = static_library('gpiosim',
    objects: libgpiosim_lib.extract_all_objects(recursive: false),
    build_by_default: false,
  )
endif

libgpiosim_inc = include_directories('.')

executable('gpiosim-selftest',
  sources: ['gpiosim-selftest.c'],
  link_with: [libgpiosim_lib],
)
