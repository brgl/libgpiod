// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <map>
#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

namespace {

const ::std::map<int, line::direction> direction_mapping = {
	{ GPIOD_LINE_DIRECTION_INPUT,		line::direction::INPUT },
	{ GPIOD_LINE_DIRECTION_OUTPUT,		line::direction::OUTPUT }
};

const ::std::map<int, line::bias> bias_mapping = {
	{ GPIOD_LINE_BIAS_UNKNOWN,		line::bias::UNKNOWN },
	{ GPIOD_LINE_BIAS_DISABLED,		line::bias::DISABLED },
	{ GPIOD_LINE_BIAS_PULL_UP,		line::bias::PULL_UP },
	{ GPIOD_LINE_BIAS_PULL_DOWN,		line::bias::PULL_DOWN }
};

const ::std::map<int, line::drive> drive_mapping = {
	{ GPIOD_LINE_DRIVE_PUSH_PULL,		line::drive::PUSH_PULL },
	{ GPIOD_LINE_DRIVE_OPEN_DRAIN,		line::drive::OPEN_DRAIN },
	{ GPIOD_LINE_DRIVE_OPEN_SOURCE,		line::drive::OPEN_SOURCE }
};

const ::std::map<int, line::edge> edge_mapping = {
	{ GPIOD_LINE_EDGE_NONE,			line::edge::NONE },
	{ GPIOD_LINE_EDGE_RISING,		line::edge::RISING },
	{ GPIOD_LINE_EDGE_FALLING,		line::edge::FALLING },
	{ GPIOD_LINE_EDGE_BOTH,			line::edge::BOTH }
};

const ::std::map<int, line::clock> clock_mapping = {
	{ GPIOD_LINE_EVENT_CLOCK_MONOTONIC,	line::clock::MONOTONIC },
	{ GPIOD_LINE_EVENT_CLOCK_REALTIME,	line::clock::REALTIME },
	{ GPIOD_LINE_EVENT_CLOCK_HTE,		line::clock::HTE }
};

} /* namespace */

void line_info::impl::set_info_ptr(line_info_ptr& new_info)
{
	this->info = ::std::move(new_info);
}

line_info::line_info()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API line_info::line_info(const line_info& other) noexcept
	: _m_priv(other._m_priv)
{

}

GPIOD_CXX_API line_info::line_info(line_info&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API line_info::~line_info()
{

}

GPIOD_CXX_API line_info& line_info::operator=(const line_info& other) noexcept
{
	this->_m_priv = other._m_priv;

	return *this;
}

GPIOD_CXX_API line_info& line_info::operator=(line_info&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API line::offset line_info::offset() const noexcept
{
	return ::gpiod_line_info_get_offset(this->_m_priv->info.get());
}

GPIOD_CXX_API ::std::string line_info::name() const noexcept
{
	const char* name = ::gpiod_line_info_get_name(this->_m_priv->info.get());

	return name ?: "";
}

GPIOD_CXX_API bool line_info::used() const noexcept
{
	return ::gpiod_line_info_is_used(this->_m_priv->info.get());
}

GPIOD_CXX_API ::std::string line_info::consumer() const noexcept
{
	const char* consumer = ::gpiod_line_info_get_consumer(this->_m_priv->info.get());

	return consumer ?: "";
}

GPIOD_CXX_API line::direction line_info::direction() const
{
	int direction = ::gpiod_line_info_get_direction(this->_m_priv->info.get());

	return map_enum_c_to_cxx(direction, direction_mapping);
}

GPIOD_CXX_API bool line_info::active_low() const noexcept
{
	return ::gpiod_line_info_is_active_low(this->_m_priv->info.get());
}

GPIOD_CXX_API line::bias line_info::bias() const
{
	int bias = ::gpiod_line_info_get_bias(this->_m_priv->info.get());

	return bias_mapping.at(bias);
}

GPIOD_CXX_API line::drive line_info::drive() const
{
	int drive = ::gpiod_line_info_get_drive(this->_m_priv->info.get());

	return drive_mapping.at(drive);
}

GPIOD_CXX_API line::edge line_info::edge_detection() const
{
	int edge = ::gpiod_line_info_get_edge_detection(this->_m_priv->info.get());

	return edge_mapping.at(edge);
}

GPIOD_CXX_API line::clock line_info::event_clock() const
{
	int clock = ::gpiod_line_info_get_event_clock(this->_m_priv->info.get());

	return clock_mapping.at(clock);
}

GPIOD_CXX_API bool line_info::debounced() const  noexcept
{
	return ::gpiod_line_info_is_debounced(this->_m_priv->info.get());
}

GPIOD_CXX_API ::std::chrono::microseconds line_info::debounce_period() const  noexcept
{
	return ::std::chrono::microseconds(
			::gpiod_line_info_get_debounce_period_us(this->_m_priv->info.get()));
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const line_info& info)
{
	::std::string name, consumer;

	name = info.name().empty() ? "unnamed" : ::std::string("'") + info.name() + "'";
	consumer = info.consumer().empty() ? "unused" : ::std::string("'") + info.name() + "'";

	out << "gpiod::line_info(offset=" << info.offset() <<
	       ", name=" << name <<
	       ", used=" << ::std::boolalpha << info.used() <<
	       ", consumer=" << consumer <<
	       ", direction=" << info.direction() <<
	       ", active_low=" << ::std::boolalpha << info.active_low() <<
	       ", bias=" << info.bias() <<
	       ", drive=" << info.drive() <<
	       ", edge_detection=" << info.edge_detection() <<
	       ", event_clock=" << info.event_clock() <<
	       ", debounced=" << ::std::boolalpha << info.debounced();

	if (info.debounced())
		out << ", debounce_period=" << info.debounce_period().count() << "us";

	out << ")";

	return out;
}

} /* namespace gpiod */
