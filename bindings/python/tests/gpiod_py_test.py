#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
#

import errno
import gpiod
import gpiomockup
import os
import select
import threading
import unittest

from packaging import version

mockup = None
default_consumer = 'gpiod-py-test'

class MockupTestCase(unittest.TestCase):

    chip_sizes = None
    flags = 0

    def setUp(self):
        mockup.probe(self.chip_sizes, flags=self.flags)

    def tearDown(self):
        mockup.remove()

class EventThread(threading.Thread):

    def __init__(self, chip_idx, line_offset, freq):
        threading.Thread.__init__(self)
        self.chip_idx = chip_idx
        self.line_offset = line_offset
        self.freq = freq
        self.lock = threading.Lock()
        self.cond = threading.Condition(self.lock)
        self.should_stop = False

    def run(self):
        i = 0;
        while True:
            with self.lock:
                if self.should_stop:
                    break;

                if not self.cond.wait(float(self.freq) / 1000):
                    mockup.chip_set_pull(self.chip_idx,
                                         self.line_offset, i % 2)
                    i += 1

    def stop(self):
        with self.lock:
            self.should_stop = True
            self.cond.notify_all()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.stop()
        self.join()

def check_kernel(major, minor, release):
    current = os.uname().release
    required = '{}.{}.{}'.format(major, minor, release)
    if version.parse(current) < version.parse(required):
        raise NotImplementedError(
                'linux kernel version must be at least {} - got {}'.format(required, current))

#
# Chip test cases
#

