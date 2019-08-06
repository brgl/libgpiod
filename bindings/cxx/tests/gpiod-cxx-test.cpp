/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <linux/version.h>
#include <sys/utsname.h>
#include <system_error>
#include <sstream>

namespace {

class kernel_checker
{
public:

	kernel_checker(unsigned int major, unsigned int minor, unsigned int release)
	{
		unsigned long curr_major, curr_minor, curr_release, curr_ver, req_ver;
		::std::string major_str, minor_str, release_str;
		::utsname un;
		int ret;

		ret = ::uname(::std::addressof(un));
		if (ret)
			throw ::std::system_error(errno, ::std::system_category(),
						  "unable to read the kernel version");

		::std::stringstream ver_stream(::std::string(un.release));
		::std::getline(ver_stream, major_str, '.');
		::std::getline(ver_stream, minor_str, '.');
		::std::getline(ver_stream, release_str, '.');

		curr_major = ::std::stoul(major_str, nullptr, 0);
		curr_minor = ::std::stoul(minor_str, nullptr, 0);
		curr_release = ::std::stoul(release_str, nullptr, 0);

		curr_ver = KERNEL_VERSION(curr_major, curr_minor, curr_release);
		req_ver = KERNEL_VERSION(major, minor, release);

		if (curr_ver < req_ver)
			throw ::std::system_error(EOPNOTSUPP, ::std::system_category(),
						  "kernel release must be at least: " +
						  ::std::to_string(major) + "." +
						  ::std::to_string(minor) + "." +
						  ::std::to_string(release));
	}

	kernel_checker(const kernel_checker& other) = delete;
	kernel_checker(kernel_checker&& other) = delete;
	kernel_checker& operator=(const kernel_checker& other) = delete;
	kernel_checker& operator=(kernel_checker&& other) = delete;
};

kernel_checker require_kernel(5, 2, 7);

} /* namespace */
