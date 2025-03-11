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

The core C library does not have any external dependencies other than the
standard C library with GNU extensions.

The project is built using GNU autotools. In the general case, the steps needed
to download a source tarball, unpack it, build the library together with the
command-line tools and install the resulting binaries are as follows:

.. code-block:: none

   wget https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/libgpiod-x.y.z.tar.xz
   tar -xvf ./libgpiod-x.y.z.tar.xz
   cd ./libgpiod-x.y.z/
   ./configure --enable-tools
   make
   sudo make install

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

.. note::
   The command-line tools optionally depend on libedit for the interactive
   feature.

The project can also be built directly from the git repository. However in this
case the configure script does not exist and must be created first - either by
calling ``autoreconf``:

.. code-block:: none

   autoreconf -ifv
   ./configure --enable-tools
   make

Or by executing the provided ``autogen.sh`` script directly from the git tree:

.. code-block:: none

   ./autogen.sh --enable-tools
   make

.. note::
   The autogen script will execute ``./configure`` and pass all the
   command-line arguments to it.

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
