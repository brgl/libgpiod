// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <functional>
#include <gpiod.hpp>
#include <map>
#include <system_error>
#include <utility>

#include "internal.hpp"

namespace gpiod {

namespace {

GPIOD_CXX_API void chip_deleter(::gpiod_chip* chip)
{
	::gpiod_chip_unref(chip);
}

} /* namespace */

GPIOD_CXX_API bool is_gpiochip_device(const ::std::string& path)
{
	return ::gpiod_is_gpiochip_device(path.c_str());
}

GPIOD_CXX_API chip::chip(const ::std::string& path)
	: _m_chip()
{
	this->open(path);
}

GPIOD_CXX_API chip::chip(::gpiod_chip* chip)
	: _m_chip(chip, chip_deleter)
{

}

GPIOD_CXX_API chip::chip(const ::std::weak_ptr<::gpiod_chip>& chip_ptr)
	: _m_chip(chip_ptr)
{

}

GPIOD_CXX_API void chip::open(const ::std::string& path)
{
	::gpiod_chip *chip = ::gpiod_chip_open(path.c_str());
	if (!chip)
		throw ::std::system_error(errno, ::std::system_category(),
					  "cannot open GPIO device " + path);

	this->_m_chip.reset(chip, chip_deleter);
}

GPIOD_CXX_API void chip::reset(void) noexcept
{
	this->_m_chip.reset();
}

GPIOD_CXX_API ::std::string chip::name(void) const
{
	this->throw_if_noref();

	return ::std::string(::gpiod_chip_name(this->_m_chip.get()));
}

GPIOD_CXX_API ::std::string chip::label(void) const
{
	this->throw_if_noref();

	return ::std::string(::gpiod_chip_label(this->_m_chip.get()));
}

GPIOD_CXX_API unsigned int chip::num_lines(void) const
{
	this->throw_if_noref();

	return ::gpiod_chip_num_lines(this->_m_chip.get());
}

GPIOD_CXX_API line chip::get_line(unsigned int offset) const
{
	this->throw_if_noref();

	if (offset >= this->num_lines())
		throw ::std::out_of_range("line offset greater than the number of lines");

	::gpiod_line* line_handle = ::gpiod_chip_get_line(this->_m_chip.get(), offset);
	if (!line_handle)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error getting GPIO line from chip");

	return line(line_handle, *this);
}

GPIOD_CXX_API int chip::find_line(const ::std::string& name) const
{
	this->throw_if_noref();

	for (unsigned int offset = 0; offset < this->num_lines(); offset++) {
		auto line = this->get_line(offset);

		if (line.name() == name)
			return offset;
	}

	return -1;
}

GPIOD_CXX_API line_bulk chip::get_lines(const ::std::vector<unsigned int>& offsets) const
{
	line_bulk lines;

	for (auto& it: offsets)
		lines.append(this->get_line(it));

	return lines;
}

GPIOD_CXX_API line_bulk chip::get_all_lines(void) const
{
	line_bulk lines;

	for (unsigned int i = 0; i < this->num_lines(); i++)
		lines.append(this->get_line(i));

	return lines;
}

GPIOD_CXX_API bool chip::operator==(const chip& rhs) const noexcept
{
	return this->_m_chip.get() == rhs._m_chip.get();
}

GPIOD_CXX_API bool chip::operator!=(const chip& rhs) const noexcept
{
	return this->_m_chip.get() != rhs._m_chip.get();
}

GPIOD_CXX_API chip::operator bool(void) const noexcept
{
	return this->_m_chip.get() != nullptr;
}

GPIOD_CXX_API bool chip::operator!(void) const noexcept
{
	return this->_m_chip.get() == nullptr;
}

GPIOD_CXX_API void chip::throw_if_noref(void) const
{
	if (!this->_m_chip.get())
		throw ::std::logic_error("object not associated with an open GPIO chip");
}

} /* namespace gpiod */
