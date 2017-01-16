#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#

AUTOMAKE_OPTIONS = foreign
SUBDIRS = include src

doc:
	@(cat Doxyfile; echo PROJECT_NUMBER = $(VERSION_STR)) | doxygen -
.PHONY: doc
