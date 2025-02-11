..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

Downloading, building & installing
==================================

Downloading
-----------

Libgpiod is packaged by several linux distributions. You should typically be
able to install it using your package manager. If you want to build libgpiod
from sources, the upstream git repository for libgpiod is hosted at
`kernel.org <https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/>`_.
together with
`release tarballs <https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/>`_.

Building
--------

This is a pretty standard autotools project. The core C library does not have
any external dependencies other than the standard C library with GNU extensions.

The build system requires the following packages to be installed on the host
system for the basic build:

  * ``autotools``
  * ``autoconf-archive``
  * ``libtool``
  * ``pkg-config``

.. note::
   Development files for additional libraries may be required depending on
   selected options. The configure script will report any missing additional
   required dependencies.

To build the project (including command-line utilities) run:

.. code-block:: none

   ./autogen.sh --enable-tools=yes
   make

.. note::
   The command-line tools optionally depend on libedit for the interactive
   feature.

The autogen script will execute ``./configure`` and pass all the command-line
arguments to it.

.. note::
   If building from release tarballs, the configure script is already provided
   and there's no need to invoke autogen.sh.

For all configure features, see: ``./configure --help``.

Installing
----------

To install the project run:

.. code-block:: none

   make install

.. note::
   The above may require superuser privileges.

This will install libgpiod under the default system paths. If you want to
install it under non-standard path, pass the ``--prefix=<install path>``
option to ``configure``.
