// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <gpiod.hpp>
#include <array>
#include <map>
#include <system_error>

#include "internal.hpp"

namespace gpiod {

namespace {

const ::std::map<int, int> drive_mapping = {
	{ GPIOD_LINE_DRIVE_PUSH_PULL,	line::DRIVE_PUSH_PULL, },
	{ GPIOD_LINE_DRIVE_OPEN_DRAIN,	line::DRIVE_OPEN_DRAIN, },
	{ GPIOD_LINE_DRIVE_OPEN_SOURCE,	line::DRIVE_OPEN_SOURCE, },
};

const ::std::map<int, int> bias_mapping = {
	{ GPIOD_LINE_BIAS_UNKNOWN,	line::BIAS_UNKNOWN, },
	{ GPIOD_LINE_BIAS_DISABLED,	line::BIAS_DISABLED, },
	{ GPIOD_LINE_BIAS_PULL_UP,	line::BIAS_PULL_UP, },
	{ GPIOD_LINE_BIAS_PULL_DOWN,	line::BIAS_PULL_DOWN, },
};

} /* namespace */

GPIOD_CXX_API line::line(void)
	: _m_line(nullptr),
	  _m_owner()
{

}

GPIOD_CXX_API line::line(::gpiod_line* line, const chip& owner)
	: _m_line(line),
	  _m_owner(owner._m_chip)
{

}

GPIOD_CXX_API unsigned int line::offset(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_offset(this->_m_line);
}

GPIOD_CXX_API ::std::string line::name(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	const char* name = ::gpiod_line_name(this->_m_line);

	return name ? ::std::string(name) : ::std::string();
}

GPIOD_CXX_API ::std::string line::consumer(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	const char* consumer = ::gpiod_line_consumer(this->_m_line);

	return consumer ? ::std::string(consumer) : ::std::string();
}

GPIOD_CXX_API int line::direction(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	int dir = ::gpiod_line_direction(this->_m_line);

	return dir == GPIOD_LINE_DIRECTION_INPUT ? DIRECTION_INPUT : DIRECTION_OUTPUT;
}

GPIOD_CXX_API bool line::is_active_low(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_active_low(this->_m_line);
}

GPIOD_CXX_API int line::bias(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return bias_mapping.at(::gpiod_line_bias(this->_m_line));
}

GPIOD_CXX_API bool line::is_used(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_used(this->_m_line);
}

GPIOD_CXX_API int line::drive(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return drive_mapping.at(::gpiod_line_drive(this->_m_line));
}

GPIOD_CXX_API void line::request(const line_request& config, int default_val) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.request(config, { default_val });
}

GPIOD_CXX_API void line::release(void) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.release();
}

/*
 * REVISIT: Check the performance of get/set_value & event_wait compared to
 * the C API. Creating a line_bulk object involves a memory allocation every
 * time this method if called. If the performance is significantly lower,
 * switch to calling the C functions for setting/getting line values and
 * polling for events on single lines directly.
 */

GPIOD_CXX_API int line::get_value(void) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	return bulk.get_values()[0];
}

GPIOD_CXX_API void line::set_value(int val) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_values({ val });
}

GPIOD_CXX_API void line::set_config(int direction, ::std::bitset<32> flags,
				    int value) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_config(direction, flags, { value });
}

GPIOD_CXX_API void line::set_flags(::std::bitset<32> flags) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_flags(flags);
}

GPIOD_CXX_API void line::set_direction_input() const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_direction_input();
}

GPIOD_CXX_API void line::set_direction_output(int value) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_direction_output({ value });
}

GPIOD_CXX_API bool line::event_wait(const ::std::chrono::nanoseconds& timeout) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	line_bulk event_bulk = bulk.event_wait(timeout);

	return !!event_bulk;
}

GPIOD_CXX_API line_event line::make_line_event(const ::gpiod_line_event& event) const noexcept
{
	line_event ret;

	if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
		ret.event_type = line_event::RISING_EDGE;
	else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
		ret.event_type = line_event::FALLING_EDGE;

	ret.timestamp = ::std::chrono::duration_cast<::std::chrono::nanoseconds>(
				::std::chrono::seconds(event.ts.tv_sec)) +
				::std::chrono::nanoseconds(event.ts.tv_nsec);

	ret.source = *this;

	return ret;
}

GPIOD_CXX_API line_event line::event_read(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	::gpiod_line_event event_buf;
	line_event event;
	int rv;

	rv = ::gpiod_line_event_read(this->_m_line, ::std::addressof(event_buf));
	if (rv < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading line event");

	return this->make_line_event(event_buf);
}

GPIOD_CXX_API ::std::vector<line_event> line::event_read_multiple(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	/* 16 is the maximum number of events stored in the kernel FIFO. */
	::std::array<::gpiod_line_event, 16> event_buf;
	::std::vector<line_event> events;
	int rv;

	rv = ::gpiod_line_event_read_multiple(this->_m_line,
					      event_buf.data(), event_buf.size());
	if (rv < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading multiple line events");

	events.reserve(rv);
	for (int i = 0; i < rv; i++)
		events.push_back(this->make_line_event(event_buf[i]));

	return events;
}

GPIOD_CXX_API int line::event_get_fd(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	int ret = ::gpiod_line_event_get_fd(this->_m_line);

	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to get the line event file descriptor");

	return ret;
}

GPIOD_CXX_API const chip line::get_chip(void) const
{
	return chip(this->_m_owner);
}

GPIOD_CXX_API void line::reset(void)
{
	this->_m_line = nullptr;
	this->_m_owner.reset();
}

GPIOD_CXX_API bool line::operator==(const line& rhs) const noexcept
{
	return this->_m_line == rhs._m_line;
}

GPIOD_CXX_API bool line::operator!=(const line& rhs) const noexcept
{
	return this->_m_line != rhs._m_line;
}

GPIOD_CXX_API line::operator bool(void) const noexcept
{
	return this->_m_line != nullptr;
}

GPIOD_CXX_API bool line::operator!(void) const noexcept
{
	return this->_m_line == nullptr;
}

GPIOD_CXX_API void line::throw_if_null(void) const
{
	if (!this->_m_line)
		throw ::std::logic_error("object not holding a GPIO line handle");
}

GPIOD_CXX_API line::chip_guard::chip_guard(const line& line)
	: _m_chip(line._m_owner)
{

}

} /* namespace gpiod */
