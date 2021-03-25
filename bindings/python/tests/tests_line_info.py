# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import errno
import gpiod
import unittest

from . import gpiosim
from gpiod.line import Direction, Bias, Drive, Clock

HogDir = gpiosim.Chip.Direction


class GetLineInfo(unittest.TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(
            num_lines=4,
            line_names={0: "foobar"},
        )

        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self):
        self.chip.close()
        self.chip = None
        self.sim = None

    def test_get_line_info_by_offset(self):
        self.chip.get_line_info(0)

    def test_get_line_info_by_offset_keyword(self):
        self.chip.get_line_info(line=0)

    def test_get_line_info_by_name(self):
        self.chip.get_line_info("foobar")

    def test_get_line_info_by_name_keyword(self):
        self.chip.get_line_info(line="foobar")

    def test_get_line_info_by_offset_string(self):
        self.chip.get_line_info("2")

    def test_offset_out_of_range(self):
        with self.assertRaises(ValueError) as ex:
            self.chip.get_line_info(4)

    def test_no_offset(self):
        with self.assertRaises(TypeError):
            self.chip.get_line_info()


class LinePropertiesCanBeRead(unittest.TestCase):
    def test_basic_properties(self):
        sim = gpiosim.Chip(
            num_lines=8,
            line_names={1: "foo", 2: "bar", 4: "baz", 5: "xyz"},
            hogs={3: ("hog3", HogDir.OUTPUT_HIGH), 4: ("hog4", HogDir.OUTPUT_LOW)},
        )

        with gpiod.Chip(sim.dev_path) as chip:
            info4 = chip.get_line_info(4)
            info6 = chip.get_line_info(6)

            self.assertEqual(info4.offset, 4)
            self.assertEqual(info4.name, "baz")
            self.assertTrue(info4.used)
            self.assertEqual(info4.consumer, "hog4")
            self.assertEqual(info4.direction, Direction.OUTPUT)
            self.assertFalse(info4.active_low)
            self.assertEqual(info4.bias, Bias.UNKNOWN)
            self.assertEqual(info4.drive, Drive.PUSH_PULL)
            self.assertEqual(info4.event_clock, Clock.MONOTONIC)
            self.assertFalse(info4.debounced)
            self.assertEqual(info4.debounce_period.total_seconds(), 0.0)

            self.assertEqual(info6.offset, 6)
            self.assertEqual(info6.name, None)
            self.assertFalse(info6.used)
            self.assertEqual(info6.consumer, None)
            self.assertEqual(info6.direction, Direction.INPUT)
            self.assertFalse(info6.active_low)
            self.assertEqual(info6.bias, Bias.UNKNOWN)
            self.assertEqual(info6.drive, Drive.PUSH_PULL)
            self.assertEqual(info6.event_clock, Clock.MONOTONIC)
            self.assertFalse(info6.debounced)
            self.assertEqual(info6.debounce_period.total_seconds(), 0.0)


class LineInfoStringRepresentation(unittest.TestCase):
    def test_line_info_str(self):
        sim = gpiosim.Chip(
            line_names={0: "foo"}, hogs={0: ("hogger", HogDir.OUTPUT_HIGH)}
        )

        with gpiod.Chip(sim.dev_path) as chip:
            info = chip.get_line_info(0)

            self.assertEqual(
                str(info),
                '<LineInfo offset=0 name="foo" used=True consumer="hogger" direction=Direction.OUTPUT active_low=False bias=Bias.UNKNOWN drive=Drive.PUSH_PULL edge_detection=Edge.NONE event_clock=Clock.MONOTONIC debounced=False debounce_period=0:00:00>',
            )
