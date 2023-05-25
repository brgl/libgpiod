# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from . import _ext
from .edge_event import EdgeEvent
from .exception import RequestReleasedError
from .internal import poll_fd
from .line import Value
from .line_settings import LineSettings, _line_settings_to_ext
from collections.abc import Iterable
from datetime import timedelta
from typing import Optional, Union

__all__ = "LineRequest"


class LineRequest:
    """
    Stores the context of a set of requested GPIO lines.
    """

    def __init__(self, req: _ext.Request):
        """
        DON'T USE

        LineRequest objects can only be instantiated by a Chip parent. This is
        not part of stable API.
        """
        self._req = req

    def __bool__(self) -> bool:
        """
        Boolean conversion for GPIO line requests.

        Returns:
          True if the request is live and False if it's been released.
        """
        return True if self._req else False

    def __enter__(self):
        """
        Controlled execution enter callback.
        """
        self._check_released()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
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
        self._req.release()
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

    def _check_line_name(self, line):
        if isinstance(line, str):
            if line not in self._name_map:
                raise ValueError("unknown line name: {}".format(line))

            return True

        return False

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

        offsets = [
            self._name_map[line] if self._check_line_name(line) else line
            for line in lines
        ]

        buf = [None] * len(lines)

        self._req.get_values(offsets, buf)
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

        mapped = {
            self._name_map[line] if self._check_line_name(line) else line: values[line]
            for line in values
        }

        self._req.set_values(mapped)

    def reconfigure_lines(
        self, config: dict[tuple[Union[int, str]], LineSettings]
    ) -> None:
        """
        Reconfigure requested lines.

        Args:
          config
            Dictionary mapping offsets or names (or tuples thereof) to
            LineSettings. If None is passed as the value of the mapping,
            default settings are used.
        """
        self._check_released()

        line_cfg = _ext.LineConfig()

        for lines, settings in config.items():
            if isinstance(lines, int) or isinstance(lines, str):
                lines = [lines]

            offsets = [
                self._name_map[line] if self._check_line_name(line) else line
                for line in lines
            ]

            line_cfg.add_line_settings(offsets, _line_settings_to_ext(settings))

        self._req.reconfigure_lines(line_cfg)

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

        return self._req.read_edge_events(max_events)

    def __str__(self):
        """
        Return a user-friendly, human-readable description of this request.
        """
        if not self._req:
            return "<LineRequest RELEASED>"

        return "<LineRequest num_lines={} offsets={} fd={}>".format(
            self.num_lines, self.offsets, self.fd
        )

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
        return self._req.fd
