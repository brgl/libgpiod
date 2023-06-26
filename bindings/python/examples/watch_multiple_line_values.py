#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of watching for edges on multiple lines."""

import gpiod

from gpiod.line import Edge


def edge_type_str(event):
    if event.event_type is event.Type.RISING_EDGE:
        return "Rising"
    if event.event_type is event.Type.FALLING_EDGE:
        return "Falling"
    return "Unknown"


def watch_multiple_line_values(chip_path, line_offsets):
    with gpiod.request_lines(
        chip_path,
        consumer="watch-multiple-line-values",
        config={tuple(line_offsets): gpiod.LineSettings(edge_detection=Edge.BOTH)},
    ) as request:
        while True:
            for event in request.read_edge_events():
                print(
                    "offset: {}  type: {:<7}  event #{}  line event #{}".format(
                        event.line_offset,
                        edge_type_str(event),
                        event.global_seqno,
                        event.line_seqno,
                    )
                )


if __name__ == "__main__":
    try:
        watch_multiple_line_values("/dev/gpiochip0", [5, 3, 7])
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
