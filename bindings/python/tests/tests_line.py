# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 Kent Gibson <warthog618@gmail.com>

from unittest import TestCase

from gpiod.line import Value


class LineValue(TestCase):
    def test_cast_bool(self):
        self.assertTrue(bool(Value.ACTIVE))
        self.assertFalse(bool(Value.INACTIVE))
