#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of watching for edges on a single line."""

import gpiod

from datetime import timedelta
from gpiod.line import Bias, Edge


def edge_type(event):
    if event.event_type is event.Type.RISING_EDGE:
        return "Rising "
    if event.event_type is event.Type.FALLING_EDGE:
        return "Falling"
    return "Unknown"


def watch_line_value():
    # example configuration - customise to suit your situation
    chip_path = "/dev/gpiochip0"
    line_offset = 5

    # assume a button connecting the pin to ground,
    # so pull it up and provide some debounce.
    with gpiod.request_lines(
        chip_path,
        consumer="watch-line-value",
        config={
            line_offset: gpiod.LineSettings(
                edge_detection=Edge.BOTH,
                bias=Bias.PULL_UP,
                debounce_period=timedelta(milliseconds=10),
            )
        },
    ) as request:
        while True:
            # blocks until at least one event is available
            for event in request.read_edge_events():
                print(
                    "offset: %d, type: %s, event #%d"
                    % (event.line_offset, edge_type(event), event.line_seqno)
                )


if __name__ == "__main__":
    watch_line_value()
