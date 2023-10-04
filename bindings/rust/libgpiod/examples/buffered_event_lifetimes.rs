// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// An example demonstrating that an edge event must be cloned to outlive
// subsequent writes to the containing event buffer.

use libgpiod::line;

fn main() -> libgpiod::Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 5;

    let mut lsettings = line::Settings::new()?;
    lsettings.set_edge_detection(Some(line::Edge::Both))?;

    let mut lconfig = line::Config::new()?;
    lconfig.add_line_settings(&[line_offset], lsettings)?;

    let mut rconfig = libgpiod::request::Config::new()?;
    rconfig.set_consumer("buffered-event-lifetimes")?;

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let request = chip.request_lines(Some(&rconfig), &lconfig)?;

    let mut buffer = libgpiod::request::Buffer::new(4)?;

    loop {
        // Blocks until at least one event is available
        let mut events = request.read_edge_events(&mut buffer)?;

        // This can't be used across the next read_edge_events().
        let event = events.next().unwrap()?;

        // This will out live `event` and the next read_edge_events().
        let cloned_event = libgpiod::request::Event::try_clone(event)?;

        let events = request.read_edge_events(&mut buffer)?;
        for event in events {
            let event = event?;
            println!(
                "line: {}  type: {:?}  event #{}",
                event.line_offset(),
                event.event_type(),
                event.line_seqno(),
            );
        }

        // `cloned_event` is still available to be used.
        println!(
            "line: {}  type: {:?}  event #{}",
            cloned_event.line_offset(),
            cloned_event.event_type(),
            cloned_event.line_seqno(),
        );
    }
}
