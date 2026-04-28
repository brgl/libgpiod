# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Qualcomm Technologies, Inc. and/or its subsidiaries

if opt_bindings_cxx.allowed()
  subdir('cxx')
endif

if opt_bindings_glib.allowed() and glib_dep.found() and glib_mkenums.found()
  subdir('glib')
endif

if opt_bindings_python.allowed()
  subdir('python')
endif

if opt_bindings_rust.allowed()
  subdir('rust')
endif
