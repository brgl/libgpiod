#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

'''Reimplementation of the gpiofind tool in Python.'''

import gpiod
import os
import sys

if __name__ == '__main__':
    for entry in os.scandir('/dev/'):
        if gpiod.is_gpiochip_device(entry.path):
            with gpiod.Chip(entry.path) as chip:
                lines = chip.find_line(sys.argv[1], unique=True)
                if lines is not None:
                     line = lines.to_list()[0]
                     print('{} {}'.format(line.owner().name(), line.offset()))
                     sys.exit(0)

    sys.exit(1)
