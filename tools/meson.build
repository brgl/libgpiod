# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

tools_common_lib = static_library('tools-common',
  sources: ['tools-common.c'],
  include_directories: libgpiod_inc,
  pic: true,
)

tools_common_dep = declare_dependency(
  link_with: tools_common_lib,
  include_directories: include_directories('.'),
)

tools_c_args = []
tools_deps = [libgpiod_dep, tools_common_dep]

if opt_gpioset_interactive.allowed() and libedit_dep.found()
  tools_c_args += ['-DGPIOSET_INTERACTIVE']
  tools_deps += [libedit_dep]
endif

tools_exes = {}
foreach tool : ['gpiodetect', 'gpioinfo', 'gpioget', 'gpioset', 'gpiomon', 'gpionotify']
  tools_exes += {tool: executable(tool,
    sources: [tool + '.c'],
    include_directories: libgpiod_inc,
    dependencies: tools_deps,
    c_args: tools_c_args,
    install: true,
  )}
endforeach
