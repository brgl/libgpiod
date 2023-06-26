#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of reading multiple lines."""

import gpiod

from gpiod.line import Direction


def get_multiple_line_values(chip_path, line_offsets):
    with gpiod.request_lines(
        chip_path,
        consumer="get-multiple-line-values",
        config={tuple(line_offsets): gpiod.LineSettings(direction=Direction.INPUT)},
    ) as request:
        vals = request.get_values()

        for offset, val in zip(line_offsets, vals):
            print("{}={} ".format(offset, val), end="")
        print()


if __name__ == "__main__":
    try:
        get_multiple_line_values("/dev/gpiochip0", [5, 3, 7])
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
