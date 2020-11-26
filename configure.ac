# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2019 Bartosz Golaszewski <bartekgola@gmail.com>
#

AC_PREREQ([2.69])

AC_INIT([libgpiod],[2.0])
AC_SUBST(EXTRA_VERSION, [-devel])

AC_DEFINE_UNQUOTED([GPIOD_VERSION_STR],
			["$PACKAGE_VERSION$EXTRA_VERSION"],
			[Full library version string.])
AC_SUBST(VERSION_STR, [$PACKAGE_VERSION$EXTRA_VERSION])

# From the libtool manual:
#
# (...)
# 3. If the library source code has changed at all since the last update, then
#    increment revision ('c:r:a' becomes 'c:r+1:a').
# 4. If any interfaces have been added, removed, or changed since the last
#    update, increment current, and set revision to 0.
# 5. If any interfaces have been added since the last public release, then
#    increment age.
# 6. If any interfaces have been removed or changed since the last public
#    release, then set age to 0.
#
# Define the libtool version as (C.R.A):
# NOTE: this version only applies to the core C library.
AC_SUBST(ABI_VERSION, [4.1.2])
# Have a separate ABI version for C++ bindings:
AC_SUBST(ABI_CXX_VERSION, [2.1.1])
# ABI version for libgpiomockup (we need this since it can be installed if we
# enable install-tests).
AC_SUBST(ABI_MOCKUP_VERSION, [0.1.0])

AC_CONFIG_AUX_DIR([autostuff])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_pattern_forbid([^AX_],
	[Unexpanded AX_ macro found. Please install GNU autoconf-archive.])

AC_ARG_VAR([PYTHON_CPPFLAGS],
	[Compiler flags to find Python headers [default: auto-detect]])
AC_ARG_VAR([PYTHON_LIBS],
	[Libraries to link into Python extensions [default: auto-detect]])

AC_CONFIG_SRCDIR([lib])
AC_CONFIG_HEADER([config.h])

AC_DEFINE([_GNU_SOURCE], [], [We want GNU extensions])

# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])

AM_PROG_AR
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT

AC_DEFUN([ERR_NOT_FOUND],
	[AC_MSG_ERROR([$1 not found (needed to build $2)], [1])])

AC_DEFUN([FUNC_NOT_FOUND_LIB],
	[ERR_NOT_FOUND([$1()], [the library])])

AC_DEFUN([HEADER_NOT_FOUND_LIB],
	[ERR_NOT_FOUND([$1 header], [the library])])

AC_DEFUN([HEADER_NOT_FOUND_TESTS],
	[ERR_NOT_FOUND([$1 header], [the test suite])])

AC_DEFUN([HEADER_NOT_FOUND_CXX],
	[ERR_NOT_FOUND([$1 header], [C++ bindings])])

# This is always checked (library needs this)
AC_HEADER_STDC
AC_FUNC_MALLOC
AC_CHECK_FUNC([ioctl], [], [FUNC_NOT_FOUND_LIB([ioctl])])
AC_CHECK_FUNC([asprintf], [], [FUNC_NOT_FOUND_LIB([asprintf])])
AC_CHECK_FUNC([scandir], [], [FUNC_NOT_FOUND_LIB([scandir])])
AC_CHECK_FUNC([alphasort], [], [FUNC_NOT_FOUND_LIB([alphasort])])
AC_CHECK_FUNC([ppoll], [], [FUNC_NOT_FOUND_LIB([ppoll])])
AC_CHECK_FUNC([realpath], [], [FUNC_NOT_FOUND_LIB([realpath])])
AC_CHECK_HEADERS([getopt.h], [], [HEADER_NOT_FOUND_LIB([getopt.h])])
AC_CHECK_HEADERS([dirent.h], [], [HEADER_NOT_FOUND_LIB([dirent.h])])
AC_CHECK_HEADERS([sys/poll.h], [], [HEADER_NOT_FOUND_LIB([sys/poll.h])])
AC_CHECK_HEADERS([sys/sysmacros.h], [], [HEADER_NOT_FOUND_LIB([sys/sysmacros.h])])
AC_CHECK_HEADERS([linux/gpio.h], [], [HEADER_NOT_FOUND_LIB([linux/gpio.h])])
AC_CHECK_HEADERS([linux/version.h], [], [HEADER_NOT_FOUND_LIB([linux/version.h])])

AC_COMPILE_IFELSE([AC_LANG_SOURCE(
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
#error
#endif
)],
[], [AC_MSG_ERROR(["libgpiod needs linux headers version >= v5.10.0"])])

