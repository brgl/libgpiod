# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl>

AC_PREREQ([2.71])

AC_INIT([libgpiod], [2.2], [], [],
AC_SUBST(EXTRA_VERSION, [])
	[https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/])

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
AC_SUBST(ABI_VERSION, [4.1.1])
# Have a separate ABI version for C++ bindings:
AC_SUBST(ABI_CXX_VERSION, [3.0.1])
# ABI version for libgpiosim (we need this since it can be installed if we
# enable tests).
AC_SUBST(ABI_GPIOSIM_VERSION, [1.1.0])
# ... and another one for GLib bindings:
AC_SUBST(ABI_GLIB_VERSION, [1.0.0])

AC_CONFIG_AUX_DIR([autostuff])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_pattern_forbid([^AX_],
	[Unexpanded AX_ macro found. Please install GNU autoconf-archive.])

AC_CONFIG_SRCDIR([lib])
AC_CONFIG_HEADERS([config.h])

AC_DEFINE([_GNU_SOURCE], [], [We want GNU extensions])

# Silence warning: ar: 'u' modifier ignored since 'D' is the default
AC_SUBST(AR_FLAGS, [cr])

AM_PROG_AR
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_EGREP

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
AC_CHECK_INCLUDES_DEFAULT

AC_FUNC_MALLOC
AC_HEADER_STDBOOL
AC_CHECK_FUNC([ioctl], [], [FUNC_NOT_FOUND_LIB([ioctl])])
AC_CHECK_FUNC([open], [], [FUNC_NOT_FOUND_LIB([open])])
AC_CHECK_FUNC([close], [], [FUNC_NOT_FOUND_LIB([close])])
AC_CHECK_FUNC([read], [], [FUNC_NOT_FOUND_LIB([read])])
AC_CHECK_FUNC([ppoll], [], [FUNC_NOT_FOUND_LIB([ppoll])])
AC_CHECK_FUNC([realpath], [], [FUNC_NOT_FOUND_LIB([realpath])])
AC_CHECK_FUNC([readlink], [], [FUNC_NOT_FOUND_LIB([readlink])])
AC_CHECK_HEADERS([fcntl.h], [], [HEADER_NOT_FOUND_LIB([fcntl.h])])
AC_CHECK_HEADERS([getopt.h], [], [HEADER_NOT_FOUND_LIB([getopt.h])])
AC_CHECK_HEADERS([dirent.h], [], [HEADER_NOT_FOUND_LIB([dirent.h])])
AC_CHECK_HEADERS([poll.h], [], [HEADER_NOT_FOUND_LIB([poll.h])])
AC_CHECK_HEADERS([sys/sysmacros.h], [], [HEADER_NOT_FOUND_LIB([sys/sysmacros.h])])
AC_CHECK_HEADERS([sys/ioctl.h], [], [HEADER_NOT_FOUND_LIB([sys/ioctl.h])])
AC_CHECK_HEADERS([sys/param.h], [], [HEADER_NOT_FOUND_LIB([sys/param.h])])
AC_CHECK_HEADERS([sys/stat.h], [], [HEADER_NOT_FOUND_LIB([sys/stat.h])])
AC_CHECK_HEADERS([sys/types.h], [], [HEADER_NOT_FOUND_LIB([sys/types.h])])
AC_CHECK_HEADERS([linux/const.h], [], [HEADER_NOT_FOUND_LIB([linux/const.h])])
AC_CHECK_HEADERS([linux/ioctl.h], [], [HEADER_NOT_FOUND_LIB([linux/ioctl.h])])
AC_CHECK_HEADERS([linux/types.h], [], [HEADER_NOT_FOUND_LIB([linux/types.h])])

AC_ARG_ENABLE([tools],
	[AS_HELP_STRING([--enable-tools],[enable libgpiod command-line tools [default=no]])],
	[if test "x$enableval" = xyes; then with_tools=true; fi],
	[with_tools=false])
AM_CONDITIONAL([WITH_TOOLS], [test "x$with_tools" = xtrue])

AC_DEFUN([FUNC_NOT_FOUND_TOOLS],
	[ERR_NOT_FOUND([$1()], [tools])])

AC_ARG_ENABLE([gpioset-interactive],
	[AS_HELP_STRING([--enable-gpioset-interactive],
		[enable gpioset interactive mode [default=no]])],
	[if test "x$enableval" = xyes; then with_gpioset_interactive=true; fi],
	[with_gpioset_interactive=false])
