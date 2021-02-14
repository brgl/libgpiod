#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

'''Reimplementation of the gpiodetect tool in Python.'''

import gpiod
import os

if __name__ == '__main__':
    for entry in os.scandir('/dev/'):
        if gpiod.is_gpiochip_device(entry.path):
            with gpiod.Chip(entry.path) as chip:
                print('{} [{}] ({} lines)'.format(chip.name(),
                                                  chip.label(),
                                                  chip.num_lines()))
