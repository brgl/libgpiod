# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import gpiod
import time

from . import gpiosim
from datetime import timedelta
from functools import partial
from gpiod.line import Direction, Edge
from threading import Thread
from unittest import TestCase

EventType = gpiod.EdgeEvent.Type
Pull = gpiosim.Chip.Pull


class EdgeEventWaitTimeout(TestCase):
    def test_event_wait_timeout(self):
        sim = gpiosim.Chip()

        with gpiod.request_lines(
            sim.dev_path,
            {0: gpiod.LineSettings(edge_detection=Edge.BOTH)},
        ) as req:
            self.assertEqual(req.wait_edge_events(timedelta(microseconds=10000)), False)

    def test_event_wait_timeout_float(self):
        sim = gpiosim.Chip()

        with gpiod.request_lines(
            sim.dev_path,
            {0: gpiod.LineSettings(edge_detection=Edge.BOTH)},
        ) as req:
            self.assertEqual(req.wait_edge_events(0.01), False)


class EdgeEventInvalidConfig(TestCase):
    def test_output_mode_and_edge_detection(self):
        sim = gpiosim.Chip()

        with self.assertRaises(ValueError):
            gpiod.request_lines(
                sim.dev_path,
                {
                    0: gpiod.LineSettings(
                        direction=Direction.OUTPUT, edge_detection=Edge.BOTH
                    )
                },
            )


class WaitingForEdgeEvents(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)
        self.thread = None

    def tearDown(self):
        if self.thread:
            self.thread.join()
            del self.thread
        self.sim = None

    def trigger_falling_and_rising_edge(self, offset):
        time.sleep(0.05)
        self.sim.set_pull(offset, Pull.UP)
        time.sleep(0.05)
        self.sim.set_pull(offset, Pull.DOWN)

    def trigger_rising_edge_events_on_two_offsets(self, offset0, offset1):
        time.sleep(0.05)
        self.sim.set_pull(offset0, Pull.UP)
        time.sleep(0.05)
        self.sim.set_pull(offset1, Pull.UP)

    def test_both_edge_events(self):
        with gpiod.request_lines(
            self.sim.dev_path, {2: gpiod.LineSettings(edge_detection=Edge.BOTH)}
        ) as req:
            self.thread = Thread(
                target=partial(self.trigger_falling_and_rising_edge, 2)
            )
            self.thread.start()

            self.assertTrue(req.wait_edge_events(timedelta(seconds=1)))
            events = req.read_edge_events()
            self.assertEqual(len(events), 1)
            event = events[0]
            self.assertEqual(event.event_type, EventType.RISING_EDGE)
            self.assertEqual(event.line_offset, 2)
            ts_rising = event.timestamp_ns

            self.assertTrue(req.wait_edge_events(timedelta(seconds=1)))
            events = req.read_edge_events()
            self.assertEqual(len(events), 1)
            event = events[0]
            self.assertEqual(event.event_type, EventType.FALLING_EDGE)
            self.assertEqual(event.line_offset, 2)
            ts_falling = event.timestamp_ns

            self.assertGreater(ts_falling, ts_rising)

    def test_rising_edge_event(self):
        with gpiod.request_lines(
            self.sim.dev_path, {6: gpiod.LineSettings(edge_detection=Edge.RISING)}
        ) as req:
            self.thread = Thread(
                target=partial(self.trigger_falling_and_rising_edge, 6)
            )
            self.thread.start()

            self.assertTrue(req.wait_edge_events(timedelta(seconds=1)))
            events = req.read_edge_events()
            self.assertEqual(len(events), 1)
            event = events[0]
            self.assertEqual(event.event_type, EventType.RISING_EDGE)
            self.assertEqual(event.line_offset, 6)

            self.assertFalse(req.wait_edge_events(timedelta(microseconds=10000)))

    def test_rising_edge_event(self):
        with gpiod.request_lines(
            self.sim.dev_path, {6: gpiod.LineSettings(edge_detection=Edge.FALLING)}
        ) as req:
            self.thread = Thread(
                target=partial(self.trigger_falling_and_rising_edge, 6)
            )
            self.thread.start()

            self.assertTrue(req.wait_edge_events(timedelta(seconds=1)))
            events = req.read_edge_events()
            self.assertEqual(len(events), 1)
            event = events[0]
            self.assertEqual(event.event_type, EventType.FALLING_EDGE)
            self.assertEqual(event.line_offset, 6)

            self.assertFalse(req.wait_edge_events(timedelta(microseconds=10000)))

    def test_sequence_numbers(self):
        with gpiod.request_lines(
            self.sim.dev_path, {(2, 4): gpiod.LineSettings(edge_detection=Edge.BOTH)}
        ) as req:
            self.thread = Thread(
                target=partial(self.trigger_rising_edge_events_on_two_offsets, 2, 4)
            )
            self.thread.start()

            self.assertTrue(req.wait_edge_events(timedelta(seconds=1)))
            events = req.read_edge_events()
            self.assertEqual(len(events), 1)
            event = events[0]
            self.assertEqual(event.event_type, EventType.RISING_EDGE)
            self.assertEqual(event.line_offset, 2)
            self.assertEqual(event.global_seqno, 1)
            self.assertEqual(event.line_seqno, 1)

            self.assertTrue(req.wait_edge_events(timedelta(seconds=1)))
            events = req.read_edge_events()
            self.assertEqual(len(events), 1)
            event = events[0]
            self.assertEqual(event.event_type, EventType.RISING_EDGE)
            self.assertEqual(event.line_offset, 4)
            self.assertEqual(event.global_seqno, 2)
            self.assertEqual(event.line_seqno, 1)


class ReadingMultipleEdgeEvents(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)
        self.request = gpiod.request_lines(
            self.sim.dev_path, {1: gpiod.LineSettings(edge_detection=Edge.BOTH)}
        )
        self.line_seqno = 1
        self.global_seqno = 1
        self.sim.set_pull(1, Pull.UP)
        time.sleep(0.05)
        self.sim.set_pull(1, Pull.DOWN)
        time.sleep(0.05)
        self.sim.set_pull(1, Pull.UP)
        time.sleep(0.05)

    def tearDown(self):
        self.request.release()
        del self.request
        del self.sim

    def test_read_multiple_events(self):
        self.assertTrue(self.request.wait_edge_events(timedelta(seconds=1)))
        events = self.request.read_edge_events()
        self.assertEqual(len(events), 3)

        for event in events:
            self.assertEqual(event.line_offset, 1)
            self.assertEqual(event.line_seqno, self.line_seqno)
            self.assertEqual(event.global_seqno, self.global_seqno)
            self.line_seqno += 1
            self.global_seqno += 1


class EdgeEventStringRepresentation(TestCase):
    def test_edge_event_str(self):
        sim = gpiosim.Chip()

        with gpiod.request_lines(
            path=sim.dev_path, config={0: gpiod.LineSettings(edge_detection=Edge.BOTH)}
        ) as req:
            sim.set_pull(0, Pull.UP)
            event = req.read_edge_events()[0]
            self.assertRegex(
                str(event),
                "<EdgeEvent type=Type\.RISING_EDGE timestamp_ns=[0-9]+ line_offset=0 global_seqno=1 line_seqno=1>",
            )
