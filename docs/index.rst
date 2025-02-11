..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   libgpiod documentation master file.

Welcome to libgpiod's documentation!
====================================

The **libgpiod** project provides a low-level C library, bindings to high-level
languages and tools for interacting with the GPIO (General Purpose Input/Output)
lines on Linux systems.

It replaces the older, **legacy GPIO sysfs interface**, which has been
deprecated in the Linux kernel. The newer **GPIO character device** interface
(introduced in **Linux kernel version 4.8**) provides a more flexible and
efficient way to interact with GPIO lines, and libgpiod is the **primary tool**
for working with this interface.

The character device interface guarantees all allocated resources are freed
after closing the device file descriptor and adds several new features that
are not present in the obsolete sysfs interface (like reliable event polling,
setting/reading multiple values at once or open-source and open-drain GPIOs).

Unfortunately interacting with the linux device file can no longer be done
using only standard command-line tools. This is the reason for creating a
library encapsulating the cumbersome, ioctl-based kernel-userspace interaction
in a set of convenient functions and opaque data structures.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   building
   core_api
   bindings
   gpio_tools
   dbus
   testing
   contributing
