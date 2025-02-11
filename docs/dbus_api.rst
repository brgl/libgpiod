..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   GPIO D-Bus API documentation

D-Bus API
=========

The following set of strictly defined interfaces allow users to use any
**D-Bus** library in order to interact with the **gpio-manager** as well as
reimplement the manager itself if required.

.. toctree::
   :maxdepth: 1
   :caption: Interfaces

   dbus-io.gpiod1.Chip
   dbus-io.gpiod1.Line
   dbus-io.gpiod1.Request
