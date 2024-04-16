// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

void chip_info::impl::set_info_ptr(chip_info_ptr& new_info)
{
	this->info = ::std::move(new_info);
}

GPIOD_CXX_API chip_info::chip_info()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API chip_info::chip_info(const chip_info& other)
	: _m_priv(other._m_priv)
{

}

GPIOD_CXX_API chip_info::chip_info(chip_info&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API chip_info::~chip_info()
{

}

GPIOD_CXX_API chip_info& chip_info::operator=(const chip_info& other)
{
	this->_m_priv = other._m_priv;

	return *this;
}

GPIOD_CXX_API chip_info& chip_info::operator=(chip_info&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API ::std::string chip_info::name() const noexcept
{
	return ::gpiod_chip_info_get_name(this->_m_priv->info.get());
}

GPIOD_CXX_API ::std::string chip_info::label() const noexcept
{
	return ::gpiod_chip_info_get_label(this->_m_priv->info.get());
}

GPIOD_CXX_API ::std::size_t chip_info::num_lines() const noexcept
{
	return ::gpiod_chip_info_get_num_lines(this->_m_priv->info.get());
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const chip_info& info)
{
	out << "gpiod::chip_info(name=\"" << info.name() <<
	       "\", label=\"" << info.label() <<
	       "\", num_lines=" << info.num_lines() << ")";

	return out;
}

} /* namespace gpiod */
