# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from ._ext import check_kernel_version, set_process_name

__all__ = ["check_kernel_version", "set_process_name"]
