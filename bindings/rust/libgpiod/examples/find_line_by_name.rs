// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of finding a line with the given name.

use libgpiod::{gpiochip_devices, Result};

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let line_name = "GPIO19";

    // Names are not guaranteed unique, so this finds the first line with
    // the given name.
    for chip in gpiochip_devices(&"/dev")? {
        let offset = chip.line_offset_from_name(line_name);

        if offset.is_ok() {
            let info = chip.info()?;
            println!("{}: {} {}", line_name, info.name()?, offset?);
            return Ok(());
        }
    }

    println!("line '{line_name}' not found");
    Ok(())
}
