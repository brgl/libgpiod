#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

'''Misc tests of libgpiod python bindings.

These tests assume that at least one dummy gpiochip is present in the
system and that it's detected as gpiochip0.
'''

import gpiod
import sys
import select

test_cases = []

def add_test(name, func):
    global test_cases

    test_cases.append((name, func))

def fire_line_event(chip, offset, rising):
    path = '/sys/kernel/debug/gpio-mockup-event/{}/{}'.format(chip, offset)
    with open(path, 'w') as fd:
        fd.write('{}'.format(1 if rising else 0))

def print_event(event):
    print('type: {}'.format('rising' if event.type == gpiod.LineEvent.RISING_EDGE else 'falling'))
    print('timestamp: {}.{}'.format(event.sec, event.nsec))
    print('source line offset: {}'.format(event.source.offset()))

def chip_open_default_lookup():
    by_name = gpiod.Chip('gpiochip0')
    by_path = gpiod.Chip('/dev/gpiochip0')
    by_label = gpiod.Chip('gpio-mockup-A')
    by_number = gpiod.Chip('0')
    print('All good')

add_test('Open a GPIO chip using different lookup modes', chip_open_default_lookup)

def chip_open_different_modes():
    chip = gpiod.Chip('/dev/gpiochip0', gpiod.Chip.OPEN_BY_PATH)
    chip = gpiod.Chip('gpiochip0', gpiod.Chip.OPEN_BY_NAME)
    chip = gpiod.Chip('gpio-mockup-A', gpiod.Chip.OPEN_BY_LABEL)
    chip = gpiod.Chip('0', gpiod.Chip.OPEN_BY_NUMBER)
    print('All good')

add_test('Open a GPIO chip using different modes', chip_open_different_modes)

def chip_open_nonexistent():
    try:
        chip = gpiod.Chip('/nonexistent_gpiochip')
    except OSError as ex:
        print('Exception raised as expected: {}'.format(ex))
        return

    assert False, 'OSError expected'

add_test('Try to open a nonexistent GPIO chip', chip_open_nonexistent)

def chip_open_no_args():
    try:
        chip = gpiod.Chip()
    except TypeError:
        print('Error as expected')
        return

    assert False, 'TypeError expected'

add_test('Open a GPIO chip without arguments', chip_open_no_args)

def chip_info():
    chip = gpiod.Chip('gpiochip0')
    print('name: {}'.format(chip.name()))
    print('label: {}'.format(chip.label()))
    print('lines: {}'.format(chip.num_lines()))

add_test('Print chip info', chip_info)

def print_chip():
    chip = gpiod.Chip('/dev/gpiochip0')
    print(chip)

add_test('Print chip object', print_chip)

def create_chip_without_arguments():
    try:
        chip = gpiod.Chip()
    except TypeError as ex:
        print('Exception raised as expected: {}'.format(ex))
        return

    assert False, 'TypeError expected'

add_test('Create chip object without arguments', create_chip_without_arguments)

def create_line_object():
    try:
        line = gpiod.Line()
    except NotImplementedError:
        print('Error as expected')
        return

    assert False, 'NotImplementedError expected'

add_test('Create a line object - should fail', create_line_object)

def print_line():
    chip = gpiod.Chip('gpio-mockup-A')
    line = chip.get_line(3)
    print(line)

add_test('Print line object', print_line)

def find_line():
    line = gpiod.find_line('gpio-mockup-A-4')
    print('found line - offset: {}'.format(line.offset()))

add_test('Find line globally', find_line)

def create_empty_line_bulk():
    try:
        lines = gpiod.LineBulk()
    except TypeError:
        print('Error as expected')
        return

    assert False, 'TypeError expected'

add_test('Create a line bulk object - should fail', create_empty_line_bulk)

def get_lines():
    chip = gpiod.Chip('gpio-mockup-A')

    print('getting four lines from chip')
    lines = chip.get_lines([2, 4, 5, 7])

    print('Retrieved lines:')
    for line in lines:
        print(line)

add_test('Get lines from chip', get_lines)

def get_all_lines():
    chip = gpiod.Chip('gpio-mockup-A')

    print('Retrieving all lines from chip')
    lines = chip.get_all_lines()

    print('Retrieved lines:')
    for line in lines:
        print(line)

add_test('Get all lines from chip', get_all_lines)

def find_lines():
    chip = gpiod.Chip('gpiochip0')

    print('looking up lines by names')
    lines = chip.find_lines(['gpio-mockup-A-3', 'gpio-mockup-A-4', 'gpio-mockup-A-7'])

    print('Retrieved lines:')
    for line in lines:
        print(line)

