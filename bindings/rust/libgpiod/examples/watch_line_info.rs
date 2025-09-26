// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of watching for info changes on particular lines.

use libgpiod::{Result, chip::Chip, line::InfoChangeKind};

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offsets = [5, 3, 7];

    let chip = Chip::open(&chip_path)?;
    for offset in line_offsets {
        let _info = chip.watch_line_info(offset)?;
    }

    loop {
        // Blocks until at least one event is available.
        let event = chip.read_info_event()?;
        println!(
            "line: {} {:<9} {:?}",
            event.line_info()?.offset(),
            match event.event_type()? {
                InfoChangeKind::LineRequested => "Requested",
                InfoChangeKind::LineReleased => "Released",
                InfoChangeKind::LineConfigChanged => "Reconfig",
            },
            event.timestamp()
        );
    }
}
