// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Example of a bi-directional line requested as input and then switched to output.

use libgpiod::{
    chip::Chip,
    line::{Config as LineConfig, Direction, Settings, Value},
    request::Config as ReqConfig,
    Result,
};

fn main() -> Result<()> {
    // Example configuration - customize to suit your situation
    let chip_path = "/dev/gpiochip0";
    let line_offset = 5;

    let mut lsettings = Settings::new()?;
    lsettings.set_direction(Direction::Input)?;

    let mut lconfig = LineConfig::new()?;
    lconfig.add_line_settings(&[line_offset], lsettings)?;

    let mut rconfig = ReqConfig::new()?;
    rconfig.set_consumer("reconfigure-input-to-output")?;

    let chip = Chip::open(&chip_path)?;
    // request the line initially as an input
    let mut request = chip.request_lines(Some(&rconfig), &lconfig)?;

    // read the current line value
    let value = request.value(line_offset)?;
    println!("{line_offset}={value:?} (input)");

    // switch the line to an output and drive it low
    let mut lsettings = Settings::new()?;
    lsettings.set_direction(Direction::Output)?;
    lsettings.set_output_value(Value::InActive)?;
    lconfig.add_line_settings(&[line_offset], lsettings)?;
    request.reconfigure_lines(&lconfig)?;

    // report the current driven value
    let value = request.value(line_offset)?;
    println!("{line_offset}={value:?} (output)");

    Ok(())
}
