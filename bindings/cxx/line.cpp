// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <gpiod.hpp>
#include <system_error>

namespace gpiod {

line::line(void)
	: _m_line(nullptr),
	  _m_chip()
{

}

line::line(::gpiod_line* line, const chip& owner)
	: _m_line(line),
	  _m_chip(owner)
{

}

unsigned int line::offset(void) const
{
	this->throw_if_null();

	return ::gpiod_line_offset(this->_m_line);
}

::std::string line::name(void) const
{
	this->throw_if_null();

	const char* name = ::gpiod_line_name(this->_m_line);

	return name ? ::std::string(name) : ::std::string();
}

::std::string line::consumer(void) const
{
	this->throw_if_null();

	const char* consumer = ::gpiod_line_consumer(this->_m_line);

	return consumer ? ::std::string(consumer) : ::std::string();
}

int line::direction(void) const noexcept
{
	this->throw_if_null();

	int dir = ::gpiod_line_direction(this->_m_line);

	return dir == GPIOD_LINE_DIRECTION_INPUT ? DIRECTION_INPUT : DIRECTION_OUTPUT;
}

int line::active_state(void) const noexcept
{
	this->throw_if_null();

	int active = ::gpiod_line_active_state(this->_m_line);

	return active == GPIOD_LINE_ACTIVE_STATE_HIGH ? ACTIVE_HIGH : ACTIVE_LOW;
}

bool line::is_used(void) const
{
	this->throw_if_null();

	return ::gpiod_line_is_used(this->_m_line);
}

bool line::is_open_drain(void) const
{
	this->throw_if_null();

	return ::gpiod_line_is_open_drain(this->_m_line);
}

bool line::is_open_source(void) const
{
	this->throw_if_null();

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

bool line::event_wait(const ::std::chrono::nanoseconds& timeout) const
{
	this->throw_if_null();

	line_bulk bulk({ *this });

	line_bulk event_bulk = bulk.event_wait(timeout);

	return !!event_bulk;
}

line_event line::event_read(void) const
{
	this->throw_if_null();

	::gpiod_line_event event_buf;
	line_event event;
	int rv;

	rv = ::gpiod_line_event_read(this->_m_line, ::std::addressof(event_buf));
	if (rv < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading line event");

	if (event_buf.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
		event.event_type = line_event::RISING_EDGE;
	else if (event_buf.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
		event.event_type = line_event::FALLING_EDGE;

	event.timestamp = ::std::chrono::nanoseconds(
				event_buf.ts.tv_nsec + (event_buf.ts.tv_sec * 1000000000));

	event.source = *this;

	return event;
}

int line::event_get_fd(void) const
{
	this->throw_if_null();

	int ret = ::gpiod_line_event_get_fd(this->_m_line);

	if (ret < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to get the line event file descriptor");

	return ret;
}

const chip& line::get_chip(void) const
{
	return this->_m_chip;
}

void line::reset(void)
{
	this->_m_line = nullptr;
	this->_m_chip.reset();
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

line find_line(const ::std::string& name)
{
	line ret;

	for (auto& it: make_chip_iter()) {
		ret = it.find_line(name);
		if (ret)
			break;
	}

	return ret;
}

} /* namespace gpiod */
