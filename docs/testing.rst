..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   Contribution guide.

Testing
=======

A comprehensive testing framework is included with the library and can be used
to test both the core library code as well as the kernel-to-user-space
interface.

The minimum kernel version required to run the tests can be checked in the
``tests/gpiod-test.c`` source file (it's subject to change if new features are
added to the kernel). The tests work together with the **gpio-sim** kernel
module which must either be built-in or available for loading using kmod.
A helper library - **libgpiosim** - is included to enable straightforward
interaction with the module.

To build the testing executable add the ``--enable-tests`` option when running
the configure script. If enabled, the tests will be installed next to
**gpio-tools**.

As opposed to standard autotools projects, libgpiod doesn't execute any tests
when invoking ``make check``. Instead the user must run them manually with
superuser privileges.

The testing framework uses the **GLib unit testing** library so development
package for GLib must be installed.

The **gpio-tools** programs can be tested separately using the
``gpio-tools-test.bash`` script. It requires `shunit2
<https://github.com/kward/shunit2>`_ to run and assumes that the tested
executables are in the same directory as the script.

C++, Rust and Python bindings also include their own test-suites. All three
reuse the **libgpiosim** library to avoid code duplication when interacting
with **gpio-sim**.

Python test-suite uses the standard unittest package. C++ tests use an external
testing framework - **Catch2** - which must be installed in the system. Rust
bindings use the standard tests module layout and the ``#[test]`` attribute.
