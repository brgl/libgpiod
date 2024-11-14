# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import os
from distutils.version import LooseVersion

required_kernel_version = LooseVersion("5.19.0")
current_version = LooseVersion(os.uname().release.split("-")[0])

if current_version < required_kernel_version:
    raise NotImplementedError(
        f"linux kernel version must be at least {required_kernel_version} - got {current_version}"
    )