AM_CONDITIONAL([WITH_GPIOSET_INTERACTIVE],
	[test "x$with_gpioset_interactive" = xtrue])

# FIXME Figure out why this AS_IF() cannot be dropped without causing the other
# PKG_CHECK_MODULES() expansions fail to execute pkg-config.
AS_IF([test "x$with_tools" = xtrue],
	[# These are only needed to build tools
	AC_CHECK_FUNC([daemon], [], [FUNC_NOT_FOUND_TOOLS([daemon])])
	AC_CHECK_FUNC([asprintf], [], [FUNC_NOT_FOUND_TOOLS([asprintf])])
	AC_CHECK_FUNC([scandir], [], [FUNC_NOT_FOUND_TOOLS([scandir])])
	AC_CHECK_FUNC([versionsort], [], [FUNC_NOT_FOUND_TOOLS([versionsort])])
	AC_CHECK_FUNC([strtoull], [], [FUNC_NOT_FOUND_TOOLS([strtoull])])
	AC_CHECK_FUNC([nanosleep], [], [FUNC_NOT_FOUND_TOOLS([nanosleep])])
	AS_IF([test "x$with_gpioset_interactive" = xtrue],
		[PKG_CHECK_MODULES([LIBEDIT], [libedit >= 3.1])])
	])

AC_ARG_ENABLE([tests],
	[AS_HELP_STRING([--enable-tests],[enable libgpiod tests [default=no]])],
	[if test "x$enableval" = xyes; then with_tests=true; fi],
	[with_tests=false])
AM_CONDITIONAL([WITH_TESTS], [test "x$with_tests" = xtrue])

AC_ARG_ENABLE([profiling],
	[AS_HELP_STRING([--enable-profiling],
		[enable gcov profiling on the core library and tests [default=no]])],
	[if test "x$enableval" = xyes; then with_profiling=true; fi],
	[with_profiling=false])
if test "x$with_profiling" = xtrue
then
	AC_SUBST(PROFILING_CFLAGS, ["-fprofile-arcs -ftest-coverage"])
	AC_SUBST(PROFILING_LDFLAGS, ["-lgcov"])
fi

AC_DEFUN([FUNC_NOT_FOUND_TESTS],
	[ERR_NOT_FOUND([$1()], [tests])])

if test "x$with_tests" = xtrue
then
	# For libgpiosim
	PKG_CHECK_MODULES([KMOD], [libkmod >= 18])
	PKG_CHECK_MODULES([MOUNT], [mount >= 2.33.1])

	# For core library tests
	PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.50])
	PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.50])

	AC_CHECK_FUNC([atexit], [], [FUNC_NOT_FOUND_LIB([atexit])])
	AC_CHECK_FUNC([asprintf], [], [FUNC_NOT_FOUND_LIB([asprintf])])
	AC_CHECK_FUNC([prctl], [], [FUNC_NOT_FOUND_LIB([prctl])])
	AC_CHECK_FUNC([unlink], [], [FUNC_NOT_FOUND_LIB([unlink])])
	AC_CHECK_FUNC([unlinkat], [], [FUNC_NOT_FOUND_LIB([unlinkat])])
	AC_CHECK_FUNC([openat], [], [FUNC_NOT_FOUND_LIB([openat])])
	AC_CHECK_FUNC([mkdirat], [], [FUNC_NOT_FOUND_LIB([mkdirat])])
	AC_CHECK_FUNC([write], [], [FUNC_NOT_FOUND_LIB([write])])
	AC_CHECK_FUNC([twalk], [], [FUNC_NOT_FOUND_LIB([twalk])])
	AC_CHECK_FUNC([tsearch], [], [FUNC_NOT_FOUND_LIB([tsearch])])
	AC_CHECK_FUNC([tdestroy], [], [FUNC_NOT_FOUND_LIB([tdestroy])])
	AC_CHECK_FUNC([tdelete], [], [FUNC_NOT_FOUND_LIB([tdelete])])
	AC_CHECK_HEADERS([sys/utsname.h], [], [HEADER_NOT_FOUND_LIB([sys/utsname.h])])
	AC_CHECK_HEADERS([sys/mount.h], [], [HEADER_NOT_FOUND_LIB([sys/mount.h])])
	AC_CHECK_HEADERS([sys/prctl.h], [], [HEADER_NOT_FOUND_LIB([sys/prctl.h])])
	AC_CHECK_HEADERS([sys/random.h], [], [HEADER_NOT_FOUND_LIB([sys/random.h])])
	AC_CHECK_HEADERS([linux/version.h], [], [HEADER_NOT_FOUND_LIB([linux/version.h])])
	AC_CHECK_HEADERS([pthread.h], [], [HEADER_NOT_FOUND_LIB([pthread.h])])
	AC_CHECK_LIB(pthread, pthread_mutex_lock, [], ERR_NOT_FOUND([pthread library], [tests]))

	if test "x$with_tools" = xtrue
	then
		AC_CHECK_PROG([has_shunit2], [shunit2], [true], [false])
		if test "x$has_shunit2" = "xfalse"
		then
			AC_MSG_NOTICE([shunit2 not found - gpio-tools tests cannot be run])
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
	LT_LANG([C++])
	# This needs autoconf-archive
	AX_CXX_COMPILE_STDCXX_17([ext], [mandatory])

	if test "x$with_tests" = xtrue
	then
		PKG_CHECK_MODULES([CATCH2], [catch2-with-main >= 3.0],, [
			AC_LANG_PUSH([C++])
			AC_CHECK_HEADERS([catch2/catch_all.hpp], [],
					 [HEADER_NOT_FOUND_CXX([catch2/catch_all.hpp])])
			AC_LANG_POP([C++])
		])
	fi
