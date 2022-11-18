// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation of gpiodetect tool.

use std::env;
use std::path::Path;

use libgpiod::{self, Error, Result};

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 {
        println!("Usage: {}", args[0]);
        return Err(Error::InvalidArguments);
    }

    for chip in libgpiod::gpiochip_devices(&Path::new("/dev"))? {
        let info = chip.info()?;
        println!(
            "{} [{}] ({})",
            info.name()?,
            info.label()?,
            info.num_lines(),
        );
    }

    Ok(())
}