AC_ARG_ENABLE([tools],
	[AS_HELP_STRING([--enable-tools],[enable libgpiod command-line tools [default=no]])],
	[if test "x$enableval" = xyes; then with_tools=true; fi],
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
	AC_CHECK_FUNC([setlinebuf], [], [FUNC_NOT_FOUND_TOOLS([setlinebuf])])
	AC_CHECK_HEADERS([sys/signalfd.h], [], [HEADER_NOT_FOUND_TOOLS([sys/signalfd.h])])
fi

AC_ARG_ENABLE([tests],
	[AS_HELP_STRING([--enable-tests],[enable libgpiod tests [default=no]])],
	[if test "x$enableval" = xyes; then with_tests=true; fi],
	[with_tests=false])
AM_CONDITIONAL([WITH_TESTS], [test "x$with_tests" = xtrue])

AC_DEFUN([FUNC_NOT_FOUND_TESTS],
	[ERR_NOT_FOUND([$1()], [tests])])

if test "x$with_tests" = xtrue
then
	# For libgpiomockup
	AC_CHECK_FUNC([qsort], [], [FUNC_NOT_FOUND_TESTS([qsort])])
	PKG_CHECK_MODULES([KMOD], [libkmod >= 18])
	PKG_CHECK_MODULES([UDEV], [libudev >= 215])

	# For core library tests
	PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.50])

	if test "x$with_tools" = xtrue
	then
		AC_CHECK_PROG([has_bats], [bats], [true], [false])
		if test "x$has_bats" = "xflase"
		then
			AC_MSG_NOTICE(["bats not found - gpio-tools tests cannot be run])
		fi
	fi
fi

AC_ARG_ENABLE([examples],
	[AS_HELP_STRING([--enable-examples], [enable building code examples[default=no]])],
	[if test "x$enableval" = xyes; then with_examples=true; fi],
	[with_examples=false])
AM_CONDITIONAL([WITH_EXAMPLES], [test "x$with_examples" = xtrue])

AC_ARG_ENABLE([bindings-cxx],
	[AS_HELP_STRING([--enable-bindings-cxx],[enable C++ bindings [default=no]])],
	[if test "x$enableval" = xyes; then with_bindings_cxx=true; fi],
	[with_bindings_cxx=false])
AM_CONDITIONAL([WITH_BINDINGS_CXX], [test "x$with_bindings_cxx" = xtrue])

if test "x$with_bindings_cxx" = xtrue
then
	AC_LIBTOOL_CXX
	# This needs autoconf-archive
	AX_CXX_COMPILE_STDCXX_11([ext], [mandatory])

	if test "x$with_tests" = xtrue
	then
		PKG_CHECK_MODULES([CATCH2], [catch2],, [
			AC_LANG_PUSH([C++])
			AC_CHECK_HEADERS([catch2/catch.hpp], [], [HEADER_NOT_FOUND_CXX([catch2/catch.hpp])])
			AC_LANG_POP([C++])
		])
	fi

	if test "x$with_examples" = xtrue
	then
		# Examples use C++17 features
		AX_CXX_COMPILE_STDCXX([17], [ext], [mandatory])
	fi
fi

AC_ARG_ENABLE([bindings-python],
	[AS_HELP_STRING([--enable-bindings-python],[enable python3 bindings [default=no]])],
	[if test "x$enableval" = xyes; then with_bindings_python=true; fi],
	[with_bindings_python=false])
AM_CONDITIONAL([WITH_BINDINGS_PYTHON], [test "x$with_bindings_python" = xtrue])

if test "x$with_bindings_python" = xtrue
then
	AM_PATH_PYTHON([3.0], [],
		[AC_MSG_ERROR([python3 not found - needed for python bindings])])
	AC_CHECK_PROG([has_python_config], [python3-config], [true], [false])
	if test "x$has_python_config" = xfalse
	then
		AC_MSG_ERROR([python3-config not found - needed for python bindings])
	fi
	AS_IF([test -z "$PYTHON_CPPFLAGS"],
		[AC_SUBST(PYTHON_CPPFLAGS, [`$PYTHON-config --includes`])])
	AS_IF([test -z "$PYTHON_LIBS"],
		[AC_SUBST(PYTHON_LIBS, [`$PYTHON-config --libs`])])
fi

AC_CHECK_PROG([has_doxygen], [doxygen], [true], [false])
AM_CONDITIONAL([HAS_DOXYGEN], [test "x$has_doxygen" = xtrue])
if test "x$has_doxygen" = xfalse
then
	AC_MSG_NOTICE([doxygen not found - documentation cannot be generated])
fi
AM_COND_IF([HAS_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])

if test "x$cross_compiling" = xno
then
	AC_CHECK_PROG([has_help2man], [help2man], [true], [false])
fi
AM_CONDITIONAL([WITH_MANPAGES], [test "x$has_help2man" = xtrue])
if test "x$has_help2man" = xfalse
then
	AC_MSG_NOTICE([help2man not found - man pages cannot be generated automatically])
fi

AC_CONFIG_FILES([Makefile
		 include/Makefile
		 lib/Makefile
		 lib/libgpiod.pc
		 tools/Makefile
		 tests/Makefile
		 tests/mockup/Makefile
		 bindings/cxx/libgpiodcxx.pc
		 bindings/Makefile
		 bindings/cxx/Makefile
		 bindings/cxx/examples/Makefile
		 bindings/cxx/tests/Makefile
		 bindings/python/Makefile
		 bindings/python/examples/Makefile
		 bindings/python/tests/Makefile
		 man/Makefile])

AC_OUTPUT
