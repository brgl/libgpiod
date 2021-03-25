#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""Reimplementation of the gpiofind tool in Python."""

import gpiod
import os
import sys

if __name__ == "__main__":
    for entry in os.scandir("/dev/"):
        if gpiod.is_gpiochip_device(entry.path):
            with gpiod.Chip(entry.path) as chip:
                offset = chip.line_offset_from_id(sys.argv[1])
                if offset is not None:
                    print("{} {}".format(chip.get_info().name, offset))
                    sys.exit(0)

    sys.exit(1)
