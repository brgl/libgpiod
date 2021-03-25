#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""Reimplementation of the gpiodetect tool in Python."""

import gpiod
import os

from helpers import gpio_chips

if __name__ == "__main__":
    for chip in gpio_chips():
        info = chip.get_info()
        print("{} [{}] ({} lines)".format(info.name, info.label, info.num_lines))
