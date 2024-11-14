# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import datetime
import errno
import threading
import time
from dataclasses import FrozenInstanceError
from functools import partial
from unittest import TestCase

import gpiod
from gpiod.line import Direction

from . import gpiosim

_EventType = gpiod.InfoEvent.Type


class InfoEventDataclassBehavior(TestCase):
    def test_info_event_props_are_frozen(self) -> None:
        sim = gpiosim.Chip()

        with gpiod.Chip(sim.dev_path) as chip:
            chip.watch_line_info(0)
            with chip.request_lines(config={0: None}) as request:
                self.assertTrue(chip.wait_info_event(datetime.timedelta(seconds=1)))
                event = chip.read_info_event()

                with self.assertRaises(FrozenInstanceError):
                    event.event_type = 4

                with self.assertRaises(FrozenInstanceError):
                    event.timestamp_ns = 4

                with self.assertRaises(FrozenInstanceError):
                    event.line_info = 4


def request_reconfigure_release_line(chip_path: str, offset: int) -> None:
    time.sleep(0.1)
    with gpiod.request_lines(chip_path, config={offset: None}) as request:
        time.sleep(0.1)
        request.reconfigure_lines(
            config={offset: gpiod.LineSettings(direction=Direction.OUTPUT)}
        )
        time.sleep(0.1)


class WatchingInfoEventWorks(TestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(num_lines=8, line_names={4: "foobar"})
        self.chip = gpiod.Chip(self.sim.dev_path)
        self.thread = None

    def tearDown(self) -> None:
        if self.thread:
            self.thread.join()
            self.thread = None

        self.chip.close()
        self.chip = None
        self.sim = None

    def test_watch_line_info_returns_line_info(self) -> None:
        info = self.chip.watch_line_info(7)
        self.assertEqual(info.offset, 7)

    def test_watch_line_info_keyword_argument(self) -> None:
        info = self.chip.watch_line_info(line=7)

    def test_watch_line_info_offset_out_of_range(self) -> None:
        with self.assertRaises(ValueError):
            self.chip.watch_line_info(8)

    def test_watch_line_info_no_arguments(self) -> None:
        with self.assertRaises(TypeError):
            self.chip.watch_line_info()

    def test_watch_line_info_by_line_name(self) -> None:
        self.chip.watch_line_info("foobar")

    def test_watch_line_info_invalid_argument_type(self) -> None:
        with self.assertRaises(TypeError):
            self.chip.watch_line_info(None)

    def test_wait_for_event_timeout(self) -> None:
        info = self.chip.watch_line_info(7)
        self.assertFalse(
            self.chip.wait_info_event(datetime.timedelta(microseconds=10000))
        )

    def test_request_reconfigure_release_events(self) -> None:
        info = self.chip.watch_line_info(7)
        self.assertEqual(info.direction, Direction.INPUT)

        self.thread = threading.Thread(
            target=partial(request_reconfigure_release_line, self.sim.dev_path, 7)
        )
        self.thread.start()

        self.assertTrue(self.chip.wait_info_event(datetime.timedelta(seconds=1)))
        event = self.chip.read_info_event()
        self.assertEqual(event.event_type, _EventType.LINE_REQUESTED)
        self.assertEqual(event.line_info.offset, 7)
        self.assertEqual(event.line_info.direction, Direction.INPUT)
        ts_req = event.timestamp_ns

        # Check that we can use a float directly instead of datetime.timedelta.
        self.assertTrue(self.chip.wait_info_event(1.0))
        event = self.chip.read_info_event()
        self.assertEqual(event.event_type, _EventType.LINE_CONFIG_CHANGED)
        self.assertEqual(event.line_info.offset, 7)
        self.assertEqual(event.line_info.direction, Direction.OUTPUT)
        ts_rec = event.timestamp_ns

        self.assertTrue(self.chip.wait_info_event(datetime.timedelta(seconds=1)))
        event = self.chip.read_info_event()
        self.assertEqual(event.event_type, _EventType.LINE_RELEASED)
        self.assertEqual(event.line_info.offset, 7)
        self.assertEqual(event.line_info.direction, Direction.OUTPUT)
        ts_rel = event.timestamp_ns

        # No more events.
        self.assertFalse(
            self.chip.wait_info_event(datetime.timedelta(microseconds=10000))
        )

        # Check timestamps are really monotonic.
        self.assertGreater(ts_rel, ts_rec)
        self.assertGreater(ts_rec, ts_req)


class UnwatchingLineInfo(TestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(num_lines=8, line_names={4: "foobar"})
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.chip = None
        self.sim = None

    def test_unwatch_line_info(self) -> None:
        self.chip.watch_line_info(0)
        with self.chip.request_lines(config={0: None}) as request:
            self.assertTrue(self.chip.wait_info_event(datetime.timedelta(seconds=1)))
            event = self.chip.read_info_event()
            self.assertEqual(event.event_type, _EventType.LINE_REQUESTED)
            self.chip.unwatch_line_info(0)

        self.assertFalse(
            self.chip.wait_info_event(datetime.timedelta(microseconds=10000))
        )

    def test_unwatch_not_watched_line(self) -> None:
        with self.assertRaises(OSError) as ex:
            self.chip.unwatch_line_info(2)

        self.assertEqual(ex.exception.errno, errno.EBUSY)

    def test_unwatch_line_info_no_argument(self) -> None:
        with self.assertRaises(TypeError):
            self.chip.unwatch_line_info()

    def test_unwatch_line_info_by_line_name(self) -> None:
        self.chip.watch_line_info(4)
        with self.chip.request_lines(config={4: None}) as request:
            self.assertIsNotNone(self.chip.read_info_event())
            self.chip.unwatch_line_info("foobar")

        self.assertFalse(
            self.chip.wait_info_event(datetime.timedelta(microseconds=10000))
        )


class InfoEventStringRepresentation(TestCase):
    def test_info_event_str(self) -> None:
        sim = gpiosim.Chip()

        with gpiod.Chip(sim.dev_path) as chip:
            chip.watch_line_info(0)
            with chip.request_lines(config={0: None}) as request:
                self.assertTrue(chip.wait_info_event(datetime.timedelta(seconds=1)))
                event = chip.read_info_event()
                self.assertRegex(
                    str(event),
                    '<InfoEvent type=Type\\.LINE_REQUESTED timestamp_ns=[0-9]+ line_info=<LineInfo offset=0 name="None" used=True consumer="\\?" direction=Direction\\.INPUT active_low=False bias=Bias\\.UNKNOWN drive=Drive\\.PUSH_PULL edge_detection=Edge\\.NONE event_clock=Clock\\.MONOTONIC debounced=False debounce_period=0:00:00>>',
                )
