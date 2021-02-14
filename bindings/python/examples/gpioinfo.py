#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

'''Simplified reimplementation of the gpioinfo tool in Python.'''

import gpiod
import os

if __name__ == '__main__':
    for entry in os.scandir('/dev/'):
        if gpiod.is_gpiochip_device(entry.path):
            with gpiod.Chip(entry.path) as chip:
                print('{} - {} lines:'.format(chip.name(), chip.num_lines()))

                for line in gpiod.LineIter(chip):
                    offset = line.offset()
                    name = line.name()
                    consumer = line.consumer()
                    direction = line.direction()
                    active_low = line.is_active_low()

                    print('\tline {:>3}: {:>18} {:>12} {:>8} {:>10}'.format(
                          offset,
                          'unnamed' if name is None else name,
                          'unused' if consumer is None else consumer,
                          'input' if direction == gpiod.Line.DIRECTION_INPUT else 'output',
                          'active-low' if active_low else 'active-high'))
