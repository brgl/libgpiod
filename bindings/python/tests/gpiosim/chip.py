# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from . import _ext
from enum import Enum
from typing import Optional


class Chip:
    """
    Represents a simulated GPIO chip.
    """

    class Pull(Enum):
        DOWN = _ext.PULL_DOWN
        UP = _ext.PULL_UP

    class Value(Enum):
        INACTIVE = _ext.VALUE_INACTIVE
        ACTIVE = _ext.VALUE_ACTIVE

    class Direction(Enum):
        INPUT = _ext.DIRECTION_INPUT
        OUTPUT_HIGH = _ext.DIRECTION_OUTPUT_HIGH
        OUTPUT_LOW = _ext.DIRECTION_OUTPUT_LOW

    def __init__(
        self,
        label: Optional[str] = None,
        num_lines: Optional[int] = None,
        line_names: Optional[dict[int, str]] = None,
        hogs: Optional[dict[int, tuple[str, Direction]]] = None,
    ):
        self._chip = _ext.Chip()

        if label:
            self._chip.set_label(label)

        if num_lines:
            self._chip.set_num_lines(num_lines)

        if line_names:
            for off, name in line_names.items():
                self._chip.set_line_name(off, name)

        if hogs:
            for off, (name, direction) in hogs.items():
                self._chip.set_hog(off, name, direction.value)

        self._chip.enable()

    def get_value(self, offset: int) -> Value:
        val = self._chip.get_value(offset)
        return Chip.Value(val)

    def set_pull(self, offset: int, pull: Pull) -> None:
        self._chip.set_pull(offset, pull.value)

    @property
    def dev_path(self) -> str:
        return self._chip.dev_path

    @property
    def name(self) -> str:
        return self._chip.name
