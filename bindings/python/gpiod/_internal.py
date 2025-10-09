# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from __future__ import annotations

from datetime import timedelta
from select import select
from typing import TYPE_CHECKING, Optional, Union

if TYPE_CHECKING:
    from collections.abc import Generator, Iterable

    from .line_settings import LineSettings

__all__ = ["poll_fd", "config_iter"]


def poll_fd(fd: int, timeout: Optional[Union[timedelta, float]] = None) -> bool:
    sec: Union[float, None]
    if isinstance(timeout, timedelta):
        sec = timeout.total_seconds()
    else:
        sec = timeout

    readable, _, _ = select([fd], [], [], sec)
    return True if fd in readable else False


def config_iter(
    config: dict[Union[Iterable[Union[int, str]], int, str], Optional[LineSettings]],
) -> Generator[tuple[Union[int, str], Optional[LineSettings]]]:
    for key, settings in config.items():
        if isinstance(key, int) or isinstance(key, str):
            yield key, settings
        else:
            for subkey in key:
                yield subkey, settings
