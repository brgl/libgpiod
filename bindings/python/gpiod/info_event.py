# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from dataclasses import dataclass
from enum import Enum

from . import _ext
from .line_info import LineInfo

__all__ = ["InfoEvent"]


@dataclass(frozen=True, init=False, repr=False)
class InfoEvent:
    """
    Immutable object containing data about a single line info event.
    """

    class Type(Enum):
        """Line status change event types."""

        LINE_REQUESTED = _ext.INFO_EVENT_TYPE_LINE_REQUESTED
        """Line has been requested."""
        LINE_RELEASED = _ext.INFO_EVENT_TYPE_LINE_RELEASED
        """Previously requested line has been released."""
        LINE_CONFIG_CHANGED = _ext.INFO_EVENT_TYPE_LINE_CONFIG_CHANGED
        """Line configuration has changed."""

    event_type: Type
    """Event type of the status change event."""
    timestamp_ns: int
    """Timestamp of the event."""
    line_info: LineInfo
    """Snapshot of line-info associated with the event."""

    def __init__(self, event_type: int, timestamp_ns: int, line_info: LineInfo):
        object.__setattr__(self, "event_type", InfoEvent.Type(event_type))
        object.__setattr__(self, "timestamp_ns", timestamp_ns)
        object.__setattr__(self, "line_info", line_info)

    def __str__(self) -> str:
        return f"<InfoEvent type={self.event_type} timestamp_ns={self.timestamp_ns} line_info={self.line_info}>"
