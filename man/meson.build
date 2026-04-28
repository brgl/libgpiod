# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

man_template = files('template')
man1_dir = get_option('mandir') / 'man1'

if opt_tools.allowed()
  foreach tool : ['gpiodetect', 'gpioinfo', 'gpioget', 'gpioset', 'gpiomon', 'gpionotify']
    custom_target(tool + '.1',
      output: tool + '.1',
      command: [
        help2man,
        tools_exes[tool],
        '--include', man_template,
        '--no-info',
        '--name=libgpiod command-line utility',
        '--help-option=--help',
        '--manual=' + tool,
        '--output=@OUTPUT@',
      ],
      install: true,
      install_dir: man1_dir,
    )
  endforeach
endif

if opt_dbus.allowed()
  gpiocli_cmds = [
    'detect', 'find', 'info', 'get', 'monitor', 'notify',
    'reconfigure', 'release', 'request', 'requests', 'set', 'wait',
  ]

  custom_target('gpio-manager.1',
    output: 'gpio-manager.1',
    command: [
      help2man,
      gpio_manager_exe,
      '--include', man_template,
      '--no-info',
      '--name=libgpiod D-Bus daemon',
      '--help-option=--help',
      '--manual=gpio-manager',
      '--output=@OUTPUT@',
    ],
    install: true,
    install_dir: man1_dir,
  )

  custom_target('gpiocli.1',
    output: 'gpiocli.1',
    command: [
      help2man,
      gpiocli_exe,
      '--include', man_template,
      '--no-info',
      '--name=libgpiod command-line utility',
      '--help-option=--help',
      '--manual=gpiocli',
      '--output=@OUTPUT@',
    ],
    install: true,
    install_dir: man1_dir,
  )

  foreach cmd : gpiocli_cmds
    custom_target('gpiocli-' + cmd + '.1',
      output: 'gpiocli-' + cmd + '.1',
      command: [
        help2man,
        gpiocli_exe,
        '--include', man_template,
        '--no-info',
        '--name=' + cmd,
        '--help-option=' + cmd + ' --help',
        '--manual=gpiocli-' + cmd,
        '--output=@OUTPUT@',
      ],
      install: true,
      install_dir: man1_dir,
    )
  endforeach
endif
