// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <map>
#include <stdexcept>
#include <system_error>

#include "internal.hpp"

namespace gpiod {

void throw_from_errno(const ::std::string& what)
{
	switch (errno) {
	case EINVAL:
		throw ::std::invalid_argument(what);
	case E2BIG:
		throw ::std::length_error(what);
	case ENOMEM:
		throw ::std::bad_alloc();
	case EDOM:
		throw ::std::domain_error(what);
	default:
		throw ::std::system_error(errno, ::std::system_category(), what);
	}
}

} /* namespace gpiod */
