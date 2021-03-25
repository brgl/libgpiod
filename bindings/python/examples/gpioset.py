#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""Simplified reimplementation of the gpioset tool in Python."""

import gpiod
import sys

from gpiod.line import Direction, Value

if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise TypeError(
            "usage: gpioset.py <gpiochip> <offset1>=<value1> <offset2>=<value2> ..."
        )

    path = sys.argv[1]

    def parse_value(arg):
        x, y = arg.split("=")
        return (x, Value(int(y)))

    lvs = [parse_value(arg) for arg in sys.argv[2:]]
    lines = [x[0] for x in lvs]
    values = dict(lvs)

    request = gpiod.request_lines(
        path,
        consumer="gpioset.py",
        config={tuple(lines): gpiod.LineSettings(direction=Direction.OUTPUT)},
    )

    vals = request.set_values(values)

    input()