class ChipOpen(MockupTestCase):

    chip_sizes = ( 8, 8, 8 )

    def test_open_chip_by_name(self):
        with gpiod.Chip(mockup.chip_name(1), gpiod.Chip.OPEN_BY_NAME) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_open_chip_by_path(self):
        with gpiod.Chip(mockup.chip_path(1), gpiod.Chip.OPEN_BY_PATH) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_open_chip_by_num(self):
        with gpiod.Chip('{}'.format(mockup.chip_num(1)),
                        gpiod.Chip.OPEN_BY_NUMBER) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_open_chip_by_label(self):
        with gpiod.Chip('gpio-mockup-B', gpiod.Chip.OPEN_BY_LABEL) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_lookup_chip_by_name(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_lookup_chip_by_path(self):
        with gpiod.Chip(mockup.chip_path(1)) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_lookup_chip_by_num(self):
        with gpiod.Chip('{}'.format(mockup.chip_num(1))) as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_lookup_chip_by_label(self):
        with gpiod.Chip('gpio-mockup-B') as chip:
            self.assertEqual(chip.name(), mockup.chip_name(1))

    def test_nonexistent_chip(self):
        with self.assertRaises(FileNotFoundError):
            chip = gpiod.Chip('nonexistent-chip')

    def test_open_chip_no_arguments(self):
        with self.assertRaises(TypeError):
            chip = gpiod.Chip()

class ChipClose(MockupTestCase):

    chip_sizes = ( 8, )

    def test_use_chip_after_close(self):
        chip = gpiod.Chip(mockup.chip_name(0))
        self.assertEqual(chip.name(), mockup.chip_name(0))
        chip.close()
        with self.assertRaises(ValueError):
            chip.name()

class ChipInfo(MockupTestCase):

    chip_sizes = ( 16, )

    def test_chip_get_info(self):
        chip = gpiod.Chip(mockup.chip_name(0))
        self.assertEqual(chip.name(), mockup.chip_name(0))
        self.assertEqual(chip.label(), 'gpio-mockup-A')
        self.assertEqual(chip.num_lines(), 16)

class ChipGetLines(MockupTestCase):

    chip_sizes = ( 8, 8, 4 )
    flags = gpiomockup.Mockup.FLAG_NAMED_LINES

    def test_get_single_line_by_offset(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            line = chip.get_line(4)
            self.assertEqual(line.name(), 'gpio-mockup-B-4')

    def test_find_single_line_by_name(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            line = chip.find_line('gpio-mockup-B-4')
            self.assertEqual(line.offset(), 4)

    def test_get_single_line_invalid_offset(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            with self.assertRaises(OSError) as err_ctx:
                line = chip.get_line(11)

            self.assertEqual(err_ctx.exception.errno, errno.EINVAL)

    def test_find_single_line_nonexistent(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            line = chip.find_line('nonexistent-line')
            self.assertEqual(line, None)

    def test_get_multiple_lines_by_offsets_in_tuple(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            lines = chip.get_lines(( 1, 3, 6, 7 )).to_list()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0].name(), 'gpio-mockup-B-1')
            self.assertEqual(lines[1].name(), 'gpio-mockup-B-3')
            self.assertEqual(lines[2].name(), 'gpio-mockup-B-6')
            self.assertEqual(lines[3].name(), 'gpio-mockup-B-7')

    def test_get_multiple_lines_by_offsets_in_list(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            lines = chip.get_lines([ 1, 3, 6, 7 ]).to_list()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0].name(), 'gpio-mockup-B-1')
            self.assertEqual(lines[1].name(), 'gpio-mockup-B-3')
            self.assertEqual(lines[2].name(), 'gpio-mockup-B-6')
            self.assertEqual(lines[3].name(), 'gpio-mockup-B-7')

    def test_find_multiple_lines_by_names_in_tuple(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            lines = chip.find_lines(( 'gpio-mockup-B-0',
                                      'gpio-mockup-B-3',
                                      'gpio-mockup-B-4',
                                      'gpio-mockup-B-6' )).to_list()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0].offset(), 0)
            self.assertEqual(lines[1].offset(), 3)
            self.assertEqual(lines[2].offset(), 4)
            self.assertEqual(lines[3].offset(), 6)

    def test_find_multiple_lines_by_names_in_list(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            lines = chip.find_lines([ 'gpio-mockup-B-0',
                                      'gpio-mockup-B-3',
                                      'gpio-mockup-B-4',
                                      'gpio-mockup-B-6' ]).to_list()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0].offset(), 0)
            self.assertEqual(lines[1].offset(), 3)
            self.assertEqual(lines[2].offset(), 4)
            self.assertEqual(lines[3].offset(), 6)

    def test_get_multiple_lines_invalid_offset(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            with self.assertRaises(OSError) as err_ctx:
                line = chip.get_lines(( 1, 3, 11, 7 ))

            self.assertEqual(err_ctx.exception.errno, errno.EINVAL)

    def test_find_multiple_lines_nonexistent(self):
        with gpiod.Chip(mockup.chip_name(1)) as chip:
            with self.assertRaises(TypeError):
                lines = chip.find_lines(( 'gpio-mockup-B-0',
                                          'nonexistent-line',
                                          'gpio-mockup-B-4',
                                          'gpio-mockup-B-6' )).to_list()

    def test_get_all_lines(self):
        with gpiod.Chip(mockup.chip_name(2)) as chip:
            lines = chip.get_all_lines().to_list()
            self.assertEqual(len(lines), 4)
            self.assertEqual(lines[0].name(), 'gpio-mockup-C-0')
            self.assertEqual(lines[1].name(), 'gpio-mockup-C-1')
            self.assertEqual(lines[2].name(), 'gpio-mockup-C-2')
            self.assertEqual(lines[3].name(), 'gpio-mockup-C-3')

#
# Line test cases
#

class LineGlobalFindLine(MockupTestCase):

    chip_sizes = ( 4, 8, 16 )
    flags = gpiomockup.Mockup.FLAG_NAMED_LINES

    def test_global_find_line_function(self):
        line = gpiod.find_line('gpio-mockup-B-4')
        self.assertNotEqual(line, None)
        try:
            self.assertEqual(line.owner().label(), 'gpio-mockup-B')
            self.assertEqual(line.offset(), 4)
        finally:
            line.owner().close()

    def test_global_find_line_function_nonexistent(self):
        line = gpiod.find_line('nonexistent-line')
        self.assertEqual(line, None)

class LineInfo(MockupTestCase):

    chip_sizes = ( 8, )
    flags = gpiomockup.Mockup.FLAG_NAMED_LINES

    def test_unexported_line(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(4)
            self.assertEqual(line.offset(), 4)
            self.assertEqual(line.name(), 'gpio-mockup-A-4')
            self.assertEqual(line.direction(), gpiod.Line.DIRECTION_INPUT)
            self.assertEqual(line.active_state(), gpiod.Line.ACTIVE_HIGH)
            self.assertEqual(line.consumer(), None)
            self.assertFalse(line.is_used())
            self.assertFalse(line.is_requested())

    def test_exported_line(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(4)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_OUT,
                         flags=gpiod.LINE_REQ_FLAG_ACTIVE_LOW)
            self.assertEqual(line.offset(), 4)
            self.assertEqual(line.name(), 'gpio-mockup-A-4')
            self.assertEqual(line.direction(), gpiod.Line.DIRECTION_OUTPUT)
            self.assertEqual(line.active_state(), gpiod.Line.ACTIVE_LOW)
            self.assertEqual(line.consumer(), default_consumer)
            self.assertTrue(line.is_used())
            self.assertTrue(line.is_requested())

    def test_exported_line_with_flags(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(4)
            flags = (gpiod.LINE_REQ_FLAG_ACTIVE_LOW |
                     gpiod.LINE_REQ_FLAG_OPEN_DRAIN)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_OUT,
                         flags=flags)
            self.assertEqual(line.offset(), 4)
            self.assertEqual(line.name(), 'gpio-mockup-A-4')
            self.assertEqual(line.direction(), gpiod.Line.DIRECTION_OUTPUT)
            self.assertEqual(line.active_state(), gpiod.Line.ACTIVE_LOW)
            self.assertEqual(line.consumer(), default_consumer)
            self.assertTrue(line.is_used())
            self.assertTrue(line.is_requested())
            self.assertTrue(line.is_open_drain())
            self.assertFalse(line.is_open_source())

class LineValues(MockupTestCase):

    chip_sizes = ( 8, )

    def test_get_value_single_line(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_IN)
            self.assertEqual(line.get_value(), 0)
            mockup.chip_set_pull(0, 3, 1)
            self.assertEqual(line.get_value(), 1)

    def test_set_value_single_line(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_OUT)
            line.set_value(1)
            self.assertEqual(mockup.chip_get_value(0, 3), 1)
            line.set_value(0)
            self.assertEqual(mockup.chip_get_value(0, 3), 0)

    def test_set_value_with_default_value_argument(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_OUT,
                         default_val=1)
            self.assertEqual(mockup.chip_get_value(0, 3), 1)

    def test_get_value_multiple_lines(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            lines = chip.get_lines(( 0, 3, 4, 6 ))
            lines.request(consumer=default_consumer,
                          type=gpiod.LINE_REQ_DIR_IN)
            self.assertEqual(lines.get_values(), [ 0, 0, 0, 0 ])
            mockup.chip_set_pull(0, 0, 1)
            mockup.chip_set_pull(0, 4, 1)
            mockup.chip_set_pull(0, 6, 1)
            self.assertEqual(lines.get_values(), [ 1, 0, 1, 1 ])

    def test_set_value_multiple_lines(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            lines = chip.get_lines(( 0, 3, 4, 6 ))
            lines.request(consumer=default_consumer,
                          type=gpiod.LINE_REQ_DIR_OUT)
            lines.set_values(( 1, 0, 1, 1 ))
            self.assertEqual(mockup.chip_get_value(0, 0), 1)
            self.assertEqual(mockup.chip_get_value(0, 3), 0)
            self.assertEqual(mockup.chip_get_value(0, 4), 1)
            self.assertEqual(mockup.chip_get_value(0, 6), 1)
            lines.set_values(( 0, 0, 1, 0 ))
            self.assertEqual(mockup.chip_get_value(0, 0), 0)
            self.assertEqual(mockup.chip_get_value(0, 3), 0)
            self.assertEqual(mockup.chip_get_value(0, 4), 1)
            self.assertEqual(mockup.chip_get_value(0, 6), 0)

    def test_set_multiple_values_with_default_vals_argument(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            lines = chip.get_lines(( 0, 3, 4, 6 ))
            lines.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_OUT,
                         default_vals=( 1, 0, 1, 1 ))
            self.assertEqual(mockup.chip_get_value(0, 0), 1)
            self.assertEqual(mockup.chip_get_value(0, 3), 0)
            self.assertEqual(mockup.chip_get_value(0, 4), 1)
            self.assertEqual(mockup.chip_get_value(0, 6), 1)

    def test_get_value_active_low(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_IN,
                         flags=gpiod.LINE_REQ_FLAG_ACTIVE_LOW)
            self.assertEqual(line.get_value(), 1)
            mockup.chip_set_pull(0, 3, 1)
            self.assertEqual(line.get_value(), 0)

    def test_set_value_active_low(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_OUT,
                         flags=gpiod.LINE_REQ_FLAG_ACTIVE_LOW)
            line.set_value(1)
            self.assertEqual(mockup.chip_get_value(0, 3), 0)
            line.set_value(0)
            self.assertEqual(mockup.chip_get_value(0, 3), 1)

class LineRequestBehavior(MockupTestCase):

    chip_sizes = ( 8, )

    def test_line_export_release(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_IN)
            self.assertTrue(line.is_requested())
            self.assertEqual(line.get_value(), 0)
            line.release()
            self.assertFalse(line.is_requested())

    def test_line_request_twice_two_calls(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_IN)
            with self.assertRaises(OSError) as err_ctx:
                line.request(consumer=default_consumer,
                             type=gpiod.LINE_REQ_DIR_IN)

            self.assertEqual(err_ctx.exception.errno, errno.EBUSY)

    def test_line_request_twice_in_bulk(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            lines = chip.get_lines(( 2, 3, 6, 6 ))
            with self.assertRaises(OSError) as err_ctx:
                lines.request(consumer=default_consumer,
                              type=gpiod.LINE_REQ_DIR_IN)

            self.assertEqual(err_ctx.exception.errno, errno.EBUSY)

    def test_use_value_unrequested(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            with self.assertRaises(OSError) as err_ctx:
                line.get_value()

            self.assertEqual(err_ctx.exception.errno, errno.EPERM)

#
# Iterator test cases
#

class ChipIterator(MockupTestCase):

    chip_sizes = ( 4, 8, 16 )

    def test_iterate_over_chips(self):
        gotA = False
        gotB = False
        gotC = False

        for chip in gpiod.ChipIter():
            if chip.label() == 'gpio-mockup-A':
                gotA = True
            elif chip.label() == 'gpio-mockup-B':
                gotB = True
            elif chip.label() == 'gpio-mockup-C':
                gotC = True

        self.assertTrue(gotA)
        self.assertTrue(gotB)
        self.assertTrue(gotC)

class LineIterator(MockupTestCase):

    chip_sizes = ( 4, )

    def test_iterate_over_lines(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            count = 0

            for line in gpiod.LineIter(chip):
                self.assertEqual(line.offset(), count)
                count += 1

            self.assertEqual(count, chip.num_lines())

class LineBulkIter(MockupTestCase):

    chip_sizes = ( 4, )

    def test_line_bulk_iterator(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            lines = chip.get_all_lines()
            count = 0

            for line in lines:
                self.assertEqual(line.offset(), count)
                count += 1

            self.assertEqual(count, chip.num_lines())

#
# Event test cases
#

class EventSingleLine(MockupTestCase):

    chip_sizes = ( 8, )

    def test_single_line_rising_edge_event(self):
        with EventThread(0, 4, 200):
            with gpiod.Chip(mockup.chip_name(0)) as chip:
                line = chip.get_line(4)
                line.request(consumer=default_consumer,
                             type=gpiod.LINE_REQ_EV_RISING_EDGE)
                self.assertTrue(line.event_wait(sec=1))
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.RISING_EDGE)
                self.assertEqual(event.source.offset(), 4)

    def test_single_line_falling_edge_event(self):
        with EventThread(0, 4, 200):
            with gpiod.Chip(mockup.chip_name(0)) as chip:
                line = chip.get_line(4)
                line.request(consumer=default_consumer,
                             type=gpiod.LINE_REQ_EV_FALLING_EDGE)
                self.assertTrue(line.event_wait(sec=1))
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.FALLING_EDGE)
                self.assertEqual(event.source.offset(), 4)

    def test_single_line_both_edges_events(self):
        with EventThread(0, 4, 200):
            with gpiod.Chip(mockup.chip_name(0)) as chip:
                line = chip.get_line(4)
                line.request(consumer=default_consumer,
                             type=gpiod.LINE_REQ_EV_BOTH_EDGES)
                self.assertTrue(line.event_wait(sec=1))
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.RISING_EDGE)
                self.assertEqual(event.source.offset(), 4)
                self.assertTrue(line.event_wait(sec=1))
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.FALLING_EDGE)
                self.assertEqual(event.source.offset(), 4)

    def test_single_line_both_edges_events_active_low(self):
        with EventThread(0, 4, 200):
            with gpiod.Chip(mockup.chip_name(0)) as chip:
                line = chip.get_line(4)
                line.request(consumer=default_consumer,
                             type=gpiod.LINE_REQ_EV_BOTH_EDGES,
                             flags=gpiod.LINE_REQ_FLAG_ACTIVE_LOW)
                self.assertTrue(line.event_wait(sec=1))
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.FALLING_EDGE)
                self.assertEqual(event.source.offset(), 4)
                self.assertTrue(line.event_wait(sec=1))
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.RISING_EDGE)
                self.assertEqual(event.source.offset(), 4)

class EventBulk(MockupTestCase):

    chip_sizes = ( 8, )

    def test_watch_multiple_lines_for_events(self):
        with EventThread(0, 2, 200):
            with gpiod.Chip(mockup.chip_name(0)) as chip:
                lines = chip.get_lines(( 0, 1, 2, 3, 4 ))
                lines.request(consumer=default_consumer,
                              type=gpiod.LINE_REQ_EV_BOTH_EDGES)
                event_lines = lines.event_wait(sec=1)
                self.assertEqual(len(event_lines), 1)
                line = event_lines[0]
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.RISING_EDGE)
                self.assertEqual(event.source.offset(), 2)
                event_lines = lines.event_wait(sec=1)
                self.assertEqual(len(event_lines), 1)
                line = event_lines[0]
                event = line.event_read()
                self.assertEqual(event.type, gpiod.LineEvent.FALLING_EDGE)
                self.assertEqual(event.source.offset(), 2)

class EventValues(MockupTestCase):

    chip_sizes = ( 8, )

    def test_request_for_events_get_value(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_EV_BOTH_EDGES)
            self.assertEqual(line.get_value(), 0)
            mockup.chip_set_pull(0, 3, 1)
            self.assertEqual(line.get_value(), 1)

    def test_request_for_events_get_value_active_low(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_EV_BOTH_EDGES,
                         flags=gpiod.LINE_REQ_FLAG_ACTIVE_LOW)
            self.assertEqual(line.get_value(), 1)
            mockup.chip_set_pull(0, 3, 1)
            self.assertEqual(line.get_value(), 0)

class EventFileDescriptor(MockupTestCase):

    chip_sizes = ( 8, )

    def test_event_get_fd(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_EV_BOTH_EDGES)
            fd = line.event_get_fd();
            self.assertGreaterEqual(fd, 0)

    def test_event_get_fd_not_requested(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            with self.assertRaises(OSError) as err_ctx:
                fd = line.event_get_fd();

            self.assertEqual(err_ctx.exception.errno, errno.EPERM)

    def test_event_get_fd_requested_for_values(self):
        with gpiod.Chip(mockup.chip_name(0)) as chip:
            line = chip.get_line(3)
            line.request(consumer=default_consumer,
                         type=gpiod.LINE_REQ_DIR_IN)
            with self.assertRaises(OSError) as err_ctx:
                fd = line.event_get_fd();

            self.assertEqual(err_ctx.exception.errno, errno.EPERM)

    def test_event_fd_polling(self):
        with EventThread(0, 2, 200):
            with gpiod.Chip(mockup.chip_name(0)) as chip:
                lines = chip.get_lines(( 0, 1, 2, 3, 4, 5, 6 ))
                lines.request(consumer=default_consumer,
                              type=gpiod.LINE_REQ_EV_BOTH_EDGES)

                inputs = []
                for line in lines:
                    inputs.append(line.event_get_fd())

                readable, writable, exceptional = select.select(inputs, [],
                                                                inputs, 1.0)

                self.assertEqual(len(readable), 1)
                event = lines.to_list()[2].event_read()
                self.assertEqual(event.type, gpiod.LineEvent.RISING_EDGE)
                self.assertEqual(event.source.offset(), 2)

#
# Main
#

if __name__ == '__main__':
    check_kernel(5, 2, 11)
    mockup = gpiomockup.Mockup()
    unittest.main()
