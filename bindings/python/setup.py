# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from os import environ, path
from setuptools import setup, Extension, find_packages

def src(filename):
    return path.join(path.dirname(__file__), filename)

gpiod_ext = Extension(
    "gpiod._ext",
    sources=[
        src("gpiod/ext/chip.c"),
        src("gpiod/ext/common.c"),
        src("gpiod/ext/line-config.c"),
        src("gpiod/ext/line-settings.c"),
        src("gpiod/ext/module.c"),
        src("gpiod/ext/request.c"),
    ],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiod"],
    extra_compile_args=["-Wall", "-Wextra"],
)

gpiosim_ext = Extension(
    "tests.gpiosim._ext",
    sources=[src("tests/gpiosim/ext.c")],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiosim"],
    extra_compile_args=["-Wall", "-Wextra"],
)

procname_ext = Extension(
    "tests.procname._ext",
    sources=[src("tests/procname/ext.c")],
    define_macros=[("_GNU_SOURCE", "1")],
    extra_compile_args=["-Wall", "-Wextra"],
)

extensions = [gpiod_ext]
if "GPIOD_WITH_TESTS" in environ and environ["GPIOD_WITH_TESTS"] == "1":
    extensions.append(gpiosim_ext)
    extensions.append(procname_ext)

with open(src("gpiod/version.py"), "r") as fd:
    exec(fd.read())

setup(
    name="libgpiod",
    packages=find_packages(include=["gpiod"]),
    ext_modules=extensions,
    version=__version__,
    author="Bartosz Golaszewski",
    author_email="brgl@bgdev.pl",
    description="Python bindings for libgpiod",
    platforms=["linux"],
    license="LGPLv2.1",
)
