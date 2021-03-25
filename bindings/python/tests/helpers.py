# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

import os


class LinkGuard:
    def __init__(self, src, dst):
        self.src = src
        self.dst = dst

    def __enter__(self):
        os.symlink(self.src, self.dst)

    def __exit__(self, type, val, tb):
        os.unlink(self.dst)
