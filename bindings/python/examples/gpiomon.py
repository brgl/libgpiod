#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

'''Simplified reimplementation of the gpiomon tool in Python.'''

import gpiod
import sys

if __name__ == '__main__':
    def print_event(event):
        if event.type == gpiod.LineEvent.RISING_EDGE:
            evstr = ' RISING EDGE'
        elif event.type == gpiod.LineEvent.FALLING_EDGE:
            evstr = 'FALLING EDGE'
        else:
            raise TypeError('Invalid event type')

        print('event: {} offset: {} timestamp: [{}.{}]'.format(evstr,
                                                               event.source.offset(),
                                                               event.sec, event.nsec))

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
