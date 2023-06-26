#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of watching for rising edges on a single line."""

import gpiod

from gpiod.line import Edge


def watch_line_rising(chip_path, line_offset):
    with gpiod.request_lines(
        chip_path,
        consumer="watch-line-rising",
        config={line_offset: gpiod.LineSettings(edge_detection=Edge.RISING)},
    ) as request:
        while True:
            # Blocks until at least one event is available
            for event in request.read_edge_events():
                print(
                    "line: {}  type: Rising   event #{}".format(
                        event.line_offset, event.line_seqno
                    )
                )


if __name__ == "__main__":
    try:
        watch_line_rising("/dev/gpiochip0", 5)
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
