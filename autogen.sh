#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
# SPDX-FileCopyrightText: 2017 Thierry Reding <treding@nvidia.com>

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir"

autoreconf --force --install --verbose || exit 1
cd $ORIGDIR || exit $?

if test -z "$NOCONFIGURE"; then
	exec "$srcdir"/configure "$@"
fi
