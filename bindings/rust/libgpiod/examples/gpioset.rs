// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation of the gpioset tool.

use std::env;
use std::io::{stdin, Read};

use libgpiod::{
    chip::Chip,
    line::{self, Direction, Offset, SettingVal, Value},
    request, Error, Result,
};

fn usage(name: &str) {
    println!("Usage: {} <chip> <line_offset0>=<value0> ...", name);
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        usage(&args[0]);
        return Err(Error::InvalidArguments);
    }

    let mut lconfig = line::Config::new()?;

    for arg in &args[2..] {
        let pair: Vec<&str> = arg.split('=').collect();
        if pair.len() != 2 {
            usage(&args[0]);
            return Err(Error::InvalidArguments);
        }

        let offset = pair[0]
            .parse::<Offset>()
            .map_err(|_| Error::InvalidArguments)?;
        let value = pair[1]
            .parse::<i32>()
            .map_err(|_| Error::InvalidArguments)?;

        let mut lsettings = line::Settings::new()?;
        lsettings.set_prop(&[
            SettingVal::Direction(Direction::Output),
            SettingVal::OutputValue(Value::new(value)?),
        ])?;
        lconfig.add_line_settings(&[offset], lsettings)?;
    }

    let path = format!("/dev/gpiochip{}", args[1]);
    let chip = Chip::open(&path)?;

    let mut rconfig = request::Config::new()?;
    rconfig.set_consumer(&args[0])?;

    chip.request_lines(Some(&rconfig), &lconfig)?;

    // Wait for keypress, let user verify line status.
    stdin().read_exact(&mut [0u8]).unwrap();

    Ok(())
}
