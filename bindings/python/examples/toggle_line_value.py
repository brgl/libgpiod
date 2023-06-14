#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of toggling a single line."""

import gpiod
import time

from gpiod.line import Direction, Value


def toggle_value(value):
    if value == Value.INACTIVE:
        return Value.ACTIVE
    return Value.INACTIVE


def print_value(value):
    if value == Value.ACTIVE:
        print("Active")
    else:
        print("Inactive")


def toggle_line_value():
    # example configuration - customise to suit your situation
    chip_path = "/dev/gpiochip0"
    line_offset = 5

    value = Value.ACTIVE

    request = gpiod.request_lines(
        chip_path,
        consumer="toggle-line-value",
        config={
            line_offset: gpiod.LineSettings(
                direction=Direction.OUTPUT, output_value=value
            )
        },
    )

    while True:
        print_value(value)
        time.sleep(1)
        value = toggle_value(value)
        request.set_value(line_offset, value)


if __name__ == "__main__":
    toggle_line_value()
