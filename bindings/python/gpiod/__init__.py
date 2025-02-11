# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""
Python bindings for libgpiod.

This module wraps the native C API of libgpiod in a set of python classes.
"""

from collections.abc import Iterable
from typing import Optional, Union

from . import (
    _ext,
    chip,
    chip_info,
    edge_event,
    exception,
    info_event,
    line,
    line_info,
    line_request,
    line_settings,
    version,
)
from .chip import *
from .chip_info import *
from .edge_event import *
from .exception import *
from .info_event import *
from .line_info import *
from .line_request import *
from .line_settings import *
from .version import __version__

api_version = _ext.api_version

# public submodules
__all__ = [
    "chip",
    "chip_info",
    "edge_event",
    "exception",
    "info_event",
    "line",
    "line_info",
    "line_request",
    "line_settings",
    "version",
]

# re-export public submodule exports
# do not re-export line objects, this is not an oversight
__all__ += (
    chip.__all__
    + chip_info.__all__
    + edge_event.__all__
    + exception.__all__
    + info_event.__all__
    + line_info.__all__
    + line_request.__all__
    + line_settings.__all__
)

# module methods/attributes
__all__ += [
    "__version__",
    "api_version",
    "is_gpiochip_device",
    "request_lines",
]


def is_gpiochip_device(path: str) -> bool:
    """
    Check if the file pointed to by path is a GPIO chip character device.

    Args:
      path
        Path to the file that should be checked.

    Returns:
      Returns true if so, False otherwise.
    """
    return _ext.is_gpiochip_device(path)


def request_lines(
    path: str,
    config: dict[Union[Iterable[Union[int, str]], int, str], Optional[LineSettings]],
    consumer: Optional[str] = None,
    event_buffer_size: Optional[int] = None,
    output_values: Optional[dict[Union[int, str], line.Value]] = None,
) -> LineRequest:
    """
    Open a GPIO chip pointed to by 'path', request lines according to the
    configuration arguments, close the chip and return the request object.

    Args:
      path
        Path to the GPIO character device file.
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
      Returns a new LineRequest object.
    """
    with Chip(path) as chip:
        return chip.request_lines(
            config=config,
            consumer=consumer,
            event_buffer_size=event_buffer_size,
            output_values=output_values,
        )
