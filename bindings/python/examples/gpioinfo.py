#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""Simplified reimplementation of the gpioinfo tool in Python."""

import gpiod
import os

from helpers import gpio_chips

if __name__ == "__main__":
    for chip in gpio_chips():
        cinfo = chip.get_info()
        print("{} - {} lines:".format(cinfo.name, cinfo.num_lines))

        for offset in range(0, cinfo.num_lines):
            linfo = chip.get_line_info(offset)
            is_input = linfo.direction == gpiod.line.Direction.INPUT
            print(
                "\tline {:>3}: {:>18} {:>12} {:>8} {:>10}".format(
                    linfo.offset,
                    linfo.name or "unnamed",
                    linfo.consumer or "unused",
                    "input" if is_input else "output",
                    "active-low" if linfo.active_low else "active-high",
                )
            )
