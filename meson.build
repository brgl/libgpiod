# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

project('libgpiod', ['c', 'cpp'],
  version: '2.4',
  license: 'LGPL-2.1-or-later',
  default_options: [
    'c_std=gnu89',
    'cpp_std=gnu++17',
    'warning_level=2',
  ],
  meson_version: '>= 0.64.0',
)

extra_version = '+devel'
version_str = meson.project_version() + extra_version

project_url = 'https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/'

# Core libgpiod API version.
api_version = '2.2.1'

# Core C library ABI version.
libgpiod_soversion      = 3
libgpiod_version        = '3.1.2'
# Have a separate ABI version for C++ bindings:
libgpiodcxx_soversion   = 2
libgpiodcxx_version     = '2.1.1'
# ABI version for libgpiosim (we need this since it can be installed if we
# enable tests).
libgpiosim_soversion    = 1
libgpiosim_version      = '1.1.0'
# ... and another one for GLib bindings:
libgpiod_glib_soversion = 1
libgpiod_glib_version   = '1.0.0'
# ... and another for the libgpiotools shared library:
libgpiotools_soversion  = 1
libgpiotools_version    = '1.0.0'

add_project_arguments(
  '-D_GNU_SOURCE',
  '-DGPIOD_VERSION_STR="@0@"'.format(version_str),
  '-DGPIOD_API_VERSION_STR="@0@"'.format(api_version),
  language: ['c', 'cpp'],
)

opt_tools               = get_option('tools')
opt_gpioset_interactive = get_option('gpioset-interactive')
opt_tests               = get_option('tests')
opt_examples            = get_option('examples')
opt_bindings_cxx        = get_option('bindings-cxx')
opt_bindings_python     = get_option('bindings-python')
opt_bindings_rust       = get_option('bindings-rust')
opt_bindings_glib       = get_option('bindings-glib')
opt_dbus                = get_option('dbus')
opt_introspection       = get_option('introspection')
opt_systemd             = get_option('systemd')

if get_option('profiling')
  profiling_c_args    = ['-fprofile-arcs', '-ftest-coverage']
  profiling_link_args = ['-lgcov']
else
  profiling_c_args    = []
  profiling_link_args = []
endif

# D-Bus implies glib bindings.
if opt_dbus.enabled() and not opt_bindings_glib.enabled()
  opt_bindings_glib = opt_dbus
endif

# Introspection requires glib bindings.
if opt_introspection.enabled()
  assert(opt_bindings_glib.allowed(),
    'introspection requires -Dbindings-glib=enabled')
endif

# systemd requires D-Bus.
if opt_systemd.enabled()
  assert(opt_dbus.allowed(),
    'systemd support requires -Ddbus=enabled')
endif

libgpiod_inc = include_directories('include')

libedit_dep = dependency('libedit', version: '>= 3.1', required: opt_gpioset_interactive)

if opt_tests.allowed()
  libkmod_dep   = dependency('libkmod', version: '>= 18', required: opt_tests)
  mount_dep     = dependency('mount', version: '>= 2.33.1', required: opt_tests)
  threads_dep   = dependency('threads', required: opt_tests)
  # glib/gio are also used by tests (different min-version from bindings).
  glib_test_dep = dependency('glib-2.0', version: '>= 2.74', required: opt_tests)
  gio_test_dep  = dependency('gio-2.0', version: '>= 2.74', required: opt_tests)

  tests_enabled = libkmod_dep.found() and mount_dep.found()
else
  tests_enabled = false
endif

if opt_bindings_cxx.allowed() and tests_enabled
  catch2_test_dep = dependency('catch2-with-main', version: '>= 3.0', required: false)
  cxx_tests_enabled = catch2_test_dep.found()
  if not cxx_tests_enabled
    # Some distros don't ship pkgconfig or cmake info for catch2 but they do
    # package it so check the header.
    cxx = meson.get_compiler('cpp')
    if cxx.has_header('catch2/catch_all.hpp')
      catch2_test_dep = declare_dependency()
      cxx_tests_enabled = true
    endif
  endif
  if not cxx_tests_enabled
    warning('catch2 not found, C++ bindings tests will not be built')
  endif
