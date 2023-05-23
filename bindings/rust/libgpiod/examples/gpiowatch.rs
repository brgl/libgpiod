// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation of the gpiowatch tool.

use std::env;

use libgpiod::{chip::Chip, line::Offset, Error, Result};

fn usage(name: &str) {
    println!("Usage: {} <chip> <offset0> ...", name);
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        usage(&args[0]);
        return Err(Error::InvalidArguments);
    }

    let path = format!("/dev/gpiochip{}", args[1]);
    let offset = args[2]
        .parse::<Offset>()
        .map_err(|_| Error::InvalidArguments)?;

    let chip = Chip::open(&path)?;
    let _info = chip.watch_line_info(offset)?;

    match chip.wait_info_event(None) {
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

    let event = chip.read_info_event()?;
    println!(
        "line: {} type: {:?}, time: {:?}",
        offset,
        event.event_type()?,
        event.timestamp()
    );

    chip.unwatch(offset);
    Ok(())
}
