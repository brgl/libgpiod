/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

#include <gpiod.hpp>

#include <system_error>
#include <map>

namespace gpiod {

namespace {

const ::std::map<int, int> reqtype_mapping = {
	{ line_request::DIRECTION_AS_IS,	GPIOD_LINE_REQUEST_DIRECTION_AS_IS, },
	{ line_request::DIRECTION_INPUT,	GPIOD_LINE_REQUEST_DIRECTION_INPUT, },
	{ line_request::DIRECTION_OUTPUT,	GPIOD_LINE_REQUEST_DIRECTION_OUTPUT, },
	{ line_request::EVENT_FALLING_EDGE,	GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE, },
	{ line_request::EVENT_RISING_EDGE,	GPIOD_LINE_REQUEST_EVENT_RISING_EDGE, },
	{ line_request::EVENT_BOTH_EDGES,	GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES, },
};

struct bitset_cmp
{
	bool operator()(const ::std::bitset<32>& lhs, const ::std::bitset<32>& rhs)
	{
		return lhs.to_ulong() < rhs.to_ulong();
	}
};

const ::std::map<::std::bitset<32>, int, bitset_cmp> reqflag_mapping = {
	{ line_request::FLAG_ACTIVE_LOW,	GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, },
	{ line_request::FLAG_OPEN_DRAIN,	GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, },
	{ line_request::FLAG_OPEN_SOURCE,	GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE, },
};

} /* namespace */

const ::std::bitset<32> line_request::FLAG_ACTIVE_LOW("001");
const ::std::bitset<32> line_request::FLAG_OPEN_SOURCE("010");
const ::std::bitset<32> line_request::FLAG_OPEN_DRAIN("100");

const unsigned int line_bulk::MAX_LINES = GPIOD_LINE_BULK_MAX_LINES;

line_bulk::line_bulk(const ::std::vector<line>& lines)
	: _m_bulk()
{
	this->_m_bulk.reserve(lines.size());

	for (auto& it: lines)
		this->add(it);
}

void line_bulk::add(const line& new_line)
{
	if (!new_line)
		throw ::std::logic_error("line_bulk cannot hold empty line objects");

	if (this->_m_bulk.size() >= MAX_LINES)
		throw ::std::logic_error("maximum number of lines reached");

	if (this->_m_bulk.size() >= 1 && this->_m_bulk.begin()->get_chip() != new_line.get_chip())
		throw std::logic_error("line_bulk cannot hold GPIO lines from different chips");

	this->_m_bulk.push_back(new_line);
}

line& line_bulk::get(unsigned int offset)
{
	return this->_m_bulk.at(offset);
}

line& line_bulk::operator[](unsigned int offset)
{
	return this->_m_bulk[offset];
}

unsigned int line_bulk::size(void) const noexcept
{
	return this->_m_bulk.size();
}

bool line_bulk::empty(void) const noexcept
{
	return this->_m_bulk.empty();
}

void line_bulk::clear(void)
{
	this->_m_bulk.clear();
}

void line_bulk::request(const line_request& config, const std::vector<int> default_vals) const
{
	this->throw_if_empty();

	if (!default_vals.empty() && this->size() != default_vals.size())
		throw ::std::invalid_argument("the number of default values must correspond with the number of lines");

	if (config.request_type == line_request::DIRECTION_OUTPUT && default_vals.empty())
		throw ::std::invalid_argument("default values are required for output mode");

	::gpiod_line_request_config conf;
	::gpiod_line_bulk bulk;
	int rv;

	this->to_line_bulk(::std::addressof(bulk));

	conf.consumer = config.consumer.c_str();
	conf.request_type = reqtype_mapping.at(config.request_type);
	conf.flags = 0;

	for (auto& it: reqflag_mapping) {
		if ((it.first & config.flags).to_ulong())
			conf.flags |= it.second;
	}

	rv = ::gpiod_line_request_bulk(::std::addressof(bulk),
				       ::std::addressof(conf), default_vals.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error requesting GPIO lines");
}

::std::vector<int> line_bulk::get_values(void) const
{
	this->throw_if_empty();

	::std::vector<int> values;
	::gpiod_line_bulk bulk;
	int rv;

	this->to_line_bulk(::std::addressof(bulk));
	values.resize(this->_m_bulk.size());

	rv = ::gpiod_line_get_value_bulk(::std::addressof(bulk), values.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading GPIO line values");

	return ::std::move(values);
}

void line_bulk::set_values(const ::std::vector<int>& values) const
{
	this->throw_if_empty();

	if (values.size() != this->_m_bulk.size())
		throw ::std::invalid_argument("the size of values array must correspond with the number of lines");

	::gpiod_line_bulk bulk;
	int rv;

	this->to_line_bulk(::std::addressof(bulk));

	rv = ::gpiod_line_set_value_bulk(::std::addressof(bulk), values.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error setting GPIO line values");
}

line_bulk line_bulk::event_wait(const ::std::chrono::nanoseconds& timeout) const
{
	this->throw_if_empty();

	::gpiod_line_bulk bulk, event_bulk;
	::timespec ts;
	line_bulk ret;
	int rv;

	this->to_line_bulk(::std::addressof(bulk));

	::gpiod_line_bulk_init(::std::addressof(event_bulk));

	ts.tv_sec = timeout.count() / 1000000000ULL;
	ts.tv_nsec = timeout.count() % 1000000000ULL;

	rv = ::gpiod_line_event_wait_bulk(::std::addressof(bulk),
					  ::std::addressof(ts),
					  ::std::addressof(event_bulk));
	if (rv < 0) {
		throw ::std::system_error(errno, ::std::system_category(),
					  "error polling for events");
	} else if (rv > 0) {
		for (unsigned int i = 0; i < event_bulk.num_lines; i++)
			ret.add(line(event_bulk.lines[i], this->_m_bulk[i].get_chip()));
	}

	return ::std::move(ret);
}

line_bulk::operator bool(void) const noexcept
{
	return !this->_m_bulk.empty();
}

bool line_bulk::operator!(void) const noexcept
{
	return this->_m_bulk.empty();
}

line_bulk::iterator::iterator(const ::std::vector<line>::iterator& it)
	: _m_iter(it)
{

}

line_bulk::iterator& line_bulk::iterator::operator++(void)
{
	this->_m_iter++;

	return *this;
}

const line& line_bulk::iterator::operator*(void) const
{
	return *this->_m_iter;
}

const line* line_bulk::iterator::operator->(void) const
{
	return this->_m_iter.operator->();
}

bool line_bulk::iterator::operator==(const iterator& rhs) const noexcept
{
	return this->_m_iter == rhs._m_iter;
}

bool line_bulk::iterator::operator!=(const iterator& rhs) const noexcept
{
	return this->_m_iter != rhs._m_iter;
}

line_bulk::iterator line_bulk::begin(void) noexcept
{
	return ::std::move(line_bulk::iterator(this->_m_bulk.begin()));
}

line_bulk::iterator line_bulk::end(void) noexcept
{
	return ::std::move(line_bulk::iterator(this->_m_bulk.end()));
}

void line_bulk::throw_if_empty(void) const
{
	if (this->_m_bulk.empty())
		throw std::logic_error("line_bulk not holding any GPIO lines");
}

void line_bulk::to_line_bulk(::gpiod_line_bulk *bulk) const
{
	::gpiod_line_bulk_init(bulk);
	for (auto& it: this->_m_bulk)
		::gpiod_line_bulk_add(bulk, it._m_line);
}

} /* namespace gpiod */
