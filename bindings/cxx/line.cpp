// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <gpiod.hpp>
#include <map>
#include <system_error>

namespace gpiod {

namespace {

const ::std::map<int, int> bias_mapping = {
	{ GPIOD_LINE_BIAS_PULL_UP,	line::BIAS_PULL_UP, },
	{ GPIOD_LINE_BIAS_PULL_DOWN,	line::BIAS_PULL_DOWN, },
	{ GPIOD_LINE_BIAS_DISABLE,	line::BIAS_DISABLE, },
	{ GPIOD_LINE_BIAS_AS_IS,	line::BIAS_AS_IS, },
};

} /* namespace */

line::line(void)
	: _m_line(nullptr),
	  _m_owner()
{

}

line::line(::gpiod_line* line, const chip& owner)
	: _m_line(line),
	  _m_owner(owner._m_chip)
{

}

unsigned int line::offset(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_offset(this->_m_line);
}

::std::string line::name(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	const char* name = ::gpiod_line_name(this->_m_line);

	return name ? ::std::string(name) : ::std::string();
}

::std::string line::consumer(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	const char* consumer = ::gpiod_line_consumer(this->_m_line);

	return consumer ? ::std::string(consumer) : ::std::string();
}

int line::direction(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	int dir = ::gpiod_line_direction(this->_m_line);

	return dir == GPIOD_LINE_DIRECTION_INPUT ? DIRECTION_INPUT : DIRECTION_OUTPUT;
}

bool line::is_active_low(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_active_low(this->_m_line);
}

int line::bias(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return bias_mapping.at(::gpiod_line_bias(this->_m_line));
}

bool line::is_used(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_used(this->_m_line);
}

bool line::is_open_drain(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_open_drain(this->_m_line);
}

bool line::is_open_source(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_open_source(this->_m_line);
}

void line::request(const line_request& config, int default_val) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.request(config, { default_val });
}

void line::release(void) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.release();
}

bool line::is_requested(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	return ::gpiod_line_is_requested(this->_m_line);
}

/*
 * REVISIT: Check the performance of get/set_value & event_wait compared to
 * the C API. Creating a line_bulk object involves a memory allocation every
 * time this method if called. If the performance is significantly lower,
 * switch to calling the C functions for setting/getting line values and
 * polling for events on single lines directly.
 */

int line::get_value(void) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	return bulk.get_values()[0];
}

void line::set_value(int val) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_values({ val });
}

void line::set_config(int direction, ::std::bitset<32> flags,
			int value) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_config(direction, flags, { value });
}

void line::set_flags(::std::bitset<32> flags) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_flags(flags);
}

void line::set_direction_input() const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_direction_input();
}

void line::set_direction_output(int value) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	bulk.set_direction_output({ value });
}

bool line::event_wait(const ::std::chrono::nanoseconds& timeout) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	line_bulk event_bulk = bulk.event_wait(timeout);

	return !!event_bulk;
}

line_event line::make_line_event(const ::gpiod_line_event& event) const noexcept
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

line_event line::event_read(void) const
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

::std::vector<line_event> line::event_read_multiple(void) const
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

int line::event_get_fd(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	int ret = ::gpiod_line_event_get_fd(this->_m_line);

	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to get the line event file descriptor");

	return ret;
}

const chip line::get_chip(void) const
{
	return chip(this->_m_owner);
}

void line::update(void) const
{
	this->throw_if_null();
	line::chip_guard lock_chip(*this);

	int ret = ::gpiod_line_update(this->_m_line);

	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to update the line info");
}

void line::reset(void)
{
	this->_m_line = nullptr;
	this->_m_owner.reset();
}

bool line::operator==(const line& rhs) const noexcept
{
	return this->_m_line == rhs._m_line;
}

bool line::operator!=(const line& rhs) const noexcept
{
	return this->_m_line != rhs._m_line;
}

line::operator bool(void) const noexcept
{
	return this->_m_line != nullptr;
}

bool line::operator!(void) const noexcept
{
	return this->_m_line == nullptr;
}

void line::throw_if_null(void) const
{
	if (!this->_m_line)
		throw ::std::logic_error("object not holding a GPIO line handle");
}

line::chip_guard::chip_guard(const line& line)
	: _m_chip(line._m_owner)
{
	
}

} /* namespace gpiod */