else
  cxx_tests_enabled = false
endif

if opt_bindings_glib.allowed()
  glib_dep      = dependency('glib-2.0', version: '>= 2.80', required: opt_bindings_glib)
  gobject_dep   = dependency('gobject-2.0', version: '>= 2.80', required: opt_bindings_glib)
  gio_dep       = dependency('gio-2.0', version: '>= 2.80', required: opt_bindings_glib)
  gio_unix_dep  = dependency('gio-unix-2.0', version: '>= 2.80', required: opt_bindings_glib)
  glib_mkenums  = find_program('glib-mkenums', required: opt_bindings_glib)

  glib_enabled = glib_dep.found() and glib_mkenums.found()
else
  glib_enabled = false
endif

if opt_dbus.allowed()
  gudev_dep      = dependency('gudev-1.0', version: '>= 230', required: opt_dbus)
  gdbus_codegen  = find_program('gdbus-codegen', required: opt_dbus)

  dbus_enabled = gudev_dep.found() and gdbus_codegen.found()
else
  dbus_enabled = false
endif

if opt_introspection.allowed()
  gir_dep       = dependency('gobject-introspection-1.0', version: '>= 0.6.2',
                             required: opt_introspection)
  g_ir_scanner  = find_program('g-ir-scanner', required: opt_introspection)
  g_ir_compiler = find_program('g-ir-compiler', required: opt_introspection)

  introspection_enabled = gir_dep.found() and g_ir_scanner.found() and g_ir_compiler.found()
else
  introspection_enabled = false
endif

if opt_systemd.allowed()
  systemd_dep = dependency('systemd', required: opt_systemd)
  if systemd_dep.found()
    opt_systemd_unit_dir = get_option('systemd-unit-dir')
    if opt_systemd_unit_dir != ''
      systemd_unit_dir = opt_systemd_unit_dir
    else
      systemd_unit_dir = systemd_dep.get_variable('systemdsystemunitdir')
    endif
  endif

  systemd_enabled = systemd_dep.found()
else
  systemd_enabled = false
endif

if opt_bindings_rust.allowed()
  cargo = find_program('cargo', required: opt_bindings_rust)

  rust_enabled = cargo.found()
else
  rust_enabled = false
endif

help2man = find_program('help2man', required: false)

subdir('include')
subdir('lib')

tools_enabled = opt_tools.allowed()
if tools_enabled
  subdir('tools')
endif

examples_enabled = opt_examples.allowed()
if examples_enabled
  subdir('examples')
endif

if tests_enabled
  subdir('tests')
endif

subdir('bindings')

if dbus_enabled
  subdir('dbus')
endif

if help2man.found() and meson.can_run_host_binaries()
  subdir('man')
endif

summary({
  'Tools'                 : tools_enabled,
  'Tests'                 : tests_enabled,
  'Examples'              : examples_enabled,
  'D-Bus'                 : dbus_enabled,
}, section: 'Components', bool_yn: true)

bindings_summary = {'C++' : opt_bindings_cxx.allowed()}
if opt_bindings_cxx.allowed()
  bindings_summary += {'C++ tests' : cxx_tests_enabled}
endif
bindings_summary += {
  'Python'                : opt_bindings_python.allowed(),
  'Rust'                  : rust_enabled,
  'GLib'                  : glib_enabled,
}
summary(bindings_summary, section: 'Bindings', bool_yn: true)

summary({
  'gpioset interactive'   : libedit_dep.found(),
  'GObject introspection' : introspection_enabled,
  'systemd'               : systemd_enabled,
  'Profiling'             : get_option('profiling'),
}, section: 'Features', bool_yn: true)