fi

AC_ARG_ENABLE([bindings-python],
	[AS_HELP_STRING([--enable-bindings-python],[enable python3 bindings [default=no]])],
	[if test "x$enableval" = xyes; then with_bindings_python=true; fi],
	[with_bindings_python=false])
AM_CONDITIONAL([WITH_BINDINGS_PYTHON], [test "x$with_bindings_python" = xtrue])

if test "x$with_bindings_python" = xtrue
then
	AM_PATH_PYTHON([3.9], [],
		[AC_MSG_ERROR([python3 not found - needed for python bindings])])
	AC_CHECK_PROG([has_python_config], [python3-config], [true], [false])

	if test "x$has_python_config" = xfalse
	then
		AC_MSG_ERROR([python3-config not found - needed for python bindings])
	fi
fi

AC_ARG_ENABLE([bindings-rust],
	[AS_HELP_STRING([--enable-bindings-rust],[enable rust bindings [default=no]])],
	[if test "x$enableval" = xyes; then with_bindings_rust=true; fi],
	[with_bindings_rust=false])
AM_CONDITIONAL([WITH_BINDINGS_RUST], [test "x$with_bindings_rust" = xtrue])

if test "x$with_bindings_rust" = xtrue
then
	AC_CHECK_PROG([has_cargo], [cargo], [true], [false])
	if test "x$has_cargo" = xfalse
	then
		AC_MSG_ERROR([cargo not found - needed for rust bindings])
	fi
fi

AC_ARG_ENABLE([dbus],
	[AS_HELP_STRING([--enable-dbus], [build dbus daemon [default=no]])],
	[if test "x$enableval" == xyes; then with_dbus=true; fi],
	[with_dbus=false])
AM_CONDITIONAL([WITH_DBUS], [test "x$with_dbus" = xtrue])

AC_ARG_ENABLE([bindings-glib],
	[AS_HELP_STRING([--enable-bindings-glib],[enable GLib 2.0 bindings [default=no]])],
	[if test "x$enableval" = xyes; then with_bindings_glib=true; fi],
	[with_bindings_glib=false])

AC_DEFUN([FUNC_NOT_FOUND_DBUS],
	[ERR_NOT_FOUND([$1()], [dbus daemon])])

if test "x$with_dbus" = xtrue
then
	AC_CHECK_FUNC([daemon], [], [FUNC_NOT_FOUND_DBUS([daemon])])
	AC_CHECK_FUNC([strverscmp], [], [FUNC_NOT_FOUND_DBUS([strverscmp])])
	PKG_CHECK_MODULES([GUDEV], [gudev-1.0 >= 230])
	AC_CHECK_PROG([has_gdbus_codegen], [gdbus-codegen], [true], [false])
	if test "x$has_gdbus_codegen" = xfalse
	then
		AC_MSG_ERROR([gdbus-codegen not found - needed to build dbus daemon])
	fi

	# Imply GLib bindings for D-Bus
	with_bindings_glib=true
fi

AM_CONDITIONAL([WITH_BINDINGS_GLIB], [test "x$with_bindings_glib" = xtrue])

AC_DEFUN([GLIB_MKENUMS_NOT_FOUND],
	 [AC_MSG_ERROR([glib-mkenums not found - needed to build GLib bindings])])

