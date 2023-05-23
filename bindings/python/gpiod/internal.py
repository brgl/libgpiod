# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from datetime import timedelta
from select import select
from typing import Optional, Union


def poll_fd(fd: int, timeout: Optional[Union[timedelta, float]] = None) -> bool:
    if isinstance(timeout, timedelta):
        sec = timeout.total_seconds()
    else:
        sec = timeout

    readable, _, _ = select([fd], [], [], sec)
    return True if fd in readable else False
