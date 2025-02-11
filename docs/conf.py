# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

# This file is part of libgpiod.
#
# Configuration file for the Sphinx documentation builder.

import os
import re
import subprocess
import sys

project = "libgpiod"
copyright = "2017-2025, Bartosz Golaszewski"
author = "Bartosz Golaszewski"

master_doc = "index"
source_suffix = ".rst"

# Extract the full version from configure.ac (including -devel, -rc and other
# tags).
with open("../configure.ac", "r") as fd:
    version = ""
    extra = ""
    for line in fd.readlines():
        match = re.search(r"AC_INIT\(\[libgpiod\], \[(.*?)\]\)", line)
        if match:
            version = match.group(1)
            continue

        match = re.search(r"AC_SUBST\(EXTRA_VERSION, \[(.*?)\]\)", line)
        if match:
            extra = match.group(1)

        release = f"{version}{extra}"

extensions = ["breathe"]

breathe_projects = {"libgpiod": "./doxygen-output/xml"}
breathe_default_project = "libgpiod"

# Use the RTD theme if available
sphinx_rtd_theme = None
try:
    import sphinx_rtd_theme

    extensions.append("sphinx_rtd_theme")
except ImportError:
    pass

html_theme = "sphinx_rtd_theme" if sphinx_rtd_theme else "default"

subprocess.run(["doxygen", "Doxyfile"])
