// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <iterator>
#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

namespace {

edge_event_buffer_ptr make_edge_event_buffer(unsigned int capacity)
{
	edge_event_buffer_ptr buffer(::gpiod_edge_event_buffer_new(capacity));
	if (!buffer)
		throw_from_errno("unable to allocate the edge event buffer");

	return buffer;
}

} /* namespace */

edge_event_buffer::impl::impl(unsigned int capacity)
	: buffer(make_edge_event_buffer(capacity)),
	  events()
{
	events.reserve(capacity);

	for (unsigned int i = 0; i < capacity; i++) {
		events.push_back(edge_event());
		events.back()._m_priv.reset(new edge_event::impl_external);
	}
}

int edge_event_buffer::impl::read_events(const line_request_ptr& request, unsigned int max_events)
{
	int ret = ::gpiod_line_request_read_edge_events(request.get(),
						       this->buffer.get(), max_events);
	if (ret < 0)
		throw_from_errno("error reading edge events from file descriptor");

	for (int i = 0; i < ret; i++) {
		::gpiod_edge_event* event = ::gpiod_edge_event_buffer_get_event(this->buffer.get(), i);

		dynamic_cast<edge_event::impl_external&>(*this->events[i]._m_priv).event = event;
	}

	return ret;
}

GPIOD_CXX_API edge_event_buffer::edge_event_buffer(::std::size_t capacity)
	: _m_priv(new impl(capacity))
{

}

GPIOD_CXX_API edge_event_buffer::edge_event_buffer(edge_event_buffer&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API edge_event_buffer::~edge_event_buffer()
{

}

GPIOD_CXX_API edge_event_buffer& edge_event_buffer::operator=(edge_event_buffer&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API const edge_event& edge_event_buffer::get_event(unsigned int index) const
{
	return this->_m_priv->events.at(index);
}

GPIOD_CXX_API ::std::size_t edge_event_buffer::num_events() const
{
	return ::gpiod_edge_event_buffer_get_num_events(this->_m_priv->buffer.get());
}

GPIOD_CXX_API ::std::size_t edge_event_buffer::capacity() const noexcept
{
	return ::gpiod_edge_event_buffer_get_capacity(this->_m_priv->buffer.get());
}

GPIOD_CXX_API edge_event_buffer::const_iterator edge_event_buffer::begin() const noexcept
{
	return this->_m_priv->events.begin();
}

GPIOD_CXX_API edge_event_buffer::const_iterator edge_event_buffer::end() const noexcept
{
	return this->_m_priv->events.begin() + this->num_events();
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const edge_event_buffer& buf)
{
	out << "gpiod::edge_event_buffer(num_events=" << buf.num_events() <<
	       ", capacity=" << buf.capacity() <<
	       ", events=[";

	::std::copy(buf.begin(), ::std::prev(buf.end()),
		    ::std::ostream_iterator<edge_event>(out, ", "));
	out << *(::std::prev(buf.end()));

	out << "])";

	return out;
}

} /* namespace gpiod */
