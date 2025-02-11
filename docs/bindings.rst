..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   libgpiod high-level language bindings

High-level language bindings to libgpiod
========================================

Bindings provide a more straightforward interface to the core, low-level
C library. Object-oriented bindings for C++, GLib, python3 and Rust are
provided as part of the project. They can be enabled by passing
``--enable-bindings-cxx``, ``--enable-bindings-glib``,
``--enable-bindings-python`` and ``--enable-bindings-rust`` arguments to
configure respectively.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   cpp_api
   python_api
   glib_api
   rust_api
