// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of reading the info for a line.

use libgpiod::line::Direction;

fn main() -> libgpiod::Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 3;

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let info = chip.line_info(line_offset)?;

    let name = info.name().unwrap_or("unnamed");
    let consumer = info.consumer().unwrap_or("unused");

    let dir = match info.direction()? {
        Direction::AsIs => "none",
        Direction::Input => "input",
        Direction::Output => "output",
    };

    let low = if info.is_active_low() {
        "active-low"
    } else {
        "active-high"
    };

    println!("line {line_offset:>3}: {name:>12} {consumer:>12} {dir:>8} {low:>10}");

    Ok(())
}
