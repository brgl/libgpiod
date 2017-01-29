#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#

AUTOMAKE_OPTIONS = foreign
SUBDIRS = include src

if HAS_DOXYGEN

doc:
	@(cat Doxyfile; echo PROJECT_NUMBER = $(VERSION_STR)) | doxygen -
.PHONY: doc

endif
