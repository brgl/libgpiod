# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

if WITH_MANPAGES

dist_man1_MANS = gpiodetect.man gpioinfo.man gpioget.man gpioset.man gpiofind.man gpiomon.man

%.man: $(top_builddir)/tools/$(*F)
	help2man $(top_builddir)/tools/$(*F) --include=$(srcdir)/template --output=$(builddir)/$@ --no-info

clean-local:
	rm -f $(dist_man1_MANS)

endif

EXTRA_DIST = template
