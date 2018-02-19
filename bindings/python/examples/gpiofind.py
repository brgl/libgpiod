#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

'''Reimplementation of the gpiofind tool in Python.'''

import gpiod
import sys

line = gpiod.find_line(sys.argv[1])
print('{} {}'.format(line.owner().name(), line.offset()))
