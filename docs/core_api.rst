..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   libgpiod core API documentation

libgpiod core API
=================

This is the complete documentation of the public API made available to users of
**libgpiod**.

The API is logically split into several sections. For each opaque data class,
there's a set of functions for manipulating it. Together they can be thought
of as objects and their methods in OOP parlance.

.. note::
   General note on error handling: all functions exported by libgpiod that can
   fail, set errno to one of the error values defined in errno.h upon failure.
   The way of notifying the caller that an error occurred varies between
   functions, but in general a function that returns an ``int``, returns ``-1``
   on error, while a function returning a pointer indicates an error condition
   by returning a **NULL pointer**. It's not practical to list all possible
   error codes for every function as they propagate errors from the underlying
   libc functions.

In general libgpiod functions are **NULL-aware**. For functions that are
logically methods of data classes - ones that take a pointer to the object of
that class as the first argument - passing a NULL-pointer will result in the
program aborting the execution. For non-methods, init functions and methods
that take a pointer as any of the subsequent arguments, the handling of a
NULL-pointer depends on the implementation and may range from gracefully
handling it, ignoring it or returning an error.

**libgpiod** is **thread-aware** but does not provide any further thread-safety
guarantees. This requires the user to ensure that at most one thread may work
with an object at any time. Sharing objects across threads is allowed if
a suitable synchronization mechanism serializes the access. Different,
standalone objects can safely be used concurrently.

.. note::
   Most libgpiod objects are standalone. Exceptions - such as events allocated
   in buffers - exist and are noted in the documentation.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   core_chips
   core_chip_info
   core_line_defs
   core_line_info
   core_line_watch
   core_line_settings
   core_line_config
   core_request_config
   core_line_request
   core_edge_event
   core_misc
