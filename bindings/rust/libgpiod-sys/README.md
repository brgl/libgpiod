<!--
SPDX-License-Identifier: CC0-1.0
SPDX-FileCopyrightText: 2022 Linaro Ltd.
SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>
-->

# Generated libgpiod-sys Rust FFI bindings
Automatically generated Rust FFI bindings via
	[bindgen](https://github.com/rust-lang/rust-bindgen).

## Build requirements

A compatible variant of the C library needs to detectable using pkg-config.
Alternatively, one can inform the build system about the location of the
libs and headers by setting environment variables. The mechanism for that is
documented in the
[system_deps crate documentation](https://docs.rs/system-deps/6.1.0/system_deps/#overriding-build-flags).

If installing libgpiod is undesired, one can set the following environent
variables in order to build against the intermediate build results of a `make`
build of the C lib (paths are relative to the Cargo.toml):

	export SYSTEM_DEPS_LIBGPIOD_NO_PKG_CONFIG=1
	export SYSTEM_DEPS_LIBGPIOD_SEARCH_NATIVE="<PATH-TO-LIBGPIOD>/lib/.libs/"
	export SYSTEM_DEPS_LIBGPIOD_LIB=gpiod
	export SYSTEM_DEPS_LIBGPIOD_INCLUDE="<PATH-TO-LIBGPIOD>/include/"

## License

This project is licensed under either of

- [Apache License](http://www.apache.org/licenses/LICENSE-2.0), Version 2.0
- [BSD-3-Clause License](https://opensource.org/licenses/BSD-3-Clause)
