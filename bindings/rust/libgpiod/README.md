<!--
SPDX-License-Identifier: CC0-1.0
SPDX-FileCopyrightText: 2023 Linaro Ltd.
SPDX-FileCopyrightText: 2023 Erik Schilling <erik.schilling@linaro.org>
-->

# Safe wrapper around Rust FFI bindings for libgpiod

[libgpiod](https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/tree/README)
is a C library that provides an easy to use abstraction over the Linux GPIO
character driver. This crate builds on top of `libgpiod-sys` and exports a safe
interface to the C library.

## Build requirements

By default, `libgpiod-sys` builds against the libgpiod version identified via
`pkg-config`. See the `README.md` of `libgpiod-sys` for options to override
that.

Currently at least libgpiod 2.0 is required with the default feature set.

## Features

The Rust bindings will usually be built against whatever libgpiod version a
system provides. Hence, only the functionality of the oldest supported libgpiod
C library will be exposed by default.

Setting flags allows to increase the base version and export features of newer
versions:

- `v2_1`: Minimum version of `2.1.x`
- `vnext`: The upcoming, still unreleased version of the C lib

## Examples

Get GPIO chip information:

```rust
let chip = Chip::open(&Path::new("/dev/gpiochip0"))?;
let info = chip.info()?;

println!("{} [{}] ({} lines)", info.name()?, info.label()?, info.num_lines());
```

Print GPIO line name:

```rust
let chip = Chip::open(&Path::new("/dev/gpiochip0"))?;
let info = chip.line_info(0)?;
let name = info.name().unwrap_or("unnamed");

println!("{name}");
```

Toggle GPIO line output value:

```rust
let mut settings = line::Settings::new()?;
settings
    .set_direction(line::Direction::Output)?
    .set_output_value(Value::Active)?;

let mut line_cfg = line::Config::new()?;
line_cfg.add_line_settings(&[line_offset], settings)?;

let mut req_cfg = request::Config::new()?;
req_cfg.set_consumer("toggle-line-value")?;

let chip = Chip::open(&Path::new("/dev/gpiochip0"))?;

/* Request with value 1 */
let mut req = chip.request_lines(Some(&req_cfg), &line_cfg)?;

/* Toggle to value 0 */
req.set_value(line_offset, Value::InActive)?;
```

Read GPIO line event:

```rust
let mut lsettings = line::Settings::new()?;
lsettings
    .set_direction(line::Direction::Input)?
    .set_edge_detection(Some(line::Edge::Both))?;

let mut line_cfg = line::Config::new()?;
line_cfg.add_line_settings(&[line_offset], settings)?;

let mut req_cfg = request::Config::new()?;
req_cfg.set_consumer("toggle-line-value")?;

let chip = Chip::open(&Path::new("/dev/gpiochip0"))?;
let req = chip.request_lines(Some(&req_cfg), &line_cfg)?;

let mut buffer = request::Buffer::new(1)?;

loop {
    let events = req.read_edge_events(&mut buffer)?;
    for event in events {
        println!(
            "line: {} type: {}",
            event.line_offset(),
            match event.event_type()? {
                EdgeKind::Rising => "Rising",
                EdgeKind::Falling => "Falling",
            }
        );
    }
}
```

## License

This project is licensed under either of

- [Apache License](http://www.apache.org/licenses/LICENSE-2.0), Version 2.0
- [BSD-3-Clause License](https://opensource.org/licenses/BSD-3-Clause)
