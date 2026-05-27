# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from __future__ import annotations

import os
import sys
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from types import TracebackType


class LinkGuard:
    def __init__(self, src: str, dst: str) -> None:
        self.src = src
        self.dst = dst

    def __enter__(self) -> None:
        os.symlink(self.src, self.dst)

    def __exit__(
        self,
        type: type[BaseException] | None,
        val: BaseException | None,
        tb: TracebackType | None,
    ) -> None:
        os.unlink(self.dst)


def is_free_threaded() -> bool:
    return hasattr(sys, "_is_gil_enabled") and not sys._is_gil_enabled()
