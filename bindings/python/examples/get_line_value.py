#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of reading a single line."""

import gpiod

from gpiod.line import Direction


def get_line_value():
    # example configuration - customise to suit your situation
    chip_path = "/dev/gpiochip0"
    line_offset = 5

    with gpiod.request_lines(
        chip_path,
        consumer="get-line-value",
        config={line_offset: gpiod.LineSettings(direction=Direction.INPUT)},
    ) as request:
        value = request.get_value(line_offset)
        print(value)


if __name__ == "__main__":
    get_line_value()
