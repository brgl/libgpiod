# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

install_data('io.gpiod1.conf',
  install_dir: get_option('sysconfdir') / 'dbus-1' / 'system.d',
)

if opt_systemd.allowed() and systemd_dep.found()
  service_conf = configuration_data()
  service_conf.set('prefix', get_option('prefix'))

  gpio_manager_service = configure_file(
    input: 'gpio-manager.service.in',
    output: 'gpio-manager.service',
    configuration: service_conf,
  )

  install_data(gpio_manager_service,
    install_dir: systemd_unit_dir,
  )
  install_data('90-gpio.rules',
    install_dir: get_option('libdir') / 'udev' / 'rules.d',
  )
endif
