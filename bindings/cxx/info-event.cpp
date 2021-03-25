// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <map>
#include <ostream>

#include "internal.hpp"

namespace gpiod {

namespace {

const ::std::map<int, info_event::event_type> event_type_mapping = {
	{ GPIOD_INFO_EVENT_LINE_REQUESTED,	info_event::event_type::LINE_REQUESTED },
	{ GPIOD_INFO_EVENT_LINE_RELEASED,	info_event::event_type::LINE_RELEASED },
	{ GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED,	info_event::event_type::LINE_CONFIG_CHANGED }
};

const ::std::map<info_event::event_type, ::std::string> event_type_names = {
	{ info_event::event_type::LINE_REQUESTED,	"LINE_REQUESTED" },
	{ info_event::event_type::LINE_RELEASED,	"LINE_RELEASED" },
	{ info_event::event_type::LINE_CONFIG_CHANGED,	"LINE_CONFIG_CHANGED" }
};

} /* namespace */

void info_event::impl::set_info_event_ptr(info_event_ptr& new_event)
{
	::gpiod_line_info* info = ::gpiod_info_event_get_line_info(new_event.get());

	line_info_ptr copy(::gpiod_line_info_copy(info));
	if (!copy)
		throw_from_errno("unable to copy the line info object");

	this->event = ::std::move(new_event);
	this->info._m_priv->set_info_ptr(copy);
}

info_event::info_event()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API info_event::info_event(const info_event& other)
	: _m_priv(other._m_priv)
{

}

GPIOD_CXX_API info_event::info_event(info_event&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API info_event::~info_event()
{

}

GPIOD_CXX_API info_event& info_event::operator=(const info_event& other)
{
	this->_m_priv = other._m_priv;

	return *this;
}

GPIOD_CXX_API info_event& info_event::operator=(info_event&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API info_event::event_type info_event::type() const
{
	int type = ::gpiod_info_event_get_event_type(this->_m_priv->event.get());

	return map_int_to_enum(type, event_type_mapping);
}

GPIOD_CXX_API ::std::uint64_t info_event::timestamp_ns() const noexcept
{
	return ::gpiod_info_event_get_timestamp_ns(this->_m_priv->event.get());
}

GPIOD_CXX_API const line_info& info_event::get_line_info() const noexcept
{
	return this->_m_priv->info;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const info_event& event)
{
	out << "gpiod::info_event(event_type='" << event_type_names.at(event.type()) <<
	       "', timestamp=" << event.timestamp_ns() <<
	       ", line_info=" << event.get_line_info() <<
	       ")";

	return out;
}

} /* namespace gpiod */
