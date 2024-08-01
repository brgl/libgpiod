# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import errno
import gpiod

from . import gpiosim
from gpiod.line import Clock, Direction, Drive, Edge, Value
from unittest import TestCase

Pull = gpiosim.Chip.Pull
SimVal = gpiosim.Chip.Value


class ChipLineRequestsBehaveCorrectlyWithInvalidArguments(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self):
        self.chip.close()
        del self.chip
        del self.sim

    def test_passing_invalid_types_as_configs(self):
        with self.assertRaises(AttributeError):
            self.chip.request_lines("foobar")

        with self.assertRaises(AttributeError):
            self.chip.request_lines(None, "foobar")

    def test_offset_out_of_range(self):
        with self.assertRaises(ValueError):
            self.chip.request_lines(config={(1, 0, 4, 8): None})

    def test_line_name_not_found(self):
        with self.assertRaises(FileNotFoundError):
            self.chip.request_lines(config={"foo": None})

    def test_request_no_arguments(self):
        with self.assertRaises(TypeError):
            self.chip.request_lines()


class ModuleLineRequestsBehaveCorrectlyWithInvalidArguments(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)

    def tearDown(self):
        del self.sim

    def test_passing_invalid_types_as_configs(self):
        with self.assertRaises(AttributeError):
            gpiod.request_lines(self.sim.dev_path, "foobar")

        with self.assertRaises(AttributeError):
            gpiod.request_lines(self.sim.dev_path, None, "foobar")

    def test_offset_out_of_range(self):
        with self.assertRaises(ValueError):
            gpiod.request_lines(self.sim.dev_path, config={(1, 0, 4, 8): None})

    def test_line_name_not_found(self):
        with self.assertRaises(FileNotFoundError):
            gpiod.request_lines(self.sim.dev_path, config={"foo": None})

    def test_request_no_arguments(self):
        with self.assertRaises(TypeError):
            gpiod.request_lines()


class ChipLineRequestWorks(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8, line_names={5: "foo", 7: "bar"})
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self):
        self.chip.close()
        del self.chip
        del self.sim

    def test_request_with_positional_arguments(self):
        with self.chip.request_lines({(0, 5, 3, 1): None}, "foobar", 32) as req:
            self.assertEqual(req.offsets, [0, 5, 3, 1])
            self.assertEqual(self.chip.get_line_info(0).consumer, "foobar")

    def test_request_with_keyword_arguments(self):
        with self.chip.request_lines(
            config={(0, 5, 6): None},
            consumer="foobar",
            event_buffer_size=16,
        ) as req:
            self.assertEqual(req.offsets, [0, 5, 6])
            self.assertEqual(self.chip.get_line_info(0).consumer, "foobar")

    def test_request_single_offset_as_int(self):
        with self.chip.request_lines(config={4: None}) as req:
            self.assertEqual(req.offsets, [4])

    def test_request_single_offset_as_tuple(self):
        with self.chip.request_lines(config={(4): None}) as req:
            self.assertEqual(req.offsets, [4])

    def test_request_by_name(self):
        with self.chip.request_lines(config={(1, 2, "foo", "bar"): None}) as req:
            self.assertEqual(req.offsets, [1, 2, 5, 7])

    def test_request_single_line_by_name(self):
        with self.chip.request_lines(config={"foo": None}) as req:
            self.assertEqual(req.offsets, [5])


class ModuleLineRequestWorks(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8, line_names={5: "foo", 7: "bar"})

    def tearDown(self):
        del self.sim

    def test_request_with_positional_arguments(self):
        with gpiod.request_lines(
            self.sim.dev_path, {(0, 5, 3, 1): None}, "foobar", 32
        ) as req:
            self.assertEqual(req.offsets, [0, 5, 3, 1])
            with gpiod.Chip(self.sim.dev_path) as chip:
                self.assertEqual(chip.get_line_info(5).consumer, "foobar")

    def test_request_with_keyword_arguments(self):
        with gpiod.request_lines(
            path=self.sim.dev_path,
            config={(0, 5, 6): None},
            consumer="foobar",
            event_buffer_size=16,
        ) as req:
            self.assertEqual(req.offsets, [0, 5, 6])
            with gpiod.Chip(self.sim.dev_path) as chip:
                self.assertEqual(chip.get_line_info(5).consumer, "foobar")

    def test_request_single_offset_as_int(self):
        with gpiod.request_lines(path=self.sim.dev_path, config={4: None}) as req:
            self.assertEqual(req.offsets, [4])

    def test_request_single_offset_as_tuple(self):
        with gpiod.request_lines(path=self.sim.dev_path, config={(4): None}) as req:
            self.assertEqual(req.offsets, [4])

    def test_request_by_name(self):
        with gpiod.request_lines(
            self.sim.dev_path, {(1, 2, "foo", "bar"): None}
        ) as req:
            self.assertEqual(req.offsets, [1, 2, 5, 7])


