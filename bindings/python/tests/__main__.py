#!/usr/bin/python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import unittest

from .tests_chip import *
from .tests_chip_info import *
from .tests_edge_event import *
from .tests_info_event import *
from .tests_line_info import *
from .tests_line_settings import *
from .tests_module import *
from .tests_line_request import *

unittest.main()
