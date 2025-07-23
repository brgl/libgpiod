// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of watching for edges on multiple lines.

use libgpiod::{
    line::{self, EdgeKind},
    request, Result,
};

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offsets = [5, 3, 7];

    let mut lsettings = line::Settings::new()?;
    lsettings.set_edge_detection(Some(line::Edge::Both))?;

    let mut lconfig = line::Config::new()?;
    lconfig.add_line_settings(&line_offsets, lsettings)?;

    let mut rconfig = request::Config::new()?;
    rconfig.set_consumer("watch-multiple-line-values")?;

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let request = chip.request_lines(Some(&rconfig), &lconfig)?;

    let mut buffer = request::Buffer::new(4)?;
    loop {
        // Blocks until at least one event is available.
        let events = request.read_edge_events(&mut buffer)?;
        for event in events {
            let event = event?;
            println!(
                "offset: {}  type: {:<7}  event #{}  line event #{}",
                event.line_offset(),
                match event.event_type()? {
                    EdgeKind::Rising => "Rising",
                    EdgeKind::Falling => "Falling",
                },
                event.global_seqno(),
                event.line_seqno(),
            );
        }
    }
}
