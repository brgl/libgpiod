// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation of gpioget tool.

use std::env;

use libgpiod::{
    chip::Chip,
    line::{self, Direction, Offset},
    request, Error, Result,
};

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        println!("Usage: {} <chip> <line_offset0> ...", args[0]);
        return Err(Error::InvalidArguments);
    }

    let mut lsettings = line::Settings::new()?;
    let lconfig = line::Config::new()?;
    let mut offsets = Vec::<Offset>::new();

    for arg in &args[2..] {
        let offset = arg.parse::<Offset>().map_err(|_| Error::InvalidArguments)?;
        offsets.push(offset);
    }

    lsettings.set_direction(Direction::Input)?;
    lconfig.add_line_settings(&offsets, lsettings)?;

    let path = format!("/dev/gpiochip{}", args[1]);
    let chip = Chip::open(&path)?;

    let rconfig = request::Config::new()?;
    rconfig.set_consumer(&args[0])?;

    let request = chip.request_lines(Some(&rconfig), &lconfig)?;
    let map = request.values()?;

    println!("{:?}", map);
    Ok(())
}
