..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   GPIO D-Bus API, daemon and command-line client documentation

D-Bus interface
===============

A commonly requested feature for the GPIO character device was state persistence
after releasing the lines (as a kernel feature) or providing a central authority
(in user-space) that would be in charge of keeping the lines requested and in a
certain state (similarily to how the sysfs ABI works). **GPIO D-Bus API** has
been provided to address this requirement. We define an interface covering the
majority of the GPIO chardev's functionality and implement it from both the
server and client sides in the form of the **gpio-manager** daemon and the
**gpiocli** command-line utility for talking to the manager. It enables
relatively efficient, asynchronous control of GPIO lines, offering methods for
configuring, monitoring, and manipulating GPIOs.

.. note::
   DBus support can be built by passing ``--enable-dbus`` to configure. The
   daemon is bundled with a systemd unit file and an example configuration file
   for the ``io.gpiod1`` interface that allows all users to access basic
   information about the GPIOs in the system but only root to request lines or
   change their values.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   dbus_api
   gpio-manager<gpio-manager>
   gpiocli_top
