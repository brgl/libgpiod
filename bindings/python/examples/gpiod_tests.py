#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

'''Misc tests of libgpiod python bindings.'''

import gpiod
import sys

test_cases = []

def add_test(name, func):
    global test_cases

    test_cases.append((name, func))

def chip_open():
    chip = gpiod.Chip('gpiochip0')
    chip = gpiod.Chip('/dev/gpiochip0', gpiod.Chip.OPEN_BY_PATH)
    chip = gpiod.Chip('gpiochip0', gpiod.Chip.OPEN_BY_NAME)
    chip = gpiod.Chip('gpio-mockup-A', gpiod.Chip.OPEN_BY_LABEL)
    chip = gpiod.Chip('0', gpiod.Chip.OPEN_BY_NUMBER)
    chip = gpiod.Chip('gpio-mockup-A', gpiod.Chip.OPEN_LOOKUP)
    print('All good')

add_test('Open a GPIO chip using different modes', chip_open)

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
    lines = chip.get_lines([2, 4, 5, 7])
    print('Retrieved lines:')
    for line in lines:
        print(line)

add_test('Get lines from chip', get_lines)

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

for name, func in test_cases:
    print('==============================================')
    print('{}:'.format(name))
    func()
