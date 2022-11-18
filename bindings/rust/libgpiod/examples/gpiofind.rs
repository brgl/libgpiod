// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation of gpiofind tool.

use std::env;
use std::path::Path;

use libgpiod::{self, Error, Result};

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        println!("Usage: {} <line-name>", args[0]);
        return Err(Error::InvalidArguments);
    }

    for chip in libgpiod::gpiochip_devices(&Path::new("/dev"))? {
        let offset = chip.line_offset_from_name(&args[1]);
        let info = chip.info()?;

        if offset.is_ok() {
            println!(
                "Line {} found: Chip: {}, offset: {}",
                args[1],
                info.name()?,
                offset?
            );
            return Ok(());
        }
    }

    println!("Failed to find line: {}", args[1]);
    Ok(())
}
