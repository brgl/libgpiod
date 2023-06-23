// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of reading the info for a chip.

use libgpiod::{self, Result};

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let info = chip.info()?;
    println!(
        "{} [{}] ({} lines)",
        info.name()?,
        info.label()?,
        info.num_lines(),
    );

    Ok(())
}