if test "x$with_bindings_glib" = xtrue
then
	PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.80])
	PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= 2.80])
	PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.80])
	PKG_CHECK_MODULES([GIO_UNIX], [gio-unix-2.0 >= 2.80])
	PKG_PROG_PKG_CONFIG([0.28])
	PKG_CHECK_VAR([GLIB_MKENUMS], [glib-2.0], [glib_mkenums], [], GLIB_MKENUMS_NOT_FOUND)
	AC_CHECK_PROG([has_glib_mkenums], [glib-mkenums], [true], [false])
	if test "x$has_glib_mkenums" == xfalse
	then
		GLIB_MKENUMS_NOT_FOUND
	fi

	AC_CHECK_PROG([has_gi_docgen], [gi-docgen], [true], [false])
	if test "x$has_gi_docgen" = xfalse
	then
		AC_MSG_NOTICE([gi-docgen not found - GLib documentation cannot be generated])
	fi
fi
AM_CONDITIONAL([HAS_GI_DOCGEN], [test "x$has_gi_docgen" = xtrue])
AM_COND_IF([HAS_GI_DOCGEN], [AC_CONFIG_FILES([bindings/glib/gi-docgen.toml])])

# GObject-introspection
found_introspection=no
m4_ifdef([GOBJECT_INTROSPECTION_CHECK],
	[GOBJECT_INTROSPECTION_CHECK([0.6.2])],
	[AM_CONDITIONAL(HAVE_INTROSPECTION, test "x$found_introspection" = "xyes")])

AC_ARG_ENABLE([systemd],
	[AS_HELP_STRING([--enable-systemd], [enable systemd support [default=no]])],
	[if test "x$enableval" == xyes; then with_systemd=true; fi],
	[with_systemd=false])
AM_CONDITIONAL([WITH_SYSTEMD], [test "x$with_systemd" = xtrue])

if test "x$with_systemd" = xtrue
then
	PKG_CHECK_VAR([systemdsystemunitdir], [systemd], [systemdsystemunitdir], [],
		      AC_MSG_ERROR([systemdsystemunitdir not found - needed to enable systemd support]))
fi

AC_CHECK_PROG([has_doxygen], [doxygen], [true], [false])
AC_CHECK_PROG([has_sphinx], [sphinx-build], [true], [false])
AM_CONDITIONAL([WITH_DOCS], [test "x$has_doxygen" = xtrue && test "x$has_sphinx" = xtrue])
if test "x$has_doxygen" = xfalse
then
	AC_MSG_NOTICE([doxygen not found - documentation cannot be generated])
fi
if test "x$has_sphinx" = xfalse
then
	AC_MSG_NOTICE([sphinx-build not found - documentation cannot be generated])
fi

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
		 contrib/Makefile
		 docs/Makefile
		 examples/Makefile
		 tools/Makefile
		 tests/Makefile
		 tests/gpiosim/Makefile
		 tests/gpiosim-glib/Makefile
		 tests/harness/Makefile
		 tests/scripts/Makefile
		 bindings/cxx/libgpiodcxx.pc
		 bindings/Makefile
		 bindings/cxx/Makefile
		 bindings/cxx/gpiodcxx/Makefile
		 bindings/cxx/examples/Makefile
		 bindings/cxx/tests/Makefile
		 bindings/glib/gpiod-glib.pc
		 bindings/glib/Makefile
		 bindings/glib/examples/Makefile
		 bindings/glib/tests/Makefile
		 bindings/python/Makefile
		 bindings/python/gpiod/Makefile
		 bindings/python/gpiod/ext/Makefile
		 bindings/python/examples/Makefile
		 bindings/python/tests/Makefile
		 bindings/python/tests/gpiosim/Makefile
		 bindings/python/tests/procname/Makefile
		 bindings/rust/libgpiod-sys/src/Makefile
		 bindings/rust/libgpiod-sys/Makefile
		 bindings/rust/libgpiod/src/Makefile
		 bindings/rust/libgpiod/tests/common/Makefile
		 bindings/rust/libgpiod/tests/Makefile
		 bindings/rust/libgpiod/Makefile
		 bindings/rust/libgpiod/examples/Makefile
		 bindings/rust/Makefile
		 bindings/rust/gpiosim-sys/src/Makefile
		 bindings/rust/gpiosim-sys/Makefile
		 dbus/Makefile
		 dbus/client/Makefile
		 dbus/data/Makefile
		 dbus/lib/Makefile
		 dbus/manager/Makefile
		 dbus/tests/Makefile
		 man/Makefile])

AC_OUTPUT
