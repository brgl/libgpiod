// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of toggling a single line.

use libgpiod::line;
use std::time::Duration;

fn toggle_value(value: line::Value) -> line::Value {
    match value {
        line::Value::Active => line::Value::InActive,
        line::Value::InActive => line::Value::Active,
    }
}

fn main() -> libgpiod::Result<()> {
    // example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 5;

    let mut value = line::Value::Active;

    let mut settings = line::Settings::new()?;
    settings
        .set_direction(line::Direction::Output)?
        .set_output_value(value)?;

    let mut lconfig = line::Config::new()?;
    lconfig.add_line_settings(&[line_offset], settings)?;

    let mut rconfig = libgpiod::request::Config::new()?;
    rconfig.set_consumer("toggle-line-value")?;

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let mut req = chip.request_lines(Some(&rconfig), &lconfig)?;

    loop {
        println!("{:?}", value);
        std::thread::sleep(Duration::from_secs(1));
        value = toggle_value(value);
        req.set_value(line_offset, value)?;
    }
}
