# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2019 Bartosz Golaszewski <bartekgola@gmail.com>
#

if WITH_MANPAGES

dist_man1_MANS = gpiodetect.man gpioinfo.man gpioget.man gpioset.man gpiofind.man gpiomon.man

%.man: $(top_srcdir)/tools/$(*F)
	help2man $(top_srcdir)/tools/$(*F) --include=./template --output=./$@ --no-info

clean-local:
	rm -f $(dist_man1_MANS)

endif

EXTRA_DIST = template
