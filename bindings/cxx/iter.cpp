// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <gpiod.hpp>
#include <system_error>

namespace gpiod {

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
