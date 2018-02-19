#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

'''Simplified reimplementation of the gpioset tool in Python.'''

import gpiod
import sys

if len(sys.argv) < 3:
    raise TypeError('usage: gpioset.py <gpiochip> <offset1>=<value1> ...')

chip = gpiod.Chip(sys.argv[1])

offsets = []
values = []
for arg in sys.argv[2:]:
    arg = arg.split('=')
    offsets.append(int(arg[0]))
    values.append(int(arg[1]))

lines = chip.get_lines(offsets)
lines.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_OUT)
vals = lines.set_values(values)
