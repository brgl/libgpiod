# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

dbusdir = $(sysconfdir)/dbus-1/system.d/
dist_dbus_DATA = io.gpiod1.conf

if WITH_SYSTEMD

dist_systemdsystemunit_DATA = gpio-manager.service

udevdir = $(libdir)/udev/rules.d/
dist_udev_DATA = 90-gpio.rules

endif
