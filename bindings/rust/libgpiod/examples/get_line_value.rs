// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>
//
// Minimal example of reading a single line.

use libgpiod::{
    chip::Chip,
    line::{Config as LineConfig, Direction, Settings},
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
    rconfig.set_consumer("get-line-value")?;

    let chip = Chip::open(&chip_path)?;
    let request = chip.request_lines(Some(&rconfig), &lconfig)?;

    let value = request.value(line_offset)?;
    println!("{line_offset}={value:?}");
    Ok(())
}
