# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from . import _ext
from dataclasses import dataclass
from datetime import timedelta
from gpiod.line import Direction, Bias, Drive, Edge, Clock, Value

__all__ = "LineSettings"


@dataclass(repr=False)
class LineSettings:
    """
    Stores a set of line properties.
    """

    direction: Direction = Direction.AS_IS
    """Line direction."""
    edge_detection: Edge = Edge.NONE
    """Edge detection setting."""
    bias: Bias = Bias.AS_IS
    """Line bias setting."""
    drive: Drive = Drive.PUSH_PULL
    """Line drive setting."""
    active_low: bool = False
    """Active-low switch."""
    debounce_period: timedelta = timedelta()
    """Debounce period of the line."""
    event_clock: Clock = Clock.MONOTONIC
    """Edge event timestamping clock setting."""
    output_value: Value = Value.INACTIVE
    """Output value of the line."""

    # __repr__ generated by @dataclass uses repr for enum members resulting in
    # an unusable representation as those are of the form: <NAME: $value>
    def __repr__(self):
        return "gpiod.LineSettings(direction=gpiod.line.{}, edge_detection=gpiod.line.{}, bias=gpiod.line.{}, drive=gpiod.line.{}, active_low={}, debounce_period={}, event_clock=gpiod.line.{}, output_value=gpiod.line.{})".format(
            str(self.direction),
            str(self.edge_detection),
            str(self.bias),
            str(self.drive),
            self.active_low,
            repr(self.debounce_period),
            str(self.event_clock),
            str(self.output_value),
        )

    def __str__(self):
        return "<LineSettings direction={} edge_detection={} bias={} drive={} active_low={} debounce_period={} event_clock={} output_value={}>".format(
            self.direction,
            self.edge_detection,
            self.bias,
            self.drive,
            self.active_low,
            self.debounce_period,
            self.event_clock,
            self.output_value,
        )


def _line_settings_to_ext(settings: LineSettings) -> _ext.LineSettings:
    return _ext.LineSettings(
        direction=settings.direction.value,
        edge_detection=settings.edge_detection.value,
        bias=settings.bias.value,
        drive=settings.drive.value,
        active_low=settings.active_low,
        debounce_period=int(settings.debounce_period.total_seconds() * 1000000),
        event_clock=settings.event_clock.value,
        output_value=settings.output_value.value,
    )
