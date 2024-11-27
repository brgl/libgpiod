# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Phil Howard <phil@gadgetoid.com>

"""
Bring up just enough of setuptools/distutils in order to build the gpiod
test module C extensions.

Set "build_temp" and "build_lib" so that our source directory is not
polluted with artefacts in build/

Builds:

    tests/gpiosim/_ext.<target>.so
    tests/system/_ext.<target>.so

"""

import glob
import tempfile
from os import getenv, path

from setuptools import Distribution, Extension
from setuptools.command.build_ext import build_ext

TOP_SRCDIR = getenv("TOP_SRCDIR", "../../")
TOP_BUILDDIR = getenv("TOP_BUILDDIR", "../../")

# __version__
with open("gpiod/version.py", "r") as fd:
    exec(fd.read())

# The tests are run in-place with PYTHONPATH set to bindings/python
# so we need the gpiod extension module too.
gpiod_ext = Extension(
    "gpiod._ext",
    sources=glob.glob("gpiod/ext/*.c"),
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiod"],
    extra_compile_args=["-Wall", "-Wextra"],
    include_dirs=[
        path.join(TOP_SRCDIR, "include"),
    ],
    library_dirs=[
        path.join(TOP_BUILDDIR, "lib/.libs"),
    ],
)

gpiosim_ext = Extension(
    "tests.gpiosim._ext",
    sources=["tests/gpiosim/ext.c"],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiosim"],
    extra_compile_args=["-Wall", "-Wextra"],
    include_dirs=[
        path.join(TOP_SRCDIR, "include"),
        path.join(TOP_SRCDIR, "tests/gpiosim"),
    ],
    library_dirs=[
        path.join(TOP_BUILDDIR, "lib/.libs"),
        path.join(TOP_BUILDDIR, "tests/gpiosim/.libs"),
    ],
)

system_ext = Extension(
    "tests.system._ext",
    sources=["tests/system/ext.c"],
    define_macros=[("_GNU_SOURCE", "1")],
    extra_compile_args=["-Wall", "-Wextra"],
)

dist = Distribution(
    {
        "name": "gpiod",
        "ext_modules": [gpiosim_ext, system_ext, gpiod_ext],
        "version": __version__,
        "platforms": ["linux"],
    }
)

try:
    from setuptools.logging import configure

    configure()
except ImportError:
    try:
        from distutils.log import DEBUG, set_verbosity

        set_verbosity(DEBUG)
    except ImportError:
        # We can still build the tests, it will just be very quiet.
        pass

with tempfile.TemporaryDirectory(prefix="libgpiod-") as temp_dir:
    command = build_ext(dist)
    command.inplace = True
    command.build_temp = temp_dir
    command.build_lib = temp_dir
    command.finalize_options()
    command.run()
