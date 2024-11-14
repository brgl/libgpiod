# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import os
from unittest import TestCase

import gpiod

from . import gpiosim
from .helpers import LinkGuard


class IsGPIOChip(TestCase):
    def test_is_gpiochip_bad(self) -> None:
        self.assertFalse(gpiod.is_gpiochip_device("/dev/null"))
        self.assertFalse(gpiod.is_gpiochip_device("/dev/nonexistent"))

    def test_is_gpiochip_invalid_argument(self) -> None:
        with self.assertRaises(TypeError):
            gpiod.is_gpiochip_device(4)  # type: ignore[arg-type]

    def test_is_gpiochip_superfluous_argument(self) -> None:
        with self.assertRaises(TypeError):
            gpiod.is_gpiochip_device("/dev/null", 4)  # type: ignore[call-arg]

    def test_is_gpiochip_missing_argument(self) -> None:
        with self.assertRaises(TypeError):
            gpiod.is_gpiochip_device()  # type: ignore[call-arg]

    def test_is_gpiochip_good(self) -> None:
        sim = gpiosim.Chip()
        self.assertTrue(gpiod.is_gpiochip_device(sim.dev_path))

    def test_is_gpiochip_good_keyword_argument(self) -> None:
        sim = gpiosim.Chip()
        self.assertTrue(gpiod.is_gpiochip_device(path=sim.dev_path))

    def test_is_gpiochip_link_good(self) -> None:
        link = f"/tmp/gpiod-py-test-link.{os.getpid()}"
        sim = gpiosim.Chip()

        with LinkGuard(sim.dev_path, link):
            self.assertTrue(gpiod.is_gpiochip_device(link))

    def test_is_gpiochip_link_bad(self) -> None:
        link = f"/tmp/gpiod-py-test-link.{os.getpid()}"

        with LinkGuard("/dev/null", link):
            self.assertFalse(gpiod.is_gpiochip_device(link))


class VersionString(TestCase):
    VERSION_PATTERN = "^\\d+\\.\\d+(\\.\\d+|\\-devel|\\-rc\\d+)?$"

    def test_api_version_string(self) -> None:
        self.assertRegex(gpiod.api_version, VersionString.VERSION_PATTERN)

    def test_module_version(self) -> None:
        self.assertRegex(gpiod.__version__, VersionString.VERSION_PATTERN)
