// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of toggling a single line.

use core::time::Duration;
use libgpiod::{
    Result,
    chip::Chip,
    line::{Config as LineConfig, Direction, Settings, Value},
    request::Config as ReqConfig,
};
use std::thread::sleep;

fn toggle_value(value: Value) -> Value {
    match value {
        Value::Active => Value::InActive,
        Value::InActive => Value::Active,
    }
}

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 5;
    let mut value = Value::Active;

    let mut settings = Settings::new()?;
    settings
        .set_direction(Direction::Output)?
        .set_output_value(value)?;

    let mut lconfig = LineConfig::new()?;
    lconfig.add_line_settings(&[line_offset], settings)?;

    let mut rconfig = ReqConfig::new()?;
    rconfig.set_consumer("toggle-line-value")?;

    let chip = Chip::open(&chip_path)?;
    let mut req = chip.request_lines(Some(&rconfig), &lconfig)?;

    loop {
        println!("{line_offset}={value:?}");
        sleep(Duration::from_secs(1));
        value = toggle_value(value);
        req.set_value(line_offset, value)?;
    }
}
