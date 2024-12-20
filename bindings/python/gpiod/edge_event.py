# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from dataclasses import dataclass
from enum import Enum

from . import _ext

__all__ = ["EdgeEvent"]


@dataclass(frozen=True, init=False, repr=False)
class EdgeEvent:
    """
    Immutable object containing data about a single edge event.
    """

    class Type(Enum):
        """Possible edge event types."""

        RISING_EDGE = _ext.EDGE_EVENT_TYPE_RISING
        """Rising edge event."""
        FALLING_EDGE = _ext.EDGE_EVENT_TYPE_FALLING
        """Falling edge event."""

    event_type: Type
    """Edge event type."""
    timestamp_ns: int
    """Timestamp of the event in nanoseconds."""
    line_offset: int
    """Offset of the line on which this event was registered."""
    global_seqno: int
    """Global sequence number of this event."""
    line_seqno: int
    """Event sequence number specific to the concerned line."""

    def __init__(
        self,
        event_type: int,
        timestamp_ns: int,
        line_offset: int,
        global_seqno: int,
        line_seqno: int,
    ):
        object.__setattr__(self, "event_type", EdgeEvent.Type(event_type))
        object.__setattr__(self, "timestamp_ns", timestamp_ns)
        object.__setattr__(self, "line_offset", line_offset)
        object.__setattr__(self, "global_seqno", global_seqno)
        object.__setattr__(self, "line_seqno", line_seqno)

    def __str__(self) -> str:
        return "<EdgeEvent type={} timestamp_ns={} line_offset={} global_seqno={} line_seqno={}>".format(  # noqa: UP032
            self.event_type,
            self.timestamp_ns,
            self.line_offset,
            self.global_seqno,
            self.line_seqno,
        )
