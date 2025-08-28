// SPDX-License-Identifier: Apache-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2022-2023 Linaro Ltd.
// SPDX-FileCopyrightText: 2022 Viresh Kumar <viresh.kumar@linaro.org>
// SPDX-FileCopyrightText: 2023 Erik Schilling <erik.schilling@linaro.org>

use std::env;
use std::path::PathBuf;

fn main() {
    // Probe dependency info based on the metadata from Cargo.toml
    // (and potentially other sources like environment, pkg-config, ...)
    // https://docs.rs/system-deps/latest/system_deps/#overriding-build-flags
    let libs = system_deps::Config::new().probe().unwrap();

    // Tell cargo to invalidate the built crate whenever following files change
    println!("cargo:rerun-if-changed=wrapper.h");

    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let mut builder = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("wrapper.h")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()));

    // Inform bindgen about the include paths identified by system_deps.
    for (_name, lib) in libs {
        for include_path in lib.include_paths {
            builder = builder.clang_arg("-I").clang_arg(
                include_path
                    .to_str()
                    .expect("Failed to convert include_path to &str!"),
            );
        }
    }

    // Finish the builder and generate the bindings.
    let bindings = builder
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}