class LineRequestGettingValues(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)
        self.req = gpiod.request_lines(
            self.sim.dev_path,
            {(0, 1, 2, 3): gpiod.LineSettings(direction=Direction.INPUT)},
        )

    def tearDown(self):
        self.req.release()
        del self.req
        del self.sim

    def test_get_single_value(self):
        self.sim.set_pull(1, Pull.UP)

        self.assertEqual(self.req.get_values([1]), [Value.ACTIVE])

    def test_get_single_value_helper(self):
        self.sim.set_pull(1, Pull.UP)

        self.assertEqual(self.req.get_value(1), Value.ACTIVE)

    def test_get_values_for_subset_of_lines(self):
        self.sim.set_pull(0, Pull.UP)
        self.sim.set_pull(1, Pull.DOWN)
        self.sim.set_pull(3, Pull.UP)

        self.assertEqual(
            self.req.get_values([0, 1, 3]), [Value.ACTIVE, Value.INACTIVE, Value.ACTIVE]
        )

    def test_get_all_values(self):
        self.sim.set_pull(0, Pull.DOWN)
        self.sim.set_pull(1, Pull.UP)
        self.sim.set_pull(2, Pull.UP)
        self.sim.set_pull(3, Pull.UP)

        self.assertEqual(
            self.req.get_values(),
            [Value.INACTIVE, Value.ACTIVE, Value.ACTIVE, Value.ACTIVE],
        )

    def test_get_values_invalid_offset(self):
        with self.assertRaises(ValueError):
            self.req.get_values([9])

    def test_get_values_invalid_argument_type(self):
        with self.assertRaises(TypeError):
            self.req.get_values(True)


class LineRequestGettingValuesByName(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=4, line_names={2: "foo", 3: "bar", 1: "baz"})
        self.req = gpiod.request_lines(
            self.sim.dev_path,
            {(0, "baz", "bar", "foo"): gpiod.LineSettings(direction=Direction.INPUT)},
        )

    def tearDown(self):
        self.req.release()
        del self.req
        del self.sim

    def test_get_values_by_name(self):
        self.sim.set_pull(1, Pull.UP)
        self.sim.set_pull(2, Pull.DOWN)
        self.sim.set_pull(3, Pull.UP)

        self.assertEqual(
            self.req.get_values(["foo", "bar", 1]),
            [Value.INACTIVE, Value.ACTIVE, Value.ACTIVE],
        )

    def test_get_values_by_bad_name(self):
        with self.assertRaises(ValueError):
            self.req.get_values(["xyz"])


class LineRequestSettingValues(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)
        self.req = gpiod.request_lines(
            self.sim.dev_path,
            {(0, 1, 2, 3): gpiod.LineSettings(direction=Direction.OUTPUT)},
        )

    def tearDown(self):
        self.req.release()
        del self.req
        del self.sim

    def test_set_single_value(self):
        self.req.set_values({1: Value.ACTIVE})
        self.assertEqual(self.sim.get_value(1), SimVal.ACTIVE)

    def test_set_single_value_helper(self):
        self.req.set_value(1, Value.ACTIVE)
        self.assertEqual(self.sim.get_value(1), SimVal.ACTIVE)

    def test_set_values_for_subset_of_lines(self):
        self.req.set_values({0: Value.ACTIVE, 1: Value.INACTIVE, 3: Value.ACTIVE})

        self.assertEqual(self.sim.get_value(0), SimVal.ACTIVE)
        self.assertEqual(self.sim.get_value(1), SimVal.INACTIVE)
        self.assertEqual(self.sim.get_value(3), SimVal.ACTIVE)

    def test_set_values_invalid_offset(self):
        with self.assertRaises(ValueError):
            self.req.set_values({9: Value.ACTIVE})


