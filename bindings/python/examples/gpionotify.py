#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

"""Simplified reimplementation of the gpionotify tool in Python."""

import gpiod
import sys

if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise TypeError("usage: gpionotify.py <gpiochip> <offset1> <offset2> ...")

    path = sys.argv[1]

    with gpiod.Chip(path) as chip:
        for line in sys.argv[2:]:
            chip.watch_line_info(line)

        while True:
            print(chip.read_info_event())
