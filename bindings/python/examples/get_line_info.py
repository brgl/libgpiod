#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Minimal example of reading the info for a line."""

import gpiod


def get_line_info(chip_path: str, line_offset: int) -> None:
    with gpiod.Chip(chip_path) as chip:
        info = chip.get_line_info(line_offset)
        is_input = info.direction == gpiod.line.Direction.INPUT
        print(
            f"line {info.offset:>3}:"
            f" {info.name or 'unnamed':>12}"
            f" {info.consumer or 'unused':>12}"
            f" {'input' if is_input else 'output':>8}"
            f" {'active-low' if info.active_low else 'active-high':>10}"
        )


if __name__ == "__main__":
    try:
        get_line_info("/dev/gpiochip0", 3)
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
