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

- `vnext`: The upcoming, still unreleased version of the C lib

## License

This project is licensed under either of

- [Apache License](http://www.apache.org/licenses/LICENSE-2.0), Version 2.0
- [BSD-3-Clause License](https://opensource.org/licenses/BSD-3-Clause)
