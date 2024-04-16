// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <iterator>
#include <ostream>

#include "internal.hpp"

namespace gpiod {
namespace line {

namespace {

const ::std::map<line::value, ::std::string> value_names = {
	{ line::value::INACTIVE,	"INACTIVE" },
	{ line::value::ACTIVE,		"ACTIVE" },
};

const ::std::map<line::direction, ::std::string> direction_names = {
	{ line::direction::AS_IS,	"AS_IS" },
	{ line::direction::INPUT,	"INPUT" },
	{ line::direction::OUTPUT,	"OUTPUT" },
};

const ::std::map<line::bias, ::std::string> bias_names = {
	{ line::bias::AS_IS,		"AS_IS" },
	{ line::bias::UNKNOWN,		"UNKNOWN" },
	{ line::bias::DISABLED,		"DISABLED" },
	{ line::bias::PULL_UP,		"PULL_UP" },
	{ line::bias::PULL_DOWN,	"PULL_DOWN" },
};

const ::std::map<line::drive, ::std::string> drive_names = {
	{ line::drive::PUSH_PULL,	"PUSH_PULL" },
	{ line::drive::OPEN_DRAIN,	"OPEN_DRAIN" },
	{ line::drive::OPEN_SOURCE,	"OPEN_SOURCE" },
};

const ::std::map<line::edge, ::std::string> edge_names = {
	{ line::edge::NONE,		"NONE" },
	{ line::edge::RISING,		"RISING_EDGE" },
	{ line::edge::FALLING,		"FALLING_EDGE" },
	{ line::edge::BOTH,		"BOTH_EDGES" },
};

const ::std::map<line::clock, ::std::string> clock_names = {
	{ line::clock::MONOTONIC,	"MONOTONIC" },
	{ line::clock::REALTIME,	"REALTIME" },
	{ line::clock::HTE,		"HTE" },
};

} /* namespace */

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, line::value val)
{
	out << value_names.at(val);

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, line::direction dir)
{
	out << direction_names.at(dir);

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, line::edge edge)
{
	out << edge_names.at(edge);

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, line::bias bias)
{
	out << bias_names.at(bias);

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, line::drive drive)
{
	out << drive_names.at(drive);

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, line::clock clock)
{
	out << clock_names.at(clock);

	return out;
}

template<typename T>
::std::ostream& insert_vector(::std::ostream& out,
			      const ::std::string& name, const ::std::vector<T>& vec)
{
	out << name << "(";
	::std::copy(vec.begin(), ::std::prev(vec.end()),
		    ::std::ostream_iterator<T>(out, ", "));
	out << vec.back();
	out << ")";

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const offsets& offs)
{
	return insert_vector(out, "gpiod::offsets", offs);
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const line::values& vals)
{
	return insert_vector(out, "gpiod::values", vals);
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const line::value_mapping& mapping)
{
	out << "gpiod::value_mapping(" << mapping.first << ": " << mapping.second << ")";

	return out;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const line::value_mappings& mappings)
{
	return insert_vector(out, "gpiod::value_mappings", mappings);
}

} /* namespace line */
} /* namespace gpiod */
