// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of watching for edges on a single line.

use libgpiod::line;
use std::time::Duration;

fn main() -> libgpiod::Result<()> {
    // example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 5;

    let mut lsettings = line::Settings::new()?;
    // assume a button connecting the pin to ground,
    // so pull it up and provide some debounce.
    lsettings
        .set_edge_detection(Some(line::Edge::Both))?
        .set_bias(Some(line::Bias::PullUp))?
        .set_debounce_period(Duration::from_millis(10));

    let mut lconfig = line::Config::new()?;
    lconfig.add_line_settings(&[line_offset], lsettings)?;

    let mut rconfig = libgpiod::request::Config::new()?;
    rconfig.set_consumer("watch-line-value")?;

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let request = chip.request_lines(Some(&rconfig), &lconfig)?;

    // a larger buffer is an optimisation for reading bursts of events from the
    // kernel, but that is not necessary in this case, so 1 is fine.
    let mut buffer = libgpiod::request::Buffer::new(1)?;
    loop {
        // blocks until at least one event is available
        let events = request.read_edge_events(&mut buffer)?;
        for event in events {
            let event = event?;
            println!(
                "line: {}, type: {}, event #{}",
                event.line_offset(),
                match event.event_type()? {
                    line::EdgeKind::Rising => "Rising ",
                    line::EdgeKind::Falling => "Falling",
                },
                event.line_seqno()
            );
        }
    }
}
