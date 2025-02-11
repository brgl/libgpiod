# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>


from enum import Enum

from . import _ext

__all__ = ["Value", "Direction", "Bias", "Drive", "Edge", "Clock"]


class Value(Enum):
    """Logical line states."""

    INACTIVE = _ext.VALUE_INACTIVE
    """Line is logically inactive."""
    ACTIVE = _ext.VALUE_ACTIVE
    """Line is logically active."""

    def __bool__(self) -> bool:
        return self == self.ACTIVE


class Direction(Enum):
    """Direction settings."""

    AS_IS = _ext.DIRECTION_AS_IS
    """Request the line(s), but don't change direction."""
    INPUT = _ext.DIRECTION_INPUT
    """Direction is input - for reading the value of an externally driven GPIO line."""
    OUTPUT = _ext.DIRECTION_OUTPUT
    """Direction is output - for driving the GPIO line."""


class Bias(Enum):
    """Internal bias settings."""

    AS_IS = _ext.BIAS_AS_IS
    """Don't change the bias setting when applying line config."""
    UNKNOWN = _ext.BIAS_UNKNOWN
    """The internal bias state is unknown."""
    DISABLED = _ext.BIAS_DISABLED
    """The internal bias is disabled."""
    PULL_UP = _ext.BIAS_PULL_UP
    """The internal pull-up bias is enabled."""
    PULL_DOWN = _ext.BIAS_PULL_DOWN
    """The internal pull-down bias is enabled."""


class Drive(Enum):
    """Drive settings."""

    PUSH_PULL = _ext.DRIVE_PUSH_PULL
    """Drive setting is push-pull."""
    OPEN_DRAIN = _ext.DRIVE_OPEN_DRAIN
    """Line output is open-drain."""
    OPEN_SOURCE = _ext.DRIVE_OPEN_SOURCE
    """Line output is open-source."""


class Edge(Enum):
    """Edge detection settings."""

    NONE = _ext.EDGE_NONE
    """Line edge detection is disabled."""
    RISING = _ext.EDGE_RISING
    """Line detects rising edge events."""
    FALLING = _ext.EDGE_FALLING
    """Line detects falling edge events."""
    BOTH = _ext.EDGE_BOTH
    """Line detects both rising and falling edge events."""


class Clock(Enum):
    """Event clock settings."""

    MONOTONIC = _ext.CLOCK_MONOTONIC
    """Line uses the monotonic clock for edge event timestamps."""
    REALTIME = _ext.CLOCK_REALTIME
    """Line uses the realtime clock for edge event timestamps."""
    HTE = _ext.CLOCK_HTE
    """Line uses the hardware timestamp engine for event timestamps."""
