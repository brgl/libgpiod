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

The project is built using the meson build system. In the general case, the
steps needed to download a source tarball, unpack it, build the library
together with the command-line tools and install the resulting binaries are
as follows:

.. code-block:: none

   wget https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/libgpiod-x.y.z.tar.xz
   tar -xvf ./libgpiod-x.y.z.tar.xz
   cd ./libgpiod-x.y.z/
   mkdir build
   cd build
   meson setup .. -Dtools=enabled
   ninja
   sudo ninja install

The build system requires the following packages to be installed on the host
system for the basic build:

  * ``meson``
  * ``ninja-build``
  * ``pkg-config``

.. note::
   Development files for additional libraries may be required depending on
   selected options. Meson will report any missing additional required
   dependencies at configuration stage.

.. note::
   The command-line tools optionally depend on libedit for the interactive
   feature.

The project can also be built directly from the git repository.

For all configure features, use: ``meson introspect --buildoptions`` or see
the contents of the ``meson_options.txt`` file in the top-level directory of
the repository.

Installing
----------

To install the project run:

.. code-block:: none

   ninja install

.. note::
   The above may require superuser privileges.

This will install libgpiod under the default system paths. If you want to
install it under non-standard path, pass the ``--prefix=<install path>``
option to ``meson setup``.
