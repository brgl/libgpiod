# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from dataclasses import dataclass
from datetime import timedelta

from .line import Bias, Clock, Direction, Drive, Edge

__all__ = ["LineInfo"]


@dataclass(frozen=True, init=False, repr=False)
class LineInfo:
    """
    Snapshot of a line's status.
    """

    offset: int
    name: str
    used: bool
    consumer: str
    direction: Direction
    active_low: bool
    bias: Bias
    drive: Drive
    edge_detection: Edge
    event_clock: Clock
    debounced: bool
    debounce_period: timedelta

    def __init__(
        self,
        offset: int,
        name: str,
        used: bool,
        consumer: str,
        direction: int,
        active_low: bool,
        bias: int,
        drive: int,
        edge_detection: int,
        event_clock: int,
        debounced: bool,
        debounce_period_us: int,
    ):
        object.__setattr__(self, "offset", offset)
        object.__setattr__(self, "name", name)
        object.__setattr__(self, "used", used)
        object.__setattr__(self, "consumer", consumer)
        object.__setattr__(self, "direction", Direction(direction))
        object.__setattr__(self, "active_low", active_low)
        object.__setattr__(self, "bias", Bias(bias))
        object.__setattr__(self, "drive", Drive(drive))
        object.__setattr__(self, "edge_detection", Edge(edge_detection))
        object.__setattr__(self, "event_clock", Clock(event_clock))
        object.__setattr__(self, "debounced", debounced)
        object.__setattr__(
            self, "debounce_period", timedelta(microseconds=debounce_period_us)
        )

    def __str__(self) -> str:
        return '<LineInfo offset={} name="{}" used={} consumer="{}" direction={} active_low={} bias={} drive={} edge_detection={} event_clock={} debounced={} debounce_period={}>'.format(
            self.offset,
            self.name,
            self.used,
            self.consumer,
            self.direction,
            self.active_low,
            self.bias,
            self.drive,
            self.edge_detection,
            self.event_clock,
            self.debounced,
            self.debounce_period,
        )
