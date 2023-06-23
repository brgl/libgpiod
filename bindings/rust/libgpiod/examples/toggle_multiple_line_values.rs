// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of toggling multiple lines.

use core::time::Duration;
use libgpiod::line::{self, Offset, Value};

fn toggle_value(value: Value) -> Value {
    match value {
        Value::Active => Value::InActive,
        Value::InActive => Value::Active,
    }
}

fn toggle_values(values: &mut [Value]) {
    for i in values.iter_mut() {
        *i = toggle_value(*i);
    }
}

fn print_values(offsets: &[Offset], values: &[Value]) {
    for i in 0..offsets.len() {
        print!("{}={:?} ", offsets[i], values[i]);
    }
    println!();
}

fn main() -> libgpiod::Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offsets = [5, 3, 7];
    let mut values = vec![Value::Active, Value::Active, Value::InActive];

    let mut lsettings = line::Settings::new()?;
    lsettings.set_direction(line::Direction::Output)?;

    let mut lconfig = line::Config::new()?;
    lconfig
        .add_line_settings(&line_offsets, lsettings)?
        .set_output_values(&values)?;

    let mut rconfig = libgpiod::request::Config::new()?;
    rconfig.set_consumer("toggle-multiple-line-values")?;

    let chip = libgpiod::chip::Chip::open(&chip_path)?;
    let mut req = chip.request_lines(Some(&rconfig), &lconfig)?;

    loop {
        print_values(&line_offsets, &values);
        std::thread::sleep(Duration::from_secs(1));
        toggle_values(&mut values);
        req.set_values(&values)?;
    }
}
