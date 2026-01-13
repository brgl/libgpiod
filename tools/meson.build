# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

libgpiotools_lib = library('gpiotools',
  sources: ['gpiotools.c'],
  include_directories: libgpiod_inc,
  c_args: ['-fvisibility=hidden'],
  dependencies: [libgpiod_dep],
  version: libgpiotools_version,
  soversion: libgpiotools_soversion,
  install: true,
)

install_headers('gpiotools.h')

pkgconfig = import('pkgconfig')
pkgconfig.generate(libgpiotools_lib,
  name: 'libgpiotools',
  description: 'Collection of high-level functions for building programs using libgpiod',
  url: project_url,
  version: meson.project_version(),
)

libgpiotools_dep = declare_dependency(
  link_with: libgpiotools_lib,
  include_directories: include_directories('.'),
)

tools_common_lib = static_library('tools-common',
  sources: ['tools-common.c'],
  include_directories: libgpiod_inc,
  dependencies: [libgpiotools_dep],
  pic: true,
)

tools_common_dep = declare_dependency(
  link_with: tools_common_lib,
  include_directories: include_directories('.'),
)

tools_c_args = []
tools_deps = [libgpiod_dep, libgpiotools_dep, tools_common_dep]

if opt_gpioset_interactive.allowed() and libedit_dep.found()
  tools_c_args += ['-DGPIOSET_INTERACTIVE']
  tools_deps += [libedit_dep]
endif

tools = [
  'gpiodetect',
  'gpioinfo',
  'gpioget',
  'gpioset',
  'gpiomon',
  'gpionotify',
  'gpioctl'
]

tools_exes = {}
foreach tool : tools
  tools_exes += {tool: executable(tool,
    sources: [tool + '.c'],
    include_directories: libgpiod_inc,
    dependencies: tools_deps,
    c_args: tools_c_args,
    install: true,
  )}
endforeach
