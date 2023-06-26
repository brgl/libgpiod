#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of finding a line with the given name."""

import gpiod
import os


def generate_gpio_chips():
    for entry in os.scandir("/dev/"):
        if gpiod.is_gpiochip_device(entry.path):
            yield entry.path


def find_line_by_name(line_name):
    # Names are not guaranteed unique, so this finds the first line with
    # the given name.
    for path in generate_gpio_chips():
        with gpiod.Chip(path) as chip:
            try:
                offset = chip.line_offset_from_id(line_name)
                print("{}: {} {}".format(line_name, chip.get_info().name, offset))
                return
            except OSError:
                # An OSError is raised if the name is not found.
                continue

    print("line '{}' not found".format(line_name))


if __name__ == "__main__":
    try:
        find_line_by_name("GPIO19")
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
