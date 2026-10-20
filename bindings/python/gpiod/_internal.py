# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from __future__ import annotations

from datetime import timedelta
from select import select
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Generator, Iterable

    from .line_settings import LineSettings

__all__ = ["config_iter", "poll_fd"]


def poll_fd(fd: int, timeout: timedelta | float | None = None) -> bool:
    sec: float | None
    if isinstance(timeout, timedelta):
        sec = timeout.total_seconds()
    else:
        sec = timeout

    readable, _, _ = select([fd], [], [], sec)
    return fd in readable


def config_iter(
    config: dict[Iterable[int | str] | int | str, LineSettings | None],
) -> Generator[tuple[int | str, LineSettings | None]]:
    for key, settings in config.items():
        if isinstance(key, int) or isinstance(key, str):
            yield key, settings
        else:
            for subkey in key:
                yield subkey, settings