class LineRequestSettingValuesByName(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=4, line_names={2: "foo", 3: "bar", 1: "baz"})
        self.req = gpiod.request_lines(
            self.sim.dev_path,
            {(0, "baz", "bar", "foo"): gpiod.LineSettings(direction=Direction.OUTPUT)},
        )

    def tearDown(self):
        self.req.release()
        del self.req
        del self.sim

    def test_set_values_by_name(self):
        self.req.set_values(
            {"foo": Value.INACTIVE, "bar": Value.ACTIVE, 1: Value.ACTIVE}
        )

        self.assertEqual(self.sim.get_value(2), SimVal.INACTIVE)
        self.assertEqual(self.sim.get_value(1), SimVal.ACTIVE)
        self.assertEqual(self.sim.get_value(3), SimVal.ACTIVE)

    def test_set_values_by_bad_name(self):
        with self.assertRaises(ValueError):
            self.req.set_values({"xyz": Value.ACTIVE})


class LineRequestComplexConfig(TestCase):
    def test_complex_config(self):
        sim = gpiosim.Chip(num_lines=8)

        with gpiod.Chip(sim.dev_path) as chip:
            with chip.request_lines(
                config={
                    (0, 2, 4): gpiod.LineSettings(
                        direction=Direction.OUTPUT, output_value=Value.ACTIVE
                    ),
                    (1, 3, 5): gpiod.LineSettings(
                        direction=Direction.INPUT,
                        edge_detection=Edge.BOTH,
                        event_clock=Clock.REALTIME,
                    ),
                },
            ) as req:
                self.assertEqual(chip.get_line_info(2).direction, Direction.OUTPUT)
                self.assertEqual(chip.get_line_info(3).edge_detection, Edge.BOTH)
                self.assertEqual(chip.get_line_info(5).event_clock, Clock.REALTIME)


class LineRequestMixedConfigByName(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(
            num_lines=4, line_names={2: "foo", 3: "bar", 1: "baz", 0: "xyz"}
        )
        self.req = gpiod.request_lines(
            self.sim.dev_path,
            {
                ("baz", "bar"): gpiod.LineSettings(direction=Direction.OUTPUT),
                ("foo", "xyz"): gpiod.LineSettings(direction=Direction.INPUT),
            },
        )

    def tearDown(self):
        self.req.release()
        del self.req
        del self.sim

    def test_set_values_by_name(self):
        self.req.set_values({"bar": Value.ACTIVE, "baz": Value.INACTIVE})

        self.assertEqual(self.sim.get_value(1), SimVal.INACTIVE)
        self.assertEqual(self.sim.get_value(3), SimVal.ACTIVE)

    def test_get_values_by_name(self):
        self.sim.set_pull(0, Pull.UP)
        self.sim.set_pull(2, Pull.DOWN)

        self.assertEqual(
            self.req.get_values(["foo", "xyz", "baz"]),
            [Value.INACTIVE, Value.ACTIVE, Value.INACTIVE],
        )


class RepeatingLinesInRequestConfig(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=4, line_names={0: "foo", 2: "bar"})
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self):
        self.chip.close()
        del self.chip
        del self.sim

    def test_offsets_repeating_within_the_same_tuple(self):
        with self.assertRaises(ValueError):
            self.chip.request_lines({(0, 1, 2, 1): None})

    def test_offsets_repeating_in_different_tuples(self):
        with self.assertRaises(ValueError):
            self.chip.request_lines({(0, 1, 2): None, (3, 4, 0): None})

    def test_offset_and_name_conflict_in_the_same_tuple(self):
        with self.assertRaises(ValueError):
            self.chip.request_lines({(2, "bar"): None})

    def test_offset_and_name_conflict_in_different_tuples(self):
        with self.assertRaises(ValueError):
            self.chip.request_lines({(0, 1, 2): None, (4, 5, "bar"): None})


