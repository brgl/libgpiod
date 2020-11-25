// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <gpiod.hpp>
#include <system_error>

namespace gpiod {

namespace {

void chip_iter_deleter(::gpiod_chip_iter* iter)
{
	::gpiod_chip_iter_free_noclose(iter);
}

} /* namespace */

chip_iter make_chip_iter(void)
{
	::gpiod_chip_iter* iter = ::gpiod_chip_iter_new();
	if (!iter)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error creating GPIO chip iterator");

	return chip_iter(iter);
}

bool chip_iter::operator==(const chip_iter& rhs) const noexcept
{
	return this->_m_current == rhs._m_current;
}

bool chip_iter::operator!=(const chip_iter& rhs) const noexcept
{
	return this->_m_current != rhs._m_current;
}

chip_iter::chip_iter(::gpiod_chip_iter *iter)
	: _m_iter(iter, chip_iter_deleter)
{
	::gpiod_chip* first = ::gpiod_chip_iter_next_noclose(this->_m_iter.get());

	if (first != nullptr)
		this->_m_current = chip(first);
}

chip_iter& chip_iter::operator++(void)
{
	::gpiod_chip* next = ::gpiod_chip_iter_next_noclose(this->_m_iter.get());

	this->_m_current = next ? chip(next) : chip();

	return *this;
}

const chip& chip_iter::operator*(void) const
{
	return this->_m_current;
}

const chip* chip_iter::operator->(void) const
{
	return ::std::addressof(this->_m_current);
}

chip_iter begin(chip_iter iter) noexcept
{
	return iter;
}

chip_iter end(const chip_iter&) noexcept
{
	return chip_iter();
}

line_iter begin(line_iter iter) noexcept
{
	return iter;
}

line_iter end(const line_iter&) noexcept
{
	return line_iter();
}

line_iter::line_iter(const chip& owner)
	: _m_current(owner.get_line(0))
{

}

line_iter& line_iter::operator++(void)
{
	unsigned int offset = this->_m_current.offset() + 1;
	chip owner = this->_m_current.get_chip();

	if (offset == owner.num_lines())
		this->_m_current = line(); /* Last element */
	else
		this->_m_current = owner.get_line(offset);

	return *this;
}

const line& line_iter::operator*(void) const
{
	return this->_m_current;
}

const line* line_iter::operator->(void) const
{
	return ::std::addressof(this->_m_current);
}

bool line_iter::operator==(const line_iter& rhs) const noexcept
{
	return this->_m_current._m_line == rhs._m_current._m_line;
}

bool line_iter::operator!=(const line_iter& rhs) const noexcept
{
	return this->_m_current._m_line != rhs._m_current._m_line;
}

} /* namespace gpiod */
