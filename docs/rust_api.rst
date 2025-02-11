..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

Where are the Rust bindings?
=============================

.. warning::
   There's currently no good way of integrating rust documentation with sphinx.
   Rust bindings should be documented on https://docs.rs/ but due to a yet
   unsolved build problem, this is currently not the case. Please refer to the
   in-source comments for now.

Rust bindings are available on https://crates.io/ as the ``libgpiod`` package.

.. note::
   When building the Rust bindings along the C library using make, they will
   be automatically configured to build against the build results of the
   C library. Building rust bindings requires cargo to be available on the
   system.