class LineRequestPropertiesWork(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=16, line_names={0: "foo", 2: "bar", 5: "baz"})

    def tearDown(self):
        del self.sim

    def test_property_fd(self):
        with gpiod.request_lines(
            self.sim.dev_path,
            config={
                0: gpiod.LineSettings(
                    direction=Direction.INPUT, edge_detection=Edge.BOTH
                )
            },
        ) as req:
            self.assertGreaterEqual(req.fd, 0)

    def test_property_num_lines(self):
        with gpiod.request_lines(
            self.sim.dev_path, config={(0, 2, 3, 5, 6, 8, 12): None}
        ) as req:
            self.assertEqual(req.num_lines, 7)

    def test_property_offsets(self):
        with gpiod.request_lines(
            self.sim.dev_path, config={(1, 6, 12, 4): None}
        ) as req:
            self.assertEqual(req.offsets, [1, 6, 12, 4])

    def test_property_lines(self):
        with gpiod.request_lines(
            self.sim.dev_path, config={("foo", 1, "bar", 4, "baz"): None}
        ) as req:
            self.assertEqual(req.lines, ["foo", 1, "bar", 4, "baz"])


class LineRequestConsumerString(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=4)
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self):
        self.chip.close()
        del self.chip
        del self.sim

    def test_custom_consumer(self):
        with self.chip.request_lines(
            consumer="foobar", config={(2, 3): None}
        ) as request:
            info = self.chip.get_line_info(2)
            self.assertEqual(info.consumer, "foobar")

    def test_empty_consumer(self):
        with self.chip.request_lines(consumer="", config={(2, 3): None}) as request:
            info = self.chip.get_line_info(2)
            self.assertEqual(info.consumer, "?")

    def test_default_consumer(self):
        with self.chip.request_lines(config={(2, 3): None}) as request:
            info = self.chip.get_line_info(2)
            self.assertEqual(info.consumer, "?")


