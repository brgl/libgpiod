# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

dist_bin_SCRIPTS = gpiod_py_test.py

pyexec_LTLIBRARIES = gpiomockup.la

gpiomockup_la_SOURCES = gpiomockupmodule.c
gpiomockup_la_CFLAGS = -I$(top_srcdir)/tests/mockup/
gpiomockup_la_CFLAGS += -Wall -Wextra -g -std=gnu89 $(PYTHON_CPPFLAGS)
gpiomockup_la_LDFLAGS = -module -avoid-version
gpiomockup_la_LIBADD = $(top_builddir)/tests/mockup/libgpiomockup.la
gpiomockup_la_LIBADD += $(PYTHON_LIBS)
