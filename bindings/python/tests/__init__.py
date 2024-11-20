# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>
# SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

from .system import check_kernel_version

_required_kernel_version = (5, 19, 0)

if not check_kernel_version(*_required_kernel_version):
    raise NotImplementedError(
        f"linux kernel version must be at least v{'.'.join([str(i) for i in _required_kernel_version])}"
    )
