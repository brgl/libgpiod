#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#

AC_PREREQ(2.61)

AC_INIT([libgpiod], 0.2)
AC_SUBST(EXTRA_VERSION, [.devel])

AC_DEFINE_UNQUOTED([GPIOD_VERSION_STR],
			["$PACKAGE_VERSION$EXTRA_VERSION"],
			[Full library version string.])
AC_SUBST(VERSION_STR, [$PACKAGE_VERSION$EXTRA_VERSION])

AC_CONFIG_AUX_DIR([autostuff])
AM_INIT_AUTOMAKE([foreign subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])

AC_DEFINE([_GNU_SOURCE], [], [We want GNU extensions])

AM_PROG_AR
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL

AC_DEFUN([ERR_NOT_FOUND],
	[AC_MSG_ERROR([$1 not found (needed to build $2)], [1])])

AC_DEFUN([FUNC_NOT_FOUND_LIB],
	[ERR_NOT_FOUND([$1()], [the library])])

AC_DEFUN([HEADER_NOT_FOUND_LIB],
	[ERR_NOT_FOUND([$1 header], [the library])])

# This is always checked (library needs this)
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNC([ioctl], [], [FUNC_NOT_FOUND_LIB([ioctl])])
AC_CHECK_FUNC([asprintf], [], [FUNC_NOT_FOUND_LIB([asprintf])])
AC_CHECK_FUNC([readdir], [], [FUNC_NOT_FOUND_LIB([readdir])])
AC_CHECK_FUNC([ppoll], [], [FUNC_NOT_FOUND_LIB([ppoll])])
AC_CHECK_HEADERS([getopt.h], [], [HEADER_NOT_FOUND_LIB([getopt.h])])
AC_CHECK_HEADERS([dirent.h], [], [HEADER_NOT_FOUND_LIB([dirent.h])])
AC_CHECK_HEADERS([sys/poll.h], [], [HEADER_NOT_FOUND_LIB([sys/poll.h])])
AC_CHECK_HEADERS([linux/gpio.h], [], [HEADER_NOT_FOUND_LIB([linux/gpio.h])])

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
	[with_tools=false])
AM_CONDITIONAL([WITH_TOOLS], [test "x$with_tools" = xtrue])

AC_DEFUN([FUNC_NOT_FOUND_TOOLS],
	[ERR_NOT_FOUND([$1()], [tools])])

AC_DEFUN([HEADER_NOT_FOUND_TOOLS],
	[ERR_NOT_FOUND([$1 header], [tools])])

if test "x$with_tools" = xtrue
then
	# These are only needed to build tools
	AC_CHECK_FUNC([basename], [], [FUNC_NOT_FOUND_TOOLS([basename])])
	AC_CHECK_FUNC([daemon], [], [FUNC_NOT_FOUND_TOOLS([daemon])])
	AC_CHECK_FUNC([signalfd], [], [FUNC_NOT_FOUND_TOOLS([signalfd])])
	AC_CHECK_HEADERS([sys/signalfd.h], [], [HEADER_NOT_FOUND_TOOLS([sys/signalfd.h])])
fi

AC_ARG_ENABLE([tests],
	[AC_HELP_STRING([--enable-tests],
		[enable libgpiod tests [default=no]])],
	[
		if test "x$enableval" = xyes
		then
			with_tests=true
		else
			with_tests=false
		fi
	],
	[with_tests=false])
AM_CONDITIONAL([WITH_TESTS], [test "x$with_tests" = xtrue])

AC_DEFUN([HEADER_NOT_FOUND_TESTS],
	[ERR_NOT_FOUND([$1 header], [tests])])

AC_DEFUN([FUNC_NOT_FOUND_TESTS],
	[ERR_NOT_FOUND([$1()], [tests])])

if test "x$with_tools" = xtrue
then
	AC_CHECK_LIB([kmod], [kmod_module_probe_insert_module], [],
		[AC_MSG_ERROR([libkmod not found (needed to build tests])])
	AC_CHECK_HEADERS([libkmod.h], [], [HEADER_NOT_FOUND_TESTS([libkmod.h])])
	AC_CHECK_FUNC([qsort], [], [FUNC_NOT_FOUND_TESTS([qsort])])
fi

AC_CHECK_PROG([has_doxygen], [doxygen], [true], [false])
AM_CONDITIONAL([HAS_DOXYGEN], [test "x$has_doxygen" = xtrue])
if test "x$has_doxygen" = xfalse
then
	AC_MSG_NOTICE([doxygen not found - documentation cannot be generated])
fi

AC_CONFIG_FILES([Makefile
		 include/Makefile
		 src/Makefile
		 src/lib/Makefile
		 src/tools/Makefile
		 tests/Makefile
		 tests/unit/Makefile])

AC_OUTPUT
