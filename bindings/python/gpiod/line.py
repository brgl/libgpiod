# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>


from enum import Enum

from . import _ext

__all__ = ["Value", "Direction", "Bias", "Drive", "Edge", "Clock"]


class Value(Enum):
    """Logical line states."""

    INACTIVE = _ext.VALUE_INACTIVE
    ACTIVE = _ext.VALUE_ACTIVE

    def __bool__(self) -> bool:
        return self == self.ACTIVE


class Direction(Enum):
    """Direction settings."""

    AS_IS = _ext.DIRECTION_AS_IS
    INPUT = _ext.DIRECTION_INPUT
    OUTPUT = _ext.DIRECTION_OUTPUT


class Bias(Enum):
    """Internal bias settings."""

    AS_IS = _ext.BIAS_AS_IS
    UNKNOWN = _ext.BIAS_UNKNOWN
    DISABLED = _ext.BIAS_DISABLED
    PULL_UP = _ext.BIAS_PULL_UP
    PULL_DOWN = _ext.BIAS_PULL_DOWN


class Drive(Enum):
    """Drive settings."""

    PUSH_PULL = _ext.DRIVE_PUSH_PULL
    OPEN_DRAIN = _ext.DRIVE_OPEN_DRAIN
    OPEN_SOURCE = _ext.DRIVE_OPEN_SOURCE


class Edge(Enum):
    """Edge detection settings."""

    NONE = _ext.EDGE_NONE
    RISING = _ext.EDGE_RISING
    FALLING = _ext.EDGE_FALLING
    BOTH = _ext.EDGE_BOTH


class Clock(Enum):
    """Event clock settings."""

    MONOTONIC = _ext.CLOCK_MONOTONIC
    REALTIME = _ext.CLOCK_REALTIME
    HTE = _ext.CLOCK_HTE
