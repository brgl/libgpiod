# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

"""
Python bindings for libgpiod.

This module wraps the native C API of libgpiod in a set of python classes.
"""

from . import _ext
from . import line
from .chip import Chip
from .chip_info import ChipInfo
from .edge_event import EdgeEvent
from .exception import ChipClosedError, RequestReleasedError
from .info_event import InfoEvent
from .line_info import LineInfo
from .line_request import LineRequest
from .line_settings import LineSettings
from .version import __version__

api_version = _ext.api_version


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


def request_lines(path: str, *args, **kwargs) -> LineRequest:
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
        return chip.request_lines(*args, **kwargs)
