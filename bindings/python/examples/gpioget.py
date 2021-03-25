#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""Simplified reimplementation of the gpioget tool in Python."""

import gpiod
import sys

from gpiod.line import Direction

if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise TypeError("usage: gpioget.py <gpiochip> <offset1> <offset2> ...")

    path = sys.argv[1]
    lines = [int(line) if line.isdigit() else line for line in sys.argv[2:]]

    request = gpiod.request_lines(
        path,
        consumer="gpioget.py",
        config={tuple(lines): gpiod.LineSettings(direction=Direction.INPUT)},
    )

    vals = request.get_values()

    for val in vals:
        print("{} ".format(val.value), end="")
    print()
