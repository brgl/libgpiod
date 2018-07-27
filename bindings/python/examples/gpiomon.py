#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

'''Simplified reimplementation of the gpiomon tool in Python.'''

import gpiod
import sys

def print_event(event):
    if event.type == gpiod.LineEvent.RISING_EDGE:
        print(' RISING EDGE', end='')
    elif event.type == gpiod.LineEvent.FALLING_EDGE:
        print('FALLING EDGE', end='')
    else:
        raise TypeError('Invalid event type')

    print(' {}.{} line: {}'.format(event.sec, event.nsec, event.source.offset()))

if len(sys.argv) < 3:
    raise TypeError('usage: gpiomon.py <gpiochip> <offset1> <offset2> ...')

with gpiod.Chip(sys.argv[1]) as chip:
    offsets = []
    for off in sys.argv[2:]:
        offsets.append(int(off))

    lines = chip.get_lines(offsets)
    lines.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_EV_BOTH_EDGES)

    try:
        while True:
            ev_lines = lines.event_wait(sec=1)
            if ev_lines:
                for line in ev_lines:
                    event = line.event_read()
                    print_event(event)
    except KeyboardInterrupt:
        sys.exit(130)
