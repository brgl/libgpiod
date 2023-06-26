#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of reading the info for a line."""

import gpiod


def get_line_info(chip_path, line_offset):
    with gpiod.Chip(chip_path) as chip:
        info = chip.get_line_info(line_offset)
        is_input = info.direction == gpiod.line.Direction.INPUT
        print(
            "line {:>3}: {:>12} {:>12} {:>8} {:>10}".format(
                info.offset,
                info.name or "unnamed",
                info.consumer or "unused",
                "input" if is_input else "output",
                "active-low" if info.active_low else "active-high",
            )
        )


if __name__ == "__main__":
    try:
        get_line_info("/dev/gpiochip0", 3)
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
