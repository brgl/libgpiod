#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#

AM_CPPFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CPPFLAGS += -Wall -Wextra -g -std=gnu++11
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/

check_PROGRAMS =	gpio_cxx_tests \
			gpiodetectcxx \
			gpiofindcxx \
			gpiogetcxx \
			gpioinfocxx \
			gpiomoncxx \
			gpiosetcxx

gpio_cxx_tests_SOURCES = gpio_cxx_tests.cpp

gpiodetectcxx_SOURCES = gpiodetectcxx.cpp

gpiofindcxx_SOURCES = gpiofindcxx.cpp

gpiogetcxx_SOURCES = gpiogetcxx.cpp

gpioinfocxx_SOURCES = gpioinfocxx.cpp

gpiomoncxx_SOURCES = gpiomoncxx.cpp

gpiosetcxx_SOURCES = gpiosetcxx.cpp
