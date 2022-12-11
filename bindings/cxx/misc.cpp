// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.hpp"

namespace gpiod {

GPIOD_CXX_API bool is_gpiochip_device(const ::std::filesystem::path& path)
{
	return ::gpiod_is_gpiochip_device(path.c_str());
}

GPIOD_CXX_API const ::std::string& api_version()
{
	static const ::std::string version(::gpiod_api_version());

	return version;
}

} /* namespace gpiod */
