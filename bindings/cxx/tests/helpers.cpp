// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "helpers.hpp"

system_error_matcher::system_error_matcher(int expected_errno)
	: _m_cond(::std::system_category().default_error_condition(expected_errno))
{

}

::std::string system_error_matcher::describe() const
{
	return "matches: errno " + ::std::to_string(this->_m_cond.value());
}

bool system_error_matcher::match(const ::std::system_error& error) const
{
	return error.code().value() == this->_m_cond.value();
}

regex_matcher::regex_matcher(const ::std::string& pattern)
	: _m_pattern(pattern),
	  _m_repr("matches: regex \"" + pattern + "\"")
{

}

::std::string regex_matcher::describe() const
{
	return this->_m_repr;
}

bool regex_matcher::match(const ::std::string& str) const
{
	return ::std::regex_match(str, this->_m_pattern);
}
