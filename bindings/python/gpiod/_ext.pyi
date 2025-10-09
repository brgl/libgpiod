# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 Vincent Fazio <vfazio@gmail.com>

from typing import Optional

from .chip_info import ChipInfo
from .edge_event import EdgeEvent
from .info_event import InfoEvent
from .line import Value
from .line_info import LineInfo

class LineSettings:
    def __init__(
        self,
        direction: int,
        edge_detection: int,
        bias: int,
        drive: int,
        active_low: bool,
        debounce_period: int,
        event_clock: int,
        output_value: int,
    ) -> None: ...

class LineConfig:
    def __init__(self) -> None: ...
    def add_line_settings(self, offsets: list[int], settings: LineSettings) -> None: ...
    def set_output_values(self, global_output_values: list[Value]) -> None: ...

class Request:
    def release(self) -> None: ...
    def get_values(self, offsets: list[int], values: list[Value]) -> None: ...
    def set_values(self, values: dict[int, Value]) -> None: ...
    def reconfigure_lines(self, line_cfg: LineConfig) -> None: ...
    def read_edge_events(self, max_events: Optional[int]) -> list[EdgeEvent]: ...
    @property
    def chip_name(self) -> str: ...
    @property
    def num_lines(self) -> int: ...
    @property
    def offsets(self) -> list[int]: ...
    @property
    def fd(self) -> int: ...

class Chip:
    def __init__(self, path: str) -> None: ...
    def get_info(self) -> ChipInfo: ...
    def line_offset_from_id(self, id: str) -> int: ...
    def get_line_info(self, offset: int, watch: bool) -> LineInfo: ...
    def get_line_name(self, offset: int) -> Optional[str]: ...
    def request_lines(
        self,
        line_cfg: LineConfig,
        consumer: Optional[str],
        event_buffer_size: Optional[int],
    ) -> Request: ...
    def read_info_event(self) -> InfoEvent: ...
    def close(self) -> None: ...
    def unwatch_line_info(self, line: int) -> None: ...
    @property
    def path(self) -> str: ...
    @property
    def fd(self) -> int: ...

def is_gpiochip_device(path: str) -> bool: ...

api_version: str

# enum constants
BIAS_AS_IS: int
BIAS_DISABLED: int
BIAS_PULL_DOWN: int
BIAS_PULL_UP: int
BIAS_UNKNOWN: int
CLOCK_HTE: int
CLOCK_MONOTONIC: int
CLOCK_REALTIME: int
DIRECTION_AS_IS: int
DIRECTION_INPUT: int
DIRECTION_OUTPUT: int
DRIVE_OPEN_DRAIN: int
DRIVE_OPEN_SOURCE: int
DRIVE_PUSH_PULL: int
EDGE_BOTH: int
EDGE_EVENT_TYPE_FALLING: int
EDGE_EVENT_TYPE_RISING: int
EDGE_FALLING: int
EDGE_NONE: int
EDGE_RISING: int
INFO_EVENT_TYPE_LINE_CONFIG_CHANGED: int
INFO_EVENT_TYPE_LINE_RELEASED: int
INFO_EVENT_TYPE_LINE_REQUESTED: int
VALUE_ACTIVE: int
VALUE_INACTIVE: int
