# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2024 Kent Gibson <warthog618@gmail.com>

from unittest import TestCase

from gpiod.line import Value


class LineValue(TestCase):
    def test_cast_bool(self) -> None:
        self.assertTrue(bool(Value.ACTIVE))
        self.assertFalse(bool(Value.INACTIVE))
