# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import gpiod
import os


def gpio_chips():
    for path in [
        entry.path
        for entry in os.scandir("/dev/")
        if gpiod.is_gpiochip_device(entry.path)
    ]:
        with gpiod.Chip(path) as chip:
            yield chip
