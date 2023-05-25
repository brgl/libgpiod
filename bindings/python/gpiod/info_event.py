# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from . import _ext
from .line_info import LineInfo
from dataclasses import dataclass
from enum import Enum

__all__ = "InfoEvent"


@dataclass(frozen=True, init=False, repr=False)
class InfoEvent:
    """
    Immutable object containing data about a single line info event.
    """

    class Type(Enum):
        LINE_REQUESTED = _ext.INFO_EVENT_TYPE_LINE_REQUESTED
        LINE_RELEASED = _ext.INFO_EVENT_TYPE_LINE_RELEASED
        LINE_CONFIG_CHANGED = _ext.INFO_EVENT_TYPE_LINE_CONFIG_CHANGED

    event_type: Type
    timestamp_ns: int
    line_info: LineInfo

    def __init__(self, event_type: int, timestamp_ns: int, line_info: LineInfo):
        object.__setattr__(self, "event_type", InfoEvent.Type(event_type))
        object.__setattr__(self, "timestamp_ns", timestamp_ns)
        object.__setattr__(self, "line_info", line_info)

    def __str__(self):
        return "<InfoEvent type={} timestamp_ns={} line_info={}>".format(
            self.event_type, self.timestamp_ns, self.line_info
        )
