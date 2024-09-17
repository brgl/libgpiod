#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of asynchronously watching for edges on a single line."""

import gpiod
import select

from datetime import timedelta
from gpiod.line import Bias, Edge


def edge_type_str(event):
    if event.event_type is event.Type.RISING_EDGE:
        return "Rising"
    if event.event_type is event.Type.FALLING_EDGE:
        return "Falling"
    return "Unknown"


def async_watch_line_value(chip_path, line_offset, done_fd):
    # Assume a button connecting the pin to ground,
    # so pull it up and provide some debounce.
    with gpiod.request_lines(
        chip_path,
        consumer="async-watch-line-value",
        config={
            line_offset: gpiod.LineSettings(
                edge_detection=Edge.BOTH,
                bias=Bias.PULL_UP,
                debounce_period=timedelta(milliseconds=10),
            )
        },
    ) as request:
        poll = select.poll()
        poll.register(request.fd, select.POLLIN)
        # Other fds could be registered with the poll and be handled
        # separately using the return value (fd, event) from poll():
        poll.register(done_fd, select.POLLIN)
        while True:
            for fd, _event in poll.poll():
                if fd == done_fd:
                    # perform any cleanup before exiting...
                    return
                # handle any edge events
                for event in request.read_edge_events():
                    print(
                        "offset: {}  type: {:<7}  event #{}".format(
                            event.line_offset, edge_type_str(event), event.line_seqno
                        )
                    )


if __name__ == "__main__":
    import os
    import threading

    # run the async executor (select.poll) in a thread to demonstrate a graceful exit.
    done_fd = os.eventfd(0)

    def bg_thread():
        try:
            async_watch_line_value("/dev/gpiochip0", 5, done_fd)
        except OSError as ex:
            print(ex, "\nCustomise the example configuration to suit your situation")
        print("background thread exiting...")

    t = threading.Thread(target=bg_thread)
    t.start()

    # Wait for two minutes, unless bg_thread exits earlier, then graceful exit.
    t.join(timeout=120)
    if t.is_alive():
        os.eventfd_write(done_fd, 1)
        t.join()
    os.close(done_fd)
    print("main thread exiting...")
