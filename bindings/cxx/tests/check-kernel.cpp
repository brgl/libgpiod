// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <linux/version.h>
#include <sys/utsname.h>
#include <system_error>
#include <sstream>

namespace {

class kernel_checker
{
public:
	kernel_checker(int major, int minor, int release)
	{
		int curr_major, curr_minor, curr_release, curr_ver, req_ver;
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
		::std::getline(ver_stream, release_str, '-');

		curr_major = ::std::stoi(major_str, nullptr, 0);
		curr_minor = ::std::stoi(minor_str, nullptr, 0);
		curr_release = ::std::stoi(release_str, nullptr, 0);

		curr_ver = KERNEL_VERSION(curr_major, curr_minor, curr_release);
		req_ver = KERNEL_VERSION(major, minor, release);

		if (curr_ver < req_ver)
			throw ::std::runtime_error("kernel release must be at least: " +
						   ::std::to_string(major) + "." +
						   ::std::to_string(minor) + "." +
						   ::std::to_string(release));
	}
};

kernel_checker require_kernel(5, 19, 0);

} /* namespace */
