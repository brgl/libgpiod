// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <functional>
#include <gpiod.hpp>
#include <map>
#include <system_error>
#include <utility>

namespace gpiod {

namespace {

void chip_deleter(::gpiod_chip* chip)
{
	::gpiod_chip_close(chip);
}

} /* namespace */

bool is_gpiochip_device(const ::std::string& path)
{
	return ::gpiod_is_gpiochip_device(path.c_str());
}

chip::chip(const ::std::string& path)
	: _m_chip()
{
	this->open(path);
}

chip::chip(::gpiod_chip* chip)
	: _m_chip(chip, chip_deleter)
{

}

chip::chip(const ::std::weak_ptr<::gpiod_chip>& chip_ptr)
	: _m_chip(chip_ptr)
{

}

void chip::open(const ::std::string& path)
{
	::gpiod_chip *chip = ::gpiod_chip_open(path.c_str());
	if (!chip)
		throw ::std::system_error(errno, ::std::system_category(),
					  "cannot open GPIO device " + path);

	this->_m_chip.reset(chip, chip_deleter);
}

void chip::reset(void) noexcept
{
	this->_m_chip.reset();
}

::std::string chip::name(void) const
{
	this->throw_if_noref();

	return ::std::string(::gpiod_chip_name(this->_m_chip.get()));
}

::std::string chip::label(void) const
{
	this->throw_if_noref();

	return ::std::string(::gpiod_chip_label(this->_m_chip.get()));
}

unsigned int chip::num_lines(void) const
{
	this->throw_if_noref();

	return ::gpiod_chip_num_lines(this->_m_chip.get());
}

line chip::get_line(unsigned int offset) const
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

::std::vector<line> chip::find_line(const ::std::string& name, bool unique) const
{
	this->throw_if_noref();

	::std::vector<line> lines;

	for (auto& line: ::gpiod::line_iter(*this)) {
		if (line.name() == name)
			lines.push_back(line);
	}

	if (unique && lines.size() > 1)
		throw ::std::system_error(ERANGE, ::std::system_category(),
					  "multiple lines with the same name found");

	return lines;
}

line_bulk chip::get_lines(const ::std::vector<unsigned int>& offsets) const
{
	line_bulk lines;

	for (auto& it: offsets)
		lines.append(this->get_line(it));

	return lines;
}

line_bulk chip::get_all_lines(void) const
{
	line_bulk lines;

	for (unsigned int i = 0; i < this->num_lines(); i++)
		lines.append(this->get_line(i));

	return lines;
}

bool chip::operator==(const chip& rhs) const noexcept
{
	return this->_m_chip.get() == rhs._m_chip.get();
}

bool chip::operator!=(const chip& rhs) const noexcept
{
	return this->_m_chip.get() != rhs._m_chip.get();
}

chip::operator bool(void) const noexcept
{
	return this->_m_chip.get() != nullptr;
}

bool chip::operator!(void) const noexcept
{
	return this->_m_chip.get() == nullptr;
}

void chip::throw_if_noref(void) const
{
	if (!this->_m_chip.get())
		throw ::std::logic_error("object not associated with an open GPIO chip");
}

} /* namespace gpiod */
