# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import os
import unittest

from packaging import version

required_kernel_version = "5.19.0"
current_version = os.uname().release.split("-")[0]

if version.parse(current_version) < version.parse(required_kernel_version):
    raise NotImplementedError(
        "linux kernel version must be at least {} - got {}".format(
            required_kernel_version, current_version
        )
    )
