# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import gpiod
import os
import re
import unittest

from . import gpiosim
from .helpers import LinkGuard
from unittest import TestCase


class IsGPIOChip(TestCase):
    def test_is_gpiochip_bad(self):
        self.assertFalse(gpiod.is_gpiochip_device("/dev/null"))
        self.assertFalse(gpiod.is_gpiochip_device("/dev/nonexistent"))

    def test_is_gpiochip_invalid_argument(self):
        with self.assertRaises(TypeError):
            gpiod.is_gpiochip_device(4)

    def test_is_gpiochip_superfluous_argument(self):
        with self.assertRaises(TypeError):
            gpiod.is_gpiochip_device("/dev/null", 4)

    def test_is_gpiochip_missing_argument(self):
        with self.assertRaises(TypeError):
            gpiod.is_gpiochip_device()

    def test_is_gpiochip_good(self):
        sim = gpiosim.Chip()
        self.assertTrue(gpiod.is_gpiochip_device(sim.dev_path))

    def test_is_gpiochip_good_keyword_argument(self):
        sim = gpiosim.Chip()
        self.assertTrue(gpiod.is_gpiochip_device(path=sim.dev_path))

    def test_is_gpiochip_link_good(self):
        link = "/tmp/gpiod-py-test-link.{}".format(os.getpid())
        sim = gpiosim.Chip()

        with LinkGuard(sim.dev_path, link):
            self.assertTrue(gpiod.is_gpiochip_device(link))

    def test_is_gpiochip_link_bad(self):
        link = "/tmp/gpiod-py-test-link.{}".format(os.getpid())

        with LinkGuard("/dev/null", link):
            self.assertFalse(gpiod.is_gpiochip_device(link))


class VersionString(TestCase):
    def test_version_string(self):
        self.assertTrue(
            re.match(
                "^[0-9][1-9]?\\.[0-9][1-9]?([\\.0-9]?|\\-devel)$", gpiod.__version__
            )
        )
