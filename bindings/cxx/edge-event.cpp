// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <map>
#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

namespace {

const ::std::map<int, edge_event::event_type> event_type_mapping = {
	{ GPIOD_EDGE_EVENT_RISING_EDGE,		edge_event::event_type::RISING_EDGE },
	{ GPIOD_EDGE_EVENT_FALLING_EDGE,	edge_event::event_type::FALLING_EDGE }
};

const ::std::map<edge_event::event_type, ::std::string> event_type_names = {
	{ edge_event::event_type::RISING_EDGE,		"RISING_EDGE" },
	{ edge_event::event_type::FALLING_EDGE,		"FALLING_EDGE" }
};

} /* namespace */

::gpiod_edge_event* edge_event::impl_managed::get_event_ptr() const noexcept
{
	return this->event.get();
}

::std::shared_ptr<edge_event::impl>
edge_event::impl_managed::copy(const ::std::shared_ptr<impl>& self) const
{
	return self;
}

edge_event::impl_external::impl_external()
	: impl(),
	  event(nullptr)
{

}

::gpiod_edge_event* edge_event::impl_external::get_event_ptr() const noexcept
{
	return this->event;
}

::std::shared_ptr<edge_event::impl>
edge_event::impl_external::copy(const ::std::shared_ptr<impl>& self GPIOD_CXX_UNUSED) const
{
	::std::shared_ptr<impl> ret(new impl_managed);
	impl_managed& managed = dynamic_cast<impl_managed&>(*ret);

	managed.event.reset(::gpiod_edge_event_copy(this->event));
	if (!managed.event)
		throw_from_errno("unable to copy the edge event object");

	return ret;
}

edge_event::edge_event()
	: _m_priv()
{

}

GPIOD_CXX_API edge_event::edge_event(const edge_event& other)
	: _m_priv(other._m_priv->copy(other._m_priv))
{

}

GPIOD_CXX_API edge_event::edge_event(edge_event&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API edge_event::~edge_event()
{

}

GPIOD_CXX_API edge_event& edge_event::operator=(const edge_event& other)
{
	this->_m_priv = other._m_priv->copy(other._m_priv);

	return *this;
}

GPIOD_CXX_API edge_event& edge_event::operator=(edge_event&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API edge_event::event_type edge_event::type() const
{
	int evtype = ::gpiod_edge_event_get_event_type(this->_m_priv->get_event_ptr());

	return map_int_to_enum(evtype, event_type_mapping);
}

GPIOD_CXX_API timestamp edge_event::timestamp_ns() const noexcept
{
	return ::gpiod_edge_event_get_timestamp_ns(this->_m_priv->get_event_ptr());
}

GPIOD_CXX_API line::offset edge_event::line_offset() const noexcept
{
	return ::gpiod_edge_event_get_line_offset(this->_m_priv->get_event_ptr());
}

GPIOD_CXX_API unsigned long edge_event::global_seqno() const noexcept
{
	return ::gpiod_edge_event_get_global_seqno(this->_m_priv->get_event_ptr());
}

GPIOD_CXX_API unsigned long edge_event::line_seqno() const noexcept
{
	return ::gpiod_edge_event_get_line_seqno(this->_m_priv->get_event_ptr());
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const edge_event& event)
{
	out << "gpiod::edge_event(type='" << event_type_names.at(event.type()) <<
	       "', timestamp=" << event.timestamp_ns() <<
	       ", line_offset=" << event.line_offset() <<
	       ", global_seqno=" << event.global_seqno() <<
	       ", line_seqno=" << event.line_seqno() << ")";

	return out;
}

} /* namespace gpiod */
