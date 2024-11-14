# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from unittest import TestCase

import gpiod

from . import gpiosim


class ChipInfoProperties(TestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(label="foobar", num_lines=16)
        self.chip = gpiod.Chip(self.sim.dev_path)
        self.info = self.chip.get_info()

    def tearDown(self) -> None:
        self.info = None  # type: ignore[assignment]
        self.chip.close()
        self.chip = None  # type: ignore[assignment]
        self.sim = None  # type: ignore[assignment]

    def test_chip_info_name(self) -> None:
        self.assertEqual(self.info.name, self.sim.name)

    def test_chip_info_label(self) -> None:
        self.assertEqual(self.info.label, "foobar")

    def test_chip_info_num_lines(self) -> None:
        self.assertEqual(self.info.num_lines, 16)

    def test_chip_info_properties_are_immutable(self) -> None:
        with self.assertRaises(AttributeError):
            self.info.name = "foobar"  # type: ignore[misc]

        with self.assertRaises(AttributeError):
            self.info.num_lines = 4  # type: ignore[misc]

        with self.assertRaises(AttributeError):
            self.info.label = "foobar"  # type: ignore[misc]


class ChipInfoStringRepresentation(TestCase):
    def test_chip_info_str(self) -> None:
        sim = gpiosim.Chip(label="foobar", num_lines=16)

        with gpiod.Chip(sim.dev_path) as chip:
            info = chip.get_info()

            self.assertEqual(
                str(info),
                f'<ChipInfo name="{sim.name}" label="foobar" num_lines=16>',
            )
