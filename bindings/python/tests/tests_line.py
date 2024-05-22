# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 Kent Gibson <warthog618@gmail.com>

from gpiod.line import Value
from unittest import TestCase


class LineValue(TestCase):
    def test_cast_bool(self):
        self.assertTrue(bool(Value.ACTIVE))
        self.assertFalse(bool(Value.INACTIVE))
