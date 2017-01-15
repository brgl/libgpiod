#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#

AC_PREREQ(2.61)

AC_INIT([libgpiod], 0.0.0)
AC_SUBST(EXTRA_VERSION, [.git])

AC_DEFINE_UNQUOTED([GPIOD_VERSION_STR],
			["$PACKAGE_VERSION$EXTRA_VERSION"],
			[Full library version string.])

AC_CONFIG_AUX_DIR([autostuff])
AM_INIT_AUTOMAKE([foreign subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])

AC_ARG_ENABLE([tools],
	[AC_HELP_STRING([--enable-tools],
		[enable libgpiod command-line tools [default=no]])],
	[
		if test "x$enableval" = xyes
		then
			with_tools=true
		else
			with_tools=false
		fi
	],
	[with_tools = false])
AM_CONDITIONAL([WITH_TOOLS], [test x$with_tools = xtrue])

AM_PROG_AR
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_HEADER_STDC

AC_CHECK_FUNC([ioctl], [], [AC_MSG_ERROR([ioctl() unavailable])])
AC_CHECK_FUNC([asprintf], [], [AC_MSG_ERROR([asprintf() unavailable])])
AC_CHECK_FUNC([readdir], [], [AC_MSG_ERROR([readdir() unavailable])])
AC_CHECK_FUNC([poll], [], [AC_MSG_ERROR([poll() unavailable])])
AC_CHECK_FUNC([basename], [], [AC_MSG_ERROR([basename() unavailable])])

AC_FUNC_MALLOC

AC_CHECK_HEADERS([getopt.h], [], [AC_MSG_ERROR([getopt.h header not found])])
AC_CHECK_HEADERS([stdint.h], [], [AC_MSG_ERROR([stdint.h header not found])])
AC_CHECK_HEADERS([dirent.h], [], [AC_MSG_ERROR([dirent.h header not found])])
AC_CHECK_HEADERS([linux/gpio.h], [],
		[AC_MSG_ERROR([linux/gpio.h header not found])])

AC_CONFIG_FILES([Makefile
		 include/Makefile
		 src/Makefile
		 src/lib/Makefile
		 src/tools/Makefile])

AC_OUTPUT
