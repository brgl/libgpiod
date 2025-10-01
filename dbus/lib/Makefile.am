# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

libgpiodbus_la_CFLAGS = -include $(top_builddir)/config.h -Wall -Wextra -g
libgpiodbus_la_CFLAGS += $(GLIB_CFLAGS) $(GIO_CFLAGS)
libgpiodbus_la_CFLAGS += -DG_LOG_DOMAIN=\"gpiodbus\"
libgpiodbus_la_LDFLAGS = -version-info 1

generated-gpiodbus.h generated-gpiodbus.c: io.gpiod1.xml
	$(AM_V_GEN)gdbus-codegen \
		--interface-prefix io.gpiod1 \
		--c-namespace Gpiodbus \
		--generate-c-code generated-gpiodbus \
		--c-generate-object-manager \
		--c-generate-autocleanup=all \
		--glib-min-required 2.74.0 \
		$(srcdir)/io.gpiod1.xml

lib_LTLIBRARIES = libgpiodbus.la
libgpiodbus_la_SOURCES = \
	generated-gpiodbus.c \
	generated-gpiodbus.h \
	gpiodbus.h

BUILT_SOURCES = generated-gpiodbus.c generated-gpiodbus.h
CLEANFILES = $(BUILT_SOURCES)

dbusdir = $(datadir)/dbus-1/interfaces
dist_dbus_DATA = io.gpiod1.xml
