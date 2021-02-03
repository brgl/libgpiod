#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

'''Simplified reimplementation of the gpioget tool in Python.'''

import gpiod
import sys

if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise TypeError('usage: gpioget.py <gpiochip> <offset1> <offset2> ...')

    with gpiod.Chip(sys.argv[1]) as chip:
        offsets = []
        for off in sys.argv[2:]:
            offsets.append(int(off))

        lines = chip.get_lines(offsets)
        lines.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_IN)
        vals = lines.get_values()

        for val in vals:
            print(val, end=' ')
        print()
