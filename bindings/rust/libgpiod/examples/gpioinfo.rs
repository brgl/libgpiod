// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>
//
// Simplified Rust implementation of gpioinfo tool.

use std::env;
use std::path::Path;

use libgpiod::{
    chip::Chip,
    line::{Direction, Offset},
    Error, Result,
};

fn line_info(chip: &Chip, offset: Offset) -> Result<()> {
    let info = chip.line_info(offset)?;
    let off = info.offset();

    let name = match info.name() {
        Ok(name) => name,
        _ => "unused",
    };

    let consumer = match info.consumer() {
        Ok(name) => name,
        _ => "unnamed",
    };

    let low = if info.is_active_low() {
        "active-low"
    } else {
        "active-high"
    };

    let dir = match info.direction()? {
        Direction::AsIs => "None",
        Direction::Input => "Input",
        Direction::Output => "Output",
    };

    println!(
        "\tline {:>3}\
              \t{:>10}\
              \t{:>10}\
              \t{:>6}\
              \t{:>14}",
        off, name, consumer, dir, low
    );

    Ok(())
}

fn chip_info(chip: &Chip) -> Result<()> {
    let info = chip.info()?;
    let ngpio = info.num_lines();

    println!("GPIO Chip name: {}", info.name()?);
    println!("\tlabel: {}", info.label()?);
    println!("\tpath: {}", chip.path()?);
    println!("\tngpio: {}\n", ngpio);

    println!("\tLine information:");

    for offset in 0..ngpio {
        line_info(chip, offset as Offset)?;
    }
    println!("\n");

    Ok(())
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() > 2 {
        println!("Usage: {}", args[0]);
        return Err(Error::InvalidArguments);
    }

    if args.len() == 1 {
        for chip in libgpiod::gpiochip_devices(&Path::new("/dev"))? {
            chip_info(&chip)?;
        }
    } else {
        let index = args[1]
            .parse::<u32>()
            .map_err(|_| Error::InvalidArguments)?;
        let path = format!("/dev/gpiochip{}", index);
        if libgpiod::is_gpiochip_device(&path) {
            let chip = Chip::open(&path)?;

            chip_info(&chip)?;
        }
    }

    Ok(())
}
