#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

"""Example of a bi-directional line requested as input and then switched to output."""

import gpiod

from gpiod.line import Direction, Value


def reconfigure_input_to_output(chip_path, line_offset):
    # request the line initially as an input
    with gpiod.request_lines(
        chip_path,
        consumer="reconfigure-input-to-output",
        config={line_offset: gpiod.LineSettings(direction=Direction.INPUT)},
    ) as request:
        # read the current line value
        value = request.get_value(line_offset)
        print("{}={} (input)".format(line_offset, value))
        # switch the line to an output and drive it low
        request.reconfigure_lines(
            config={
                line_offset: gpiod.LineSettings(
                    direction=Direction.OUTPUT, output_value=Value.INACTIVE
                )
            }
        )
        # report the current driven value
        value = request.get_value(line_offset)
        print("{}={} (output)".format(line_offset, value))


if __name__ == "__main__":
    try:
        reconfigure_input_to_output("/dev/gpiochip0", 5)
    except OSError as ex:
        print(ex, "\nCustomise the example configuration to suit your situation")
