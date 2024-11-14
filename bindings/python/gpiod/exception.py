# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

__all__ = ["ChipClosedError", "RequestReleasedError"]


class ChipClosedError(Exception):
    """
    Error raised when an already closed chip is used.
    """

    def __init__(self) -> None:
        super().__init__("I/O operation on closed chip")


class RequestReleasedError(Exception):
    """
    Error raised when a released request is used.
    """

    def __init__(self) -> None:
        super().__init__("GPIO lines have been released")
