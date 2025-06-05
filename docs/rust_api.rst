..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   libgpiod Rust bindings documentation

libgpiod Rust bindings API
==========================

Rust bindings for libgpiod aim to provide a memory-safe interface to the
low-level C API. They are available on https://crates.io/ as the ``libgpiod``
package.

.. note::
   When building the Rust bindings along the C library using make, they will
   be automatically configured to build against the build results of the
   C library. Building rust bindings requires cargo to be available on the
   system.

.. warning::
   The documentation for Rust bindings is generated using ``cargo doc`` and
   cannot be easily integrated with sphinx documentation. Please navigate to
   a separate section dedicated exclusively to the Rust part of the API.


`Navigate to Rust bindings documentation <rust/doc/libgpiod/index.html>`_
