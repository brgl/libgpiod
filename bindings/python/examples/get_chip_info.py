#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of reading the info for a chip."""

import gpiod


def get_chip_info(chip_path):
    with gpiod.Chip(chip_path) as chip:
        info = chip.get_info()
        print("{} [{}] ({} lines)".format(info.name, info.label, info.num_lines))


if __name__ == "__main__":
    try:
        get_chip_info("/dev/gpiochip0")
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
