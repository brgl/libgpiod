# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from . import _ext
from .chip_info import ChipInfo
from .exception import ChipClosedError
from .info_event import InfoEvent
from .internal import poll_fd
from .line import Value
from .line_info import LineInfo
from .line_settings import LineSettings, _line_settings_to_ext
from .line_request import LineRequest
from collections import Counter
from collections.abc import Iterable
from datetime import timedelta
from errno import ENOENT
from select import select
from typing import Union, Optional

__all__ = "Chip"


class Chip:
    """
    Represents a GPIO chip.

    Chip object manages all resources associated with the GPIO chip it represents.

    The gpiochip device file is opened during the object's construction. The Chip
    object's constructor takes the path to the GPIO chip device file
    as the only argument.

    Callers must close the chip by calling the close() method when it's no longer
    used.

    Example:

        chip = gpiod.Chip(\"/dev/gpiochip0\")
        do_something(chip)
        chip.close()

    The gpiod.Chip class also supports controlled execution ('with' statement).

    Example:

        with gpiod.Chip(path="/dev/gpiochip0") as chip:
            do_something(chip)
    """

    def __init__(self, path: str):
        """
        Open a GPIO device.

        Args:
          path:
            Path to the GPIO character device file.
        """
        self._chip = _ext.Chip(path)
        self._info = None

    def __bool__(self) -> bool:
        """
        Boolean conversion for GPIO chips.

        Returns:
          True if the chip is open and False if it's closed.
        """
        return True if self._chip else False

    def __enter__(self):
        """
        Controlled execution enter callback.
        """
        self._check_closed()
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        """
        Controlled execution exit callback.
        """
        self.close()

    def _check_closed(self) -> None:
        if not self._chip:
            raise ChipClosedError()

    def close(self) -> None:
        """
        Close the associated GPIO chip descriptor. The chip object must no
        longer be used after this method is called.
        """
        self._check_closed()
        self._chip.close()
        self._chip = None

    def get_info(self) -> ChipInfo:
        """
        Get the information about the chip.

        Returns:
          New gpiod.ChipInfo object.
        """
        self._check_closed()

        if not self._info:
            self._info = self._chip.get_info()

        return self._info

    def line_offset_from_id(self, id: Union[str, int]) -> int:
        """
        Map a line's identifier to its offset within the chip.

        Args:
          id:
            Name of the GPIO line, its offset as a string or its offset as an
            integer.

        Returns:
          If id is an integer - it's returned as is (unless it's out of range
          for this chip). If it's a string, the method tries to interpret it as
          the name of the line first and tries too perform a name lookup within
          the chip. If it fails, it tries to convert the string to an integer
          and check if it represents a valid offset within the chip and if
          so - returns it.
        """
        self._check_closed()

        if not isinstance(id, int):
            try:
                return self._chip.line_offset_from_id(id)
            except OSError as ex:
                if ex.errno == ENOENT:
                    try:
                        offset = int(id)
                    except ValueError:
                        raise ex
                else:
                    raise ex
        else:
            offset = id

        if offset >= self.get_info().num_lines:
            raise ValueError("line offset of out range")

        return offset

    def _get_line_info(self, line: Union[int, str], watch: bool) -> LineInfo:
        self._check_closed()
        return self._chip.get_line_info(self.line_offset_from_id(line), watch)

    def get_line_info(self, line: Union[int, str]) -> LineInfo:
        """
        Get the snapshot of information about the line at given offset.

        Args:
          line:
            Offset or name of the GPIO line to get information for.

        Returns:
          New LineInfo object.
        """
        return self._get_line_info(line, watch=False)

    def watch_line_info(self, line: Union[int, str]) -> LineInfo:
        """
        Get the snapshot of information about the line at given offset and
        start watching it for future changes.

        Args:
          line:
            Offset or name of the GPIO line to get information for.

        Returns:
          New gpiod.LineInfo object.
        """
        return self._get_line_info(line, watch=True)

    def unwatch_line_info(self, line: Union[int, str]) -> None:
        """
        Stop watching a line for status changes.

        Args:
          line:
            Offset or name of the line to stop watching.
        """
        self._check_closed()
        return self._chip.unwatch_line_info(self.line_offset_from_id(line))

    def wait_info_event(
        self, timeout: Optional[Union[timedelta, float]] = None
    ) -> bool:
        """
        Wait for line status change events on any of the watched lines on the
        chip.

        Args:
          timeout:
            Wait time limit represented as either a datetime.timedelta object
            or the number of seconds stored in a float. If set to 0, the
            method returns immediately, if set to None it blocks indefinitely.

        Returns:
          True if an info event is ready to be read from the chip, False if the
          wait timed out without any events.
        """
        self._check_closed()

        return poll_fd(self.fd, timeout)

    def read_info_event(self) -> InfoEvent:
        """
        Read a single line status change event from the chip.

        Returns:
          New gpiod.InfoEvent object.

        Note:
          This function may block if there are no available events in the queue.
        """
        self._check_closed()
        return self._chip.read_info_event()

    def request_lines(
        self,
        config: dict[tuple[Union[int, str]], Optional[LineSettings]],
        consumer: Optional[str] = None,
        event_buffer_size: Optional[int] = None,
        output_values: Optional[dict[Union[int, str], Value]] = None,
    ) -> LineRequest:
        """
        Request a set of lines for exclusive usage.

        Args:
          config:
            Dictionary mapping offsets or names (or tuples thereof) to
            LineSettings. If None is passed as the value of the mapping,
            default settings are used.
          consumer:
            Consumer string to use for this request.
          event_buffer_size:
            Size of the kernel edge event buffer to configure for this request.
          output_values:
            Dictionary mapping offsets or names to line.Value. This can be used
            to set the desired output values globally while reusing LineSettings
            for more lines.

        Returns:
          New LineRequest object.
        """
        self._check_closed()

        line_cfg = _ext.LineConfig()

        # Sanitize lines - don't allow offset repetitions or offset-name conflicts.
        for offset, count in Counter(
            [
                self.line_offset_from_id(line)
                for line in (
                    lambda t: [
                        j for i in (t) for j in (i if isinstance(i, tuple) else (i,))
                    ]
                )(tuple(config.keys()))
            ]
        ).items():
            if count != 1:
                raise ValueError(
                    "line must be configured exactly once - offset {} repeats".format(
                        offset
                    )
                )

        # If we have global output values - map line names to offsets
        if output_values:
            mapped_output_values = {
                self.line_offset_from_id(line): value
                for line, value in output_values.items()
            }
        else:
            mapped_output_values = None

        name_map = dict()
        offset_map = dict()
        global_output_values = list()

        for lines, settings in config.items():
            offsets = list()

            if isinstance(lines, int) or isinstance(lines, str):
                lines = (lines,)

            for line in lines:
                offset = self.line_offset_from_id(line)
                offsets.append(offset)

                # If there's a global output value for this offset, store it in the
                # list for later.
                if mapped_output_values:
                    global_output_values.append(
                        mapped_output_values[offset]
                        if offset in mapped_output_values
                        else Value.INACTIVE
                    )

                if isinstance(line, str):
                    name_map[line] = offset
                    offset_map[offset] = line

            line_cfg.add_line_settings(
                offsets, _line_settings_to_ext(settings or LineSettings())
            )

        if len(global_output_values):
            line_cfg.set_output_values(global_output_values)

        req_internal = self._chip.request_lines(line_cfg, consumer, event_buffer_size)
        request = LineRequest(req_internal)

        request._chip_name = req_internal.chip_name
        request._offsets = req_internal.offsets
        request._name_map = name_map
        request._offset_map = offset_map

        request._lines = [
            offset_map[off] if off in offset_map else off for off in request.offsets
        ]

        return request

    def __repr__(self) -> str:
        """
        Return a string that can be used to re-create this chip object.
        """
        if not self._chip:
            return "<Chip CLOSED>"

        return 'gpiod.Chip("{}")'.format(self.path)

    def __str__(self) -> str:
        """
        Return a user-friendly, human-readable description of this chip.
        """
        if not self._chip:
            return "<Chip CLOSED>"

        return '<Chip path="{}" fd={} info={}>'.format(
            self.path, self.fd, self.get_info()
        )

    @property
    def path(self) -> str:
        """
        Filesystem path used to open this chip.
        """
        self._check_closed()
        return self._chip.path

    @property
    def fd(self) -> int:
        """
        File descriptor associated with this chip.
        """
        self._check_closed()
        return self._chip.fd
