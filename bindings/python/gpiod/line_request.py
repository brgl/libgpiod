# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from __future__ import annotations

from typing import TYPE_CHECKING, Optional, Union, cast

from . import _ext
from ._internal import config_iter, poll_fd
from .exception import RequestReleasedError
from .line_settings import LineSettings, _line_settings_to_ext

if TYPE_CHECKING:
    from collections.abc import Iterable
    from datetime import timedelta
    from types import TracebackType

    from .edge_event import EdgeEvent
    from .line import Value


__all__ = ["LineRequest"]


class LineRequest:
    """
    Stores the context of a set of requested GPIO lines.
    """

    def __init__(self, req: _ext.Request):
        """
        Note: LineRequest objects can only be instantiated by a Chip parent.
        LineRequest.__init__() is not part of stable API.
        """
        self._req: Union[_ext.Request, None] = req
        self._chip_name: str
        self._offsets: list[int]
        self._name_map: dict[str, int]
        self._offset_map: dict[int, str]
        self._lines: list[Union[int, str]]

    def __bool__(self) -> bool:
        """
        Boolean conversion for GPIO line requests.

        Returns:
          True if the request is live and False if it's been released.
        """
        return True if self._req else False

    def __enter__(self) -> LineRequest:
        """
        Controlled execution enter callback.
        """
        self._check_released()
        return self

    def __exit__(
        self,
        exc_type: Optional[type[BaseException]],
        exc_value: Optional[BaseException],
        traceback: Optional[TracebackType],
    ) -> None:
        """
        Controlled execution exit callback.
        """
        self.release()

    def _check_released(self) -> None:
        if not self._req:
            raise RequestReleasedError()

    def release(self) -> None:
        """
        Release this request and free all associated resources. The object must
        not be used after a call to this method.
        """
        self._check_released()
        cast("_ext.Request", self._req).release()
        self._req = None

    def get_value(self, line: Union[int, str]) -> Value:
        """
        Get a single GPIO line value.

        Args:
          line:
            Offset or name of the line to get value for.

        Returns:
          Logical value of the line.
        """
        return self.get_values([line])[0]

    def _line_to_offset(self, line: Union[int, str]) -> int:
        if isinstance(line, int):
            return line
        else:
            _line: Union[int, None]
            if (_line := self._name_map.get(line)) is None:
                raise ValueError(f"unknown line name: {line}")
            else:
                return _line

    def get_values(
        self, lines: Optional[Iterable[Union[int, str]]] = None
    ) -> list[Value]:
        """
        Get values of a set of GPIO lines.

        Args:
          lines:
            List of names or offsets of GPIO lines to get values for. Can be
            None in which case all requested lines will be read.

        Returns:
          List of logical line values.
        """
        self._check_released()

        lines = lines or self._lines

        offsets = [self._line_to_offset(line) for line in lines]

        buf = cast("list[Value]", [None] * len(offsets))

        cast("_ext.Request", self._req).get_values(offsets, buf)
        return buf

    def set_value(self, line: Union[int, str], value: Value) -> None:
        """
        Set the value of a single GPIO line.

        Args:
          line:
            Offset or name of the line to set.
          value:
            New value.
        """
        self.set_values({line: value})

    def set_values(self, values: dict[Union[int, str], Value]) -> None:
        """
        Set the values of a subset of GPIO lines.

        Args:
          values:
            Dictionary mapping line offsets or names to desired values.
        """
        self._check_released()

        mapped = {self._line_to_offset(line): value for line, value in values.items()}

        cast("_ext.Request", self._req).set_values(mapped)

    def reconfigure_lines(
        self,
        config: dict[
            Union[Iterable[Union[int, str]], int, str], Optional[LineSettings]
        ],
    ) -> None:
        """
        Reconfigure requested lines.

        Args:
          config
            Dictionary mapping offsets or names (or tuples thereof) to
            LineSettings. If no entry exists, or a None is passed as the
            settings, then the configuration for that line is not changed.
            Any settings for non-requested lines are ignored.
        """
        self._check_released()

        line_cfg = _ext.LineConfig()
        line_settings = {}

        for line, settings in config_iter(config):
            offset = self._line_to_offset(line)
            line_settings[offset] = settings

        for offset in self.offsets:
            settings = line_settings.get(offset) or LineSettings()
            line_cfg.add_line_settings([offset], _line_settings_to_ext(settings))

        cast("_ext.Request", self._req).reconfigure_lines(line_cfg)

    def wait_edge_events(
        self, timeout: Optional[Union[timedelta, float]] = None
    ) -> bool:
        """
        Wait for edge events on any of the requested lines.

        Args:
          timeout:
            Wait time limit expressed as either a datetime.timedelta object
            or the number of seconds stored in a float. If set to 0, the
            method returns immediately, if set to None it blocks indefinitely.

        Returns:
          True if events are ready to be read. False on timeout.
        """
        self._check_released()

        return poll_fd(self.fd, timeout)

    def read_edge_events(self, max_events: Optional[int] = None) -> list[EdgeEvent]:
        """
        Read a number of edge events from a line request.

        Args:
          max_events:
            Maximum number of events to read.

        Returns:
          List of read EdgeEvent objects.
        """
        self._check_released()

        return cast("_ext.Request", self._req).read_edge_events(max_events)

    def fileno(self) -> int:
        """
        Return the underlying file descriptor.
        """
        return self.fd

    def __str__(self) -> str:
        """
        Return a user-friendly, human-readable description of this request.
        """
        if not self._req:
            return "<LineRequest RELEASED>"

        return f'<LineRequest chip="{self.chip_name}" num_lines={self.num_lines} offsets={self.offsets} fd={self.fd}>'

    @property
    def chip_name(self) -> str:
        """
        Name of the chip this request was made on.
        """
        self._check_released()
        return self._chip_name

    @property
    def num_lines(self) -> int:
        """
        Number of requested lines.
        """
        self._check_released()
        return len(self._offsets)

    @property
    def offsets(self) -> list[int]:
        """
        List of requested offsets. Lines requested by name are mapped to their
        offsets.
        """
        self._check_released()
        return self._offsets

    @property
    def lines(self) -> list[Union[int, str]]:
        """
        List of requested lines. Lines requested by name are listed as such.
        """
        self._check_released()
        return self._lines

    @property
    def fd(self) -> int:
        """
        File descriptor associated with this request.
        """
        self._check_released()
        return cast("_ext.Request", self._req).fd
