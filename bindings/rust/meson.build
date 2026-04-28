# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

fs = import('fs')
libgpiod_so_dir = fs.parent(libgpiod_lib.full_path())
libgpiosim_a_dir = fs.parent(libgpiosim_static.full_path())
libgpiod_src_inc = meson.project_source_root() / 'include'
rust_src_dir = meson.current_source_dir()
rust_build_dir = meson.current_build_dir()

# We do not want to build against the system libs when building with meson. So
# we specify the paths to the build directory of the C lib.
cargo_env = environment()
cargo_env.set('SYSTEM_DEPS_LIBGPIOD_NO_PKG_CONFIG', '1')
cargo_env.set('SYSTEM_DEPS_LIBGPIOD_SEARCH_NATIVE', libgpiod_so_dir)
cargo_env.set('SYSTEM_DEPS_LIBGPIOD_LIB', 'gpiod')
cargo_env.set('SYSTEM_DEPS_LIBGPIOD_INCLUDE', libgpiod_src_inc)
# gpiosim-sys links against the static gpiosim archive via a hardcoded
# relative path in its build.rs, override that with RUSTFLAGS so the
# build succeeds with an out-of-tree meson build.
cargo_env.set('RUSTFLAGS', '-L ' + libgpiosim_a_dir)

# We use a stamp file as the custom_target output because cargo manages its
# own build artifacts inside the target directory.
rust_stamp = custom_target('rust-bindings',
  output:  'rust-bindings.stamp',
  command: [
    cargo,
    'build',
    '--features=vnext',
    '--release',
    '--lib',
    '--manifest-path', rust_src_dir / 'Cargo.toml',
    '--target-dir', rust_build_dir / 'target',
    '&&', 'touch', '@OUTPUT@',
  ],
  env: cargo_env,
  depends: [libgpiod_lib, libgpiosim_static],
  build_by_default: true,
)
