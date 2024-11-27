# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from __future__ import annotations

import os
from typing import TYPE_CHECKING, Optional

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
        type: Optional[type[BaseException]],
        val: Optional[BaseException],
        tb: Optional[TracebackType],
    ) -> None:
        os.unlink(self.dst)
