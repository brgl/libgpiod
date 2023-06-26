#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of watching for info changes on particular lines."""

import gpiod


def watch_line_info(chip_path, line_offsets):
    with gpiod.Chip(chip_path) as chip:
        for offset in line_offsets:
            chip.watch_line_info(offset)

        while True:
            print(chip.read_info_event())


if __name__ == "__main__":
    try:
        watch_line_info("/dev/gpiochip0", [5, 3, 7])
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
