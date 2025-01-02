# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>
# SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

# This file is part of libgpiod.
#
# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path("../bindings/python").resolve()))

project = "libgpiod"
copyright = "2017-2024, Bartosz Golaszewski"
author = "Bartosz Golaszewski"

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

subprocess.run(["doxygen", "Doxyfile"])

master_doc = "index"
source_suffix = ".rst"

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
