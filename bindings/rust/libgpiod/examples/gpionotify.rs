// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2023 Linaro Ltd.
// SPDX-FileCopyrightTest: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>
//
// Simplified Rust implementation of the gpionotify tool.

use std::env;

use libgpiod::{
    chip::Chip,
    line::{Offset, InfoChangeKind},
    Error, Result,
};

fn usage(name: &str) {
    println!("Usage: {} <chip> <offset0> ...", name);
}

fn main() -> Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        usage(&args[0]);
        return Err(Error::InvalidArguments);
    }

    let mut offsets = Vec::<Offset>::new();

    for arg in &args[2..] {
        let offset = arg.parse::<Offset>().map_err(|_| Error::InvalidArguments)?;
        offsets.push(offset);
    }

    let path = format!("/dev/gpiochip{}", args[1]);
    let chip = Chip::open(&path)?;

    for &offset in offsets.iter() {
        let _info = chip.watch_line_info(offset).unwrap();
    }

    loop {
        let event = chip.read_info_event().unwrap();
        println!(
            "event: {}, line: {}, timestamp: {:?}",
            match event.event_type()? {
                InfoChangeKind::LineRequested => "Line requested",
                InfoChangeKind::LineReleased => "Line released",
                InfoChangeKind::LineConfigChanged => "Line config changed",
            },
            event.line_info().unwrap().offset(),
            event.timestamp()
        );
    }
}
