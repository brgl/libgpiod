// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of watching for edges on a single line.

use libgpiod::{
    chip::Chip,
    line::{Config as LineConfig, Edge, EdgeKind, Settings},
    request::{Buffer, Config as ReqConfig},
    Result,
};

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 5;

    let mut lsettings = Settings::new()?;
    lsettings.set_edge_detection(Some(Edge::Rising))?;

    let mut lconfig = LineConfig::new()?;
    lconfig.add_line_settings(&[line_offset], lsettings)?;

    let mut rconfig = ReqConfig::new()?;
    rconfig.set_consumer("watch-line-value")?;

    let chip = Chip::open(&chip_path)?;
    let request = chip.request_lines(Some(&rconfig), &lconfig)?;

    // A larger buffer is an optimisation for reading bursts of events from the
    // kernel, but that is not necessary in this case, so 1 is fine.
    let mut buffer = Buffer::new(1)?;
    loop {
        // blocks until at least one event is available
        let events = request.read_edge_events(&mut buffer)?;
        for event in events {
            let event = event?;
            println!(
                "line: {}  type: {:<7}  event #{}",
                event.line_offset(),
                match event.event_type()? {
                    EdgeKind::Rising => "Rising",
                    EdgeKind::Falling => "Falling",
                },
                event.line_seqno()
            );
        }
    }
}
