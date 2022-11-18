// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation to show handling of events, when the buffer
// is read into multiple times. Based on gpiomon example.

use std::env;

use libgpiod::{
    chip::Chip,
    line::{self, Edge, Offset},
    request, Error, Result,
};

fn usage(name: &str) {
    println!("Usage: {} <chip> <offset0> ...", name);
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        usage(&args[0]);
        return Err(Error::InvalidArguments);
    }

    let mut lsettings = line::Settings::new()?;
    let lconfig = line::Config::new()?;
    let mut offsets = Vec::<Offset>::new();

    for arg in &args[2..] {
        let offset = arg.parse::<Offset>().map_err(|_| Error::InvalidArguments)?;
        offsets.push(offset);
    }

    lsettings.set_edge_detection(Some(Edge::Both))?;
    lconfig.add_line_settings(&offsets, lsettings)?;

    let path = format!("/dev/gpiochip{}", args[1]);
    let chip = Chip::open(&path)?;

    let rconfig = request::Config::new()?;

    let mut buffer = request::Buffer::new(1)?;
    let request = chip.request_lines(&rconfig, &lconfig)?;

    loop {
        match request.wait_edge_event(None) {
            Err(x) => {
                println!("{:?}", x);
                return Err(Error::InvalidArguments);
            }

            Ok(false) => {
                // This shouldn't happen as the call is blocking.
                panic!();
            }
            Ok(true) => (),
        }

        let mut events = request.read_edge_events(&mut buffer)?;

        // This can't be used across the next read_edge_events().
        let event = events.next().unwrap()?;

        // This will out live `event` and the next read_edge_events().
        let cloned_event = request::Event::event_clone(event)?;

        let events = request.read_edge_events(&mut buffer)?;
        for event in events {
            let event = event?;
            println!(
                "line: {} type: {:?}, time: {:?}",
                event.line_offset(),
                event.event_type(),
                event.timestamp()
            );
        }

        // `cloned_event` is still available to be used.
        println!(
            "line: {} type: {:?}, time: {:?}",
            cloned_event.line_offset(),
            cloned_event.event_type(),
            cloned_event.timestamp()
        );
    }
}