add_test('Find multiple lines by name', find_lines)

def create_line_bulk_from_lines():
    chip = gpiod.Chip('gpio-mockup-A')
    line1 = chip.get_line(2)
    line2 = chip.get_line(4)
    line3 = chip.get_line(6)
    lines = gpiod.LineBulk([line1, line2, line3])
    print('Created LineBulk:')
    print(lines)

add_test('Create a LineBulk from a list of lines', create_line_bulk_from_lines)

def line_bulk_to_list():
    chip = gpiod.Chip('gpio-mockup-A')
    lines = chip.get_lines((1, 2, 3))
    print(lines.to_list())

add_test('Convert a LineBulk to a list', line_bulk_to_list)

def line_flags():
    chip = gpiod.Chip('gpiochip0')
    line = chip.get_line(3)

    print('line is used: {}'.format(line.is_used()))
    print('line is requested: {}'.format(line.is_requested()))

    print('requesting line')
    line.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_OUT,
                 flags=(gpiod.LINE_REQ_FLAG_OPEN_DRAIN | gpiod.LINE_REQ_FLAG_ACTIVE_LOW))

    print('line is used: {}'.format(line.is_used()))
    print('line is open drain: {}'.format(line.is_open_drain()))
    print('line is open source: {}'.format(line.is_open_source()))
    print('line is requested: {}'.format(line.is_requested()))
    print('line is active-low: {}'.format(
            "True" if line.active_state() == gpiod.Line.ACTIVE_LOW else "False"))

add_test('Check various line flags', line_flags)

def get_value_single_line():
    chip = gpiod.Chip('gpio-mockup-A')
    line = chip.get_line(2)
    line.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_IN)
    print('line value: {}'.format(line.get_value()))

add_test('Get value - single line', get_value_single_line)

def set_value_single_line():
    chip = gpiod.Chip('gpiochip0')
    line = chip.get_line(3)
    line.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_IN)

    print('line value before: {}'.format(line.get_value()))
    line.release()
    line.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_OUT)
    line.set_value(1)
    line.release()
    line.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_DIR_IN)
    print('line value after: {}'.format(line.get_value()))

add_test('Set value - single line', set_value_single_line)

def line_event_single_line():
    chip = gpiod.Chip('gpiochip0')
    line = chip.get_line(1)

    print('requesting line for events')
    line.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_EV_BOTH_EDGES)

    print('generating a line event')
    fire_line_event('gpiochip0', 1, True)
    assert line.event_wait(sec=1), 'Expected a line event to occur'

    print('event received')
    event = line.event_read()
    print_event(event)

add_test('Monitor a single line for events', line_event_single_line)

def line_event_multiple_lines():
    chip = gpiod.Chip('gpiochip0')
    lines = chip.get_lines((1, 2, 3, 4, 5))

    print('requesting lines for events')
    lines.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_EV_BOTH_EDGES)

    print('generating two line events')
    fire_line_event('gpiochip0', 1, True)
    fire_line_event('gpiochip0', 2, True)

    events = lines.event_wait(sec=1)
    assert events is not None and len(events) == 2, 'Expected to receive two line events'

    print('events received:')
    for line in events:
        event = line.event_read()
        print_event(event)

add_test('Monitor multiple lines for events', line_event_multiple_lines)

def line_event_poll_fd():
    chip = gpiod.Chip('gpiochip0')
    lines = chip.get_lines((1, 2, 3, 4, 5, 6))
    print('requesting lines for events')
    lines.request(consumer=sys.argv[0], type=gpiod.LINE_REQ_EV_BOTH_EDGES)

    print('generating three line events')
    fire_line_event('gpiochip0', 2, True)
    fire_line_event('gpiochip0', 3, False)
    fire_line_event('gpiochip0', 5, True)

    print('retrieving the file descriptors')
    inputs = []
    fd_mapping = {}
    for line in lines:
        inputs.append(line.event_get_fd())
        fd_mapping[line.event_get_fd()] = line

    readable, writable, exceptional = select.select(inputs, [], inputs, 1.0)
    assert len(readable) == 3, 'Expected to receive three line events'

    print('events received:')
    for fd in readable:
        line = fd_mapping[fd]
        event = line.event_read()
        print_event(event)

add_test('Monitor multiple lines using their file descriptors', line_event_poll_fd)

print('API version is {}'.format(gpiod.version_string()))

for name, func in test_cases:
    print('==============================================')
    print('{}:'.format(name))
    print()
    func()
