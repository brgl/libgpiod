# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>


from dataclasses import dataclass

__all__ = ["ChipInfo"]


@dataclass(frozen=True, repr=False)
class ChipInfo:
    """
    Snapshot of a chip's status.
    """

    name: str
    label: str
    num_lines: int

    def __str__(self) -> str:
        return f'<ChipInfo name="{self.name}" label="{self.label}" num_lines={self.num_lines}>'
