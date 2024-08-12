# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

EXTRA_DIST = gpio-manager.service

dbusdir = $(sysconfdir)/dbus-1/system.d/
dbus_DATA = io.gpiod1.conf

if WITH_SYSTEMD

systemdsystemunit_DATA = gpio-manager.service

udevdir = $(libdir)/udev/rules.d/
udev_DATA = 90-gpio.rules

endif
