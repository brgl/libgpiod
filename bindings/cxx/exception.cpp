// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.hpp"

namespace gpiod {

GPIOD_CXX_API chip_closed::chip_closed(const ::std::string& what)
	: ::std::logic_error(what)
{

}

GPIOD_CXX_API chip_closed::chip_closed(const chip_closed& other) noexcept
	: ::std::logic_error(other)
{

}

GPIOD_CXX_API chip_closed::chip_closed(chip_closed&& other) noexcept
	: ::std::logic_error(other)
{

}

GPIOD_CXX_API chip_closed& chip_closed::operator=(const chip_closed& other) noexcept
{
	::std::logic_error::operator=(other);

	return *this;
}

GPIOD_CXX_API chip_closed& chip_closed::operator=(chip_closed&& other) noexcept
{
	::std::logic_error::operator=(other);

	return *this;
}

GPIOD_CXX_API chip_closed::~chip_closed()
{

}

GPIOD_CXX_API request_released::request_released(const ::std::string& what)
	: ::std::logic_error(what)
{

}

GPIOD_CXX_API request_released::request_released(const request_released& other) noexcept
	: ::std::logic_error(other)
{

}

GPIOD_CXX_API request_released::request_released(request_released&& other) noexcept
	: ::std::logic_error(other)
{

}

GPIOD_CXX_API request_released& request_released::operator=(const request_released& other) noexcept
{
	::std::logic_error::operator=(other);

	return *this;
}

GPIOD_CXX_API request_released& request_released::operator=(request_released&& other) noexcept
{
	::std::logic_error::operator=(other);

	return *this;
}

GPIOD_CXX_API request_released::~request_released()
{

}

GPIOD_CXX_API bad_mapping::bad_mapping(const ::std::string& what)
	: ::std::runtime_error(what)
{

}

GPIOD_CXX_API bad_mapping::bad_mapping(const bad_mapping& other) noexcept
	: ::std::runtime_error(other)
{

}

GPIOD_CXX_API bad_mapping::bad_mapping(bad_mapping&& other) noexcept
	: ::std::runtime_error(other)
{

}

GPIOD_CXX_API bad_mapping& bad_mapping::operator=(const bad_mapping& other) noexcept
{
	::std::runtime_error::operator=(other);

	return *this;
}

GPIOD_CXX_API bad_mapping& bad_mapping::operator=(bad_mapping&& other) noexcept
{
	::std::runtime_error::operator=(other);

	return *this;
}

GPIOD_CXX_API bad_mapping::~bad_mapping()
{

}

} /* namespace gpiod */
