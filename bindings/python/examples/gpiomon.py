#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""Simplified reimplementation of the gpiomon tool in Python."""

import gpiod
import sys

from gpiod.line import Edge

if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise TypeError("usage: gpiomon.py <gpiochip> <offset1> <offset2> ...")

    path = sys.argv[1]
    lines = [int(line) if line.isdigit() else line for line in sys.argv[2:]]

    with gpiod.request_lines(
        path,
        consumer="gpiomon.py",
        config={tuple(lines): gpiod.LineSettings(edge_detection=Edge.BOTH)},
    ) as request:
        while True:
            for event in request.read_edge_events():
                print(event)
