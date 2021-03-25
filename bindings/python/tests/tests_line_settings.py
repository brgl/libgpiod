# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import gpiod

from . import gpiosim
from datetime import timedelta
from gpiod.line import Direction, Edge, Bias, Drive, Value, Clock
from unittest import TestCase


class LineSettingsConstructor(TestCase):
    def test_default_values(self):
        settings = gpiod.LineSettings()

        self.assertEqual(settings.direction, Direction.AS_IS)
        self.assertEqual(settings.edge_detection, Edge.NONE)
        self.assertEqual(settings.bias, Bias.AS_IS)
        self.assertEqual(settings.drive, Drive.PUSH_PULL)
        self.assertFalse(settings.active_low)
        self.assertEqual(settings.debounce_period.total_seconds(), 0.0)
        self.assertEqual(settings.event_clock, Clock.MONOTONIC)
        self.assertEqual(settings.output_value, Value.INACTIVE)

    def test_keyword_arguments(self):
        settings = gpiod.LineSettings(
            direction=Direction.INPUT,
            edge_detection=Edge.BOTH,
            bias=Bias.PULL_UP,
            event_clock=Clock.REALTIME,
        )

        self.assertEqual(settings.direction, Direction.INPUT)
        self.assertEqual(settings.edge_detection, Edge.BOTH)
        self.assertEqual(settings.bias, Bias.PULL_UP)
        self.assertEqual(settings.drive, Drive.PUSH_PULL)
        self.assertFalse(settings.active_low)
        self.assertEqual(settings.debounce_period.total_seconds(), 0.0)
        self.assertEqual(settings.event_clock, Clock.REALTIME)
        self.assertEqual(settings.output_value, Value.INACTIVE)


class LineSettingsAttributes(TestCase):
    def test_line_settings_attributes_are_mutable(self):
        settings = gpiod.LineSettings()

        settings.direction = Direction.INPUT
        settings.edge_detection = Edge.BOTH
        settings.bias = Bias.DISABLED
        settings.debounce_period = timedelta(microseconds=3000)
        settings.event_clock = Clock.HTE

        self.assertEqual(settings.direction, Direction.INPUT)
        self.assertEqual(settings.edge_detection, Edge.BOTH)
        self.assertEqual(settings.bias, Bias.DISABLED)
        self.assertEqual(settings.drive, Drive.PUSH_PULL)
        self.assertFalse(settings.active_low)
        self.assertEqual(settings.debounce_period.total_seconds(), 0.003)
        self.assertEqual(settings.event_clock, Clock.HTE)
        self.assertEqual(settings.output_value, Value.INACTIVE)


class LineSettingsStringRepresentation(TestCase):
    def setUp(self):
        self.settings = gpiod.LineSettings(
            direction=Direction.OUTPUT, drive=Drive.OPEN_SOURCE, active_low=True
        )

    def test_repr(self):
        self.assertEqual(
            repr(self.settings),
            "LineSettings(direction=Direction.OUTPUT, edge_detection=Edge.NONE bias=Bias.AS_IS drive=Drive.OPEN_SOURCE active_low=True debounce_period=datetime.timedelta(0) event_clock=Clock.MONOTONIC output_value=Value.INACTIVE)",
        )

    def test_str(self):
        self.assertEqual(
            str(self.settings),
            "<LineSettings direction=Direction.OUTPUT edge_detection=Edge.NONE bias=Bias.AS_IS drive=Drive.OPEN_SOURCE active_low=True debounce_period=0:00:00 event_clock=Clock.MONOTONIC output_value=Value.INACTIVE>",
        )
