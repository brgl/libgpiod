#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

'''Reimplementation of the gpiodetect tool in Python.'''

import gpiod

for chip in gpiod.ChipIter():
    print('{} [{}] ({} lines)'.format(chip.name(),
                                      chip.label(),
                                      chip.num_lines()))