class LineRequestSetOutputValues(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(
            num_lines=4, line_names={0: "foo", 1: "bar", 2: "baz", 3: "xyz"}
        )

    def tearDown(self):
        del self.sim

    def test_request_with_globally_set_output_values(self):
        with gpiod.request_lines(
            self.sim.dev_path,
            config={(0, 1, 2, 3): gpiod.LineSettings(direction=Direction.OUTPUT)},
            output_values={
                0: Value.ACTIVE,
                1: Value.INACTIVE,
                2: Value.ACTIVE,
                3: Value.INACTIVE,
            },
        ) as request:
            self.assertEqual(self.sim.get_value(0), SimVal.ACTIVE)
            self.assertEqual(self.sim.get_value(1), SimVal.INACTIVE)
            self.assertEqual(self.sim.get_value(2), SimVal.ACTIVE)
            self.assertEqual(self.sim.get_value(3), SimVal.INACTIVE)

    def test_request_with_globally_set_output_values_with_mapping(self):
        with gpiod.request_lines(
            self.sim.dev_path,
            config={(0, 1, 2, 3): gpiod.LineSettings(direction=Direction.OUTPUT)},
            output_values={"baz": Value.ACTIVE, "foo": Value.ACTIVE},
        ) as request:
            self.assertEqual(self.sim.get_value(0), SimVal.ACTIVE)
            self.assertEqual(self.sim.get_value(1), SimVal.INACTIVE)
            self.assertEqual(self.sim.get_value(2), SimVal.ACTIVE)
            self.assertEqual(self.sim.get_value(3), SimVal.INACTIVE)

    def test_request_with_globally_set_output_values_bad_mapping(self):
        with self.assertRaises(FileNotFoundError):
            with gpiod.request_lines(
                self.sim.dev_path,
                config={(0, 1, 2, 3): gpiod.LineSettings(direction=Direction.OUTPUT)},
                output_values={"foobar": Value.ACTIVE},
            ) as request:
                pass

    def test_request_with_globally_set_output_values_bad_offset(self):
        with self.assertRaises(ValueError):
            with gpiod.request_lines(
                self.sim.dev_path,
                config={(0, 1, 2, 3): gpiod.LineSettings(direction=Direction.OUTPUT)},
                output_values={5: Value.ACTIVE},
            ) as request:
                pass


class ReconfigureRequestedLines(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8, line_names={3: "foo", 4: "bar", 6: "baz"})
        self.chip = gpiod.Chip(self.sim.dev_path)
        self.req = self.chip.request_lines(
            {
                (0, 2, "foo", "baz"): gpiod.LineSettings(
                    direction=Direction.OUTPUT, active_low=True, drive=Drive.OPEN_DRAIN
                )
            }
        )

    def tearDown(self):
        self.chip.close()
        del self.chip
        self.req.release()
        del self.req
        del self.sim

    def test_reconfigure_by_offsets(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.req.reconfigure_lines(
            {(0, 2, 3, 6): gpiod.LineSettings(direction=Direction.INPUT)}
        )
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.INPUT)

    def test_reconfigure_by_names(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.req.reconfigure_lines(
            {(0, 2, "foo", "baz"): gpiod.LineSettings(direction=Direction.INPUT)}
        )
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.INPUT)

    def test_reconfigure_by_misordered_offsets(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.req.reconfigure_lines(
            {(6, 0, 3, 2): gpiod.LineSettings(direction=Direction.INPUT)}
        )
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.INPUT)

    def test_reconfigure_by_misordered_names(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.req.reconfigure_lines(
            {(0, "baz", 2, "foo"): gpiod.LineSettings(direction=Direction.INPUT)}
        )
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.INPUT)

    def test_reconfigure_with_default(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.assertTrue(info.active_low)
        self.assertEqual(info.drive, Drive.OPEN_DRAIN)
        self.req.reconfigure_lines(
            {
                0: gpiod.LineSettings(direction=Direction.INPUT),
                2: None,
                ("baz", "foo"): gpiod.LineSettings(direction=Direction.INPUT),
            }
        )
        info = self.chip.get_line_info(0)
        self.assertEqual(info.direction, Direction.INPUT)
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.assertTrue(info.active_low)
        self.assertEqual(info.drive, Drive.OPEN_DRAIN)

    def test_reconfigure_missing_offsets(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.assertTrue(info.active_low)
        self.assertEqual(info.drive, Drive.OPEN_DRAIN)
        self.req.reconfigure_lines(
            {(6, 0): gpiod.LineSettings(direction=Direction.INPUT)}
        )
        info = self.chip.get_line_info(0)
        self.assertEqual(info.direction, Direction.INPUT)
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.assertTrue(info.active_low)
        self.assertEqual(info.drive, Drive.OPEN_DRAIN)

    def test_reconfigure_extra_offsets(self):
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.OUTPUT)
        self.req.reconfigure_lines(
            {(0, 2, 3, 6, 5): gpiod.LineSettings(direction=Direction.INPUT)}
        )
        info = self.chip.get_line_info(2)
        self.assertEqual(info.direction, Direction.INPUT)


class ReleasedLineRequestCannotBeUsed(TestCase):
    def test_using_released_line_request(self):
        sim = gpiosim.Chip()

        with gpiod.Chip(sim.dev_path) as chip:
            req = chip.request_lines(config={0: None})
            req.release()

            with self.assertRaises(gpiod.RequestReleasedError):
                req.fd


class LineRequestSurvivesParentChip(TestCase):
    def test_line_request_survives_parent_chip(self):
        sim = gpiosim.Chip()

        chip = gpiod.Chip(sim.dev_path)
        try:
            req = chip.request_lines(
                config={0: gpiod.LineSettings(direction=Direction.INPUT)}
            )
        except:
            chip.close()
            raise

        chip.close()
        self.assertEqual(req.get_values([0]), [Value.INACTIVE])
        req.release()


class LineRequestStringRepresentation(TestCase):
    def setUp(self):
        self.sim = gpiosim.Chip(num_lines=8)

    def tearDown(self):
        del self.sim

    def test_str(self):
        with gpiod.Chip(self.sim.dev_path) as chip:
            with chip.request_lines(config={(2, 6, 4, 1): None}) as req:
                self.assertEqual(
                    str(req),
                    '<LineRequest chip="{}" num_lines=4 offsets=[2, 6, 4, 1] fd={}>'.format(
                        self.sim.name, req.fd
                    ),
                )

    def test_str_released(self):
        req = gpiod.request_lines(self.sim.dev_path, config={(2, 6, 4, 1): None})
        req.release()
        self.assertEqual(str(req), "<LineRequest RELEASED>")
