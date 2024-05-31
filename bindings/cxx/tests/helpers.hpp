/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __GPIOD_CXX_TEST_HELPERS_HPP__
#define __GPIOD_CXX_TEST_HELPERS_HPP__

#include <catch2/catch_all.hpp>
#include <regex>
#include <string>
#include <sstream>
#include <system_error>

class system_error_matcher : public Catch::Matchers::MatcherBase<::std::system_error>
{
public:
	explicit system_error_matcher(int expected_errno);
	::std::string describe() const override;
	bool match(const ::std::system_error& error) const override;

private:
	::std::error_condition _m_cond;
};

class regex_matcher : public Catch::Matchers::MatcherBase<::std::string>
{
public:
	explicit regex_matcher(const ::std::string& pattern);
	::std::string describe() const override;
	bool match(const ::std::string& str) const override;

private:
	::std::regex _m_pattern;
	::std::string _m_repr;
};

template<class T> class stringify_matcher : public Catch::Matchers::MatcherBase<T>
{
public:
	explicit stringify_matcher(const ::std::string& expected) : _m_expected(expected)
	{

	}

	::std::string describe() const override
	{
		return "equals " + this->_m_expected;
	}

	bool match(const T& obj) const override
	{
		::std::stringstream buf;

		buf << obj;

		return buf.str() == this->_m_expected;
	}

private:
	::std::string _m_expected;
};

#endif /* __GPIOD_CXX_TEST_HELPERS_HPP__ */
