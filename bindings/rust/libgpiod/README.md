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

## License

This project is licensed under either of

- [Apache License](http://www.apache.org/licenses/LICENSE-2.0), Version 2.0
- [BSD-3-Clause License](https://opensource.org/licenses/BSD-3-Clause)
