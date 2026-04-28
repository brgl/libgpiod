# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

py = import('python').find_installation('python3', required: true)

subdir('gpiod')

if opt_tests.allowed()
  subdir('tests')
endif
