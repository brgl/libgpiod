# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import errno
import os
from unittest import TestCase

import gpiod

from . import gpiosim
from .helpers import LinkGuard


class ChipConstructor(TestCase):
    def test_open_existing_chip(self) -> None:
        sim = gpiosim.Chip()

        with gpiod.Chip(sim.dev_path):
            pass

    def test_open_existing_chip_with_keyword(self) -> None:
        sim = gpiosim.Chip()

        with gpiod.Chip(path=sim.dev_path):
            pass

    def test_open_chip_by_link(self) -> None:
        link = "/tmp/gpiod-py-test-link.{}".format(os.getpid())
        sim = gpiosim.Chip()

        with LinkGuard(sim.dev_path, link):
            with gpiod.Chip(link):
                pass

    def test_open_nonexistent_chip(self) -> None:
        with self.assertRaises(OSError) as ex:
            gpiod.Chip("/dev/nonexistent")

        self.assertEqual(ex.exception.errno, errno.ENOENT)

    def test_open_not_a_character_device(self) -> None:
        with self.assertRaises(OSError) as ex:
            gpiod.Chip("/tmp")

        self.assertEqual(ex.exception.errno, errno.ENOTTY)

    def test_open_not_a_gpio_device(self) -> None:
        with self.assertRaises(OSError) as ex:
            gpiod.Chip("/dev/null")

        self.assertEqual(ex.exception.errno, errno.ENODEV)

    def test_missing_path(self) -> None:
        with self.assertRaises(TypeError):
            gpiod.Chip()

    def test_invalid_type_for_path(self) -> None:
        with self.assertRaises(TypeError):
            gpiod.Chip(4)


class ChipBooleanConversion(TestCase):
    def test_chip_bool(self) -> None:
        sim = gpiosim.Chip()
        chip = gpiod.Chip(sim.dev_path)
        self.assertTrue(chip)
        chip.close()
        self.assertFalse(chip)


class ChipProperties(TestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip()
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.sim = None  # type: ignore[assignment]

    def test_get_chip_path(self) -> None:
        self.assertEqual(self.sim.dev_path, self.chip.path)

    def test_get_fd(self) -> None:
        self.assertGreaterEqual(self.chip.fd, 0)

    def test_properties_are_immutable(self) -> None:
        with self.assertRaises(AttributeError):
            self.chip.path = "foobar"

        with self.assertRaises(AttributeError):
            self.chip.fd = 4


class ChipDevPathFromLink(TestCase):
    def test_dev_path_open_by_link(self) -> None:
        sim = gpiosim.Chip()
        link = "/tmp/gpiod-py-test-link.{}".format(os.getpid())

        with LinkGuard(sim.dev_path, link):
            with gpiod.Chip(link) as chip:
                self.assertEqual(chip.path, link)


class ChipMapLine(TestCase):
    def test_lookup_by_name_good(self) -> None:
        sim = gpiosim.Chip(
            num_lines=8, line_names={1: "foo", 2: "bar", 4: "baz", 5: "xyz"}
        )

        with gpiod.Chip(sim.dev_path) as chip:
            self.assertEqual(chip.line_offset_from_id("baz"), 4)

    def test_lookup_by_name_good_keyword_argument(self) -> None:
        sim = gpiosim.Chip(
            num_lines=8, line_names={1: "foo", 2: "bar", 4: "baz", 5: "xyz"}
        )

        with gpiod.Chip(sim.dev_path) as chip:
            self.assertEqual(chip.line_offset_from_id(id="baz"), 4)

    def test_lookup_bad_name(self) -> None:
        sim = gpiosim.Chip(
            num_lines=8, line_names={1: "foo", 2: "bar", 4: "baz", 5: "xyz"}
        )

        with gpiod.Chip(sim.dev_path) as chip:
            with self.assertRaises(FileNotFoundError):
                chip.line_offset_from_id("nonexistent")

    def test_lookup_bad_offset(self) -> None:
        sim = gpiosim.Chip()

        with gpiod.Chip(sim.dev_path) as chip:
            with self.assertRaises(ValueError):
                chip.line_offset_from_id(4)

    def test_lookup_bad_offset_as_string(self) -> None:
        sim = gpiosim.Chip()

        with gpiod.Chip(sim.dev_path) as chip:
            with self.assertRaises(ValueError):
                chip.line_offset_from_id("4")

    def test_duplicate_names(self) -> None:
        sim = gpiosim.Chip(
            num_lines=8, line_names={1: "foo", 2: "bar", 4: "baz", 5: "bar"}
        )

        with gpiod.Chip(sim.dev_path) as chip:
            self.assertEqual(chip.line_offset_from_id("bar"), 2)

    def test_integer_offsets(self) -> None:
        sim = gpiosim.Chip(num_lines=8, line_names={1: "foo", 2: "bar", 6: "baz"})

        with gpiod.Chip(sim.dev_path) as chip:
            self.assertEqual(chip.line_offset_from_id(4), 4)
            self.assertEqual(chip.line_offset_from_id(1), 1)

    def test_offsets_as_string(self) -> None:
        sim = gpiosim.Chip(num_lines=8, line_names={1: "foo", 2: "bar", 7: "6"})

        with gpiod.Chip(sim.dev_path) as chip:
            self.assertEqual(chip.line_offset_from_id("2"), 2)
            self.assertEqual(chip.line_offset_from_id("6"), 7)


class ClosedChipCannotBeUsed(TestCase):
    def test_close_chip_and_try_to_use_it(self) -> None:
        sim = gpiosim.Chip(label="foobar")

        chip = gpiod.Chip(sim.dev_path)
        chip.close()

        with self.assertRaises(gpiod.ChipClosedError):
            chip.path

    def test_close_chip_and_try_controlled_execution(self) -> None:
        sim = gpiosim.Chip()

        chip = gpiod.Chip(sim.dev_path)
        chip.close()

        with self.assertRaises(gpiod.ChipClosedError):
            with chip:
                chip.fd

    def test_close_chip_twice(self) -> None:
        sim = gpiosim.Chip(label="foobar")
        chip = gpiod.Chip(sim.dev_path)
        chip.close()

        with self.assertRaises(gpiod.ChipClosedError):
            chip.close()


class StringRepresentation(TestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(num_lines=4, label="foobar")
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.sim = None  # type: ignore[assignment]

    def test_repr(self) -> None:
        self.assertEqual(repr(self.chip), 'gpiod.Chip("{}")'.format(self.sim.dev_path))

        cmp = eval(repr(self.chip))
        self.assertEqual(self.chip.path, cmp.path)

    def test_str(self) -> None:
        info = self.chip.get_info()
        self.assertEqual(
            str(self.chip),
            '<Chip path="{}" fd={} info=<ChipInfo name="{}" label="foobar" num_lines=4>>'.format(
                self.sim.dev_path, self.chip.fd, info.name
            ),
        )


class StringRepresentationClosed(TestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(num_lines=4, label="foobar")
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.sim = None  # type: ignore[assignment]

    def test_repr_closed(self) -> None:
        self.chip.close()
        self.assertEqual(repr(self.chip), "<Chip CLOSED>")

    def test_str_closed(self) -> None:
        self.chip.close()
        self.assertEqual(str(self.chip), "<Chip CLOSED>")
