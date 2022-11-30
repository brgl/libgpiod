# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from os import environ
from setuptools import setup, Extension, find_packages

gpiod_ext = Extension(
    "gpiod._ext",
    sources=[
        "gpiod/ext/chip.c",
        "gpiod/ext/common.c",
        "gpiod/ext/line-config.c",
        "gpiod/ext/line-settings.c",
        "gpiod/ext/module.c",
        "gpiod/ext/request.c",
    ],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiod"],
    extra_compile_args=["-Wall", "-Wextra"],
)

gpiosim_ext = Extension(
    "tests.gpiosim._ext",
    sources=["tests/gpiosim/ext.c"],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiosim"],
    extra_compile_args=["-Wall", "-Wextra"],
)

extensions = [gpiod_ext]
with_tests = bool(environ["GPIOD_WITH_TESTS"])
if with_tests:
    extensions.append(gpiosim_ext)

with open("gpiod/version.py", "r") as fd:
    exec(fd.read())

setup(
    name="gpiod",
    packages=find_packages(include=["gpiod"]),
    ext_modules=extensions,
    version=__version__,
)
