// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <gpiod.hpp>
#include <map>
#include <system_error>

#include "internal.hpp"

namespace gpiod {

GPIOD_CXX_API const ::std::bitset<32> line_request::FLAG_ACTIVE_LOW(GPIOD_BIT(0));
GPIOD_CXX_API const ::std::bitset<32> line_request::FLAG_OPEN_SOURCE(GPIOD_BIT(1));
GPIOD_CXX_API const ::std::bitset<32> line_request::FLAG_OPEN_DRAIN(GPIOD_BIT(2));
GPIOD_CXX_API const ::std::bitset<32> line_request::FLAG_BIAS_DISABLED(GPIOD_BIT(3));
GPIOD_CXX_API const ::std::bitset<32> line_request::FLAG_BIAS_PULL_DOWN(GPIOD_BIT(4));
GPIOD_CXX_API const ::std::bitset<32> line_request::FLAG_BIAS_PULL_UP(GPIOD_BIT(5));

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
	bool operator()(const ::std::bitset<32>& lhs, const ::std::bitset<32>& rhs) const
	{
		return lhs.to_ulong() < rhs.to_ulong();
	}
};

const ::std::map<::std::bitset<32>, int, bitset_cmp> reqflag_mapping = {
	{ line_request::FLAG_ACTIVE_LOW,	GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, },
	{ line_request::FLAG_OPEN_DRAIN,	GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, },
	{ line_request::FLAG_OPEN_SOURCE,	GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE, },
	{ line_request::FLAG_BIAS_DISABLED,	GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED, },
	{ line_request::FLAG_BIAS_PULL_DOWN,	GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN, },
	{ line_request::FLAG_BIAS_PULL_UP,	GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP, },
};

} /* namespace */

GPIOD_CXX_API const unsigned int line_bulk::MAX_LINES = 64;

GPIOD_CXX_API line_bulk::line_bulk(const ::std::vector<line>& lines)
	: _m_bulk()
{
	this->_m_bulk.reserve(lines.size());

	for (auto& it: lines)
		this->append(it);
}

GPIOD_CXX_API void line_bulk::append(const line& new_line)
{
	if (!new_line)
		throw ::std::logic_error("line_bulk cannot hold empty line objects");

	if (this->_m_bulk.size() >= MAX_LINES)
		throw ::std::logic_error("maximum number of lines reached");

	if (this->_m_bulk.size() >= 1 && this->_m_bulk.begin()->get_chip() != new_line.get_chip())
		throw ::std::logic_error("line_bulk cannot hold GPIO lines from different chips");

	this->_m_bulk.push_back(new_line);
}

GPIOD_CXX_API line& line_bulk::get(unsigned int index)
{
	return this->_m_bulk.at(index);
}

GPIOD_CXX_API line& line_bulk::operator[](unsigned int index)
{
	return this->_m_bulk[index];
}

GPIOD_CXX_API unsigned int line_bulk::size(void) const noexcept
{
	return this->_m_bulk.size();
}

GPIOD_CXX_API bool line_bulk::empty(void) const noexcept
{
	return this->_m_bulk.empty();
}

GPIOD_CXX_API void line_bulk::clear(void)
{
	this->_m_bulk.clear();
}

GPIOD_CXX_API void line_bulk::request(const line_request& config, const ::std::vector<int> default_vals) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	if (!default_vals.empty() && this->size() != default_vals.size())
		throw ::std::invalid_argument("the number of default values must correspond with the number of lines");

	::gpiod_line_request_config conf;
	auto bulk = this->to_line_bulk();
	int rv;

	conf.consumer = config.consumer.c_str();
	conf.request_type = reqtype_mapping.at(config.request_type);
	conf.flags = 0;

	for (auto& it: reqflag_mapping) {
		if ((it.first & config.flags).to_ulong())
			conf.flags |= it.second;
	}

	rv = ::gpiod_line_request_bulk(bulk.get(),
				       ::std::addressof(conf),
				       default_vals.empty() ? NULL : default_vals.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error requesting GPIO lines");
}

GPIOD_CXX_API void line_bulk::release(void) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	auto bulk = this->to_line_bulk();

	::gpiod_line_release_bulk(bulk.get());
}

GPIOD_CXX_API ::std::vector<int> line_bulk::get_values(void) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	auto bulk = this->to_line_bulk();
	::std::vector<int> values;
	int rv;

	values.resize(this->_m_bulk.size());

	rv = ::gpiod_line_get_value_bulk(bulk.get(), values.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading GPIO line values");

	return values;
}

GPIOD_CXX_API void line_bulk::set_values(const ::std::vector<int>& values) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	if (values.size() != this->_m_bulk.size())
		throw ::std::invalid_argument("the size of values array must correspond with the number of lines");

	auto bulk = this->to_line_bulk();
	int rv;

	rv = ::gpiod_line_set_value_bulk(bulk.get(), values.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error setting GPIO line values");
}

GPIOD_CXX_API void line_bulk::set_config(int direction, ::std::bitset<32> flags,
					 const ::std::vector<int> values) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	if (!values.empty() && this->_m_bulk.size() != values.size())
		throw ::std::invalid_argument("the number of default values must correspond with the number of lines");

	auto bulk = this->to_line_bulk();
	int rv, gflags;

	gflags = 0;

	for (auto& it: reqflag_mapping) {
		if ((it.first & flags).to_ulong())
			gflags |= it.second;
	}

	rv = ::gpiod_line_set_config_bulk(bulk.get(), direction,
					  gflags, values.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error setting GPIO line config");
}

GPIOD_CXX_API void line_bulk::set_flags(::std::bitset<32> flags) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	auto bulk = this->to_line_bulk();
	int rv, gflags;

	gflags = 0;

	for (auto& it: reqflag_mapping) {
		if ((it.first & flags).to_ulong())
			gflags |= it.second;
	}

	rv = ::gpiod_line_set_flags_bulk(bulk.get(), gflags);
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error setting GPIO line flags");
}

GPIOD_CXX_API void line_bulk::set_direction_input() const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	auto bulk = this->to_line_bulk();
	int rv;

	rv = ::gpiod_line_set_direction_input_bulk(bulk.get());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
			"error setting GPIO line direction to input");
}

GPIOD_CXX_API void line_bulk::set_direction_output(const ::std::vector<int>& values) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	if (values.size() != this->_m_bulk.size())
		throw ::std::invalid_argument("the size of values array must correspond with the number of lines");

	auto bulk = this->to_line_bulk();
	int rv;

	rv = ::gpiod_line_set_direction_output_bulk(bulk.get(), values.data());
	if (rv)
		throw ::std::system_error(errno, ::std::system_category(),
			"error setting GPIO line direction to output");
}

GPIOD_CXX_API line_bulk line_bulk::event_wait(const ::std::chrono::nanoseconds& timeout) const
{
	this->throw_if_empty();
	line::chip_guard lock_chip(this->_m_bulk.front());

	auto ev_bulk = this->make_line_bulk_ptr();
	auto bulk = this->to_line_bulk();
	::timespec ts;
	line_bulk ret;
	int rv;

	ts.tv_sec = timeout.count() / 1000000000ULL;
	ts.tv_nsec = timeout.count() % 1000000000ULL;

	rv = ::gpiod_line_event_wait_bulk(bulk.get(), ::std::addressof(ts), ev_bulk.get());
	if (rv < 0) {
		throw ::std::system_error(errno, ::std::system_category(),
					  "error polling for events");
	} else if (rv > 0) {
		auto chip = this->_m_bulk[0].get_chip();
		auto num_lines = ::gpiod_line_bulk_num_lines(ev_bulk.get());

		for (unsigned int i = 0; i < num_lines; i++)
			ret.append(line(::gpiod_line_bulk_get_line(ev_bulk.get(), i), chip));
	}

	return ret;
}

GPIOD_CXX_API line_bulk::operator bool(void) const noexcept
{
	return !this->_m_bulk.empty();
}

GPIOD_CXX_API bool line_bulk::operator!(void) const noexcept
{
	return this->_m_bulk.empty();
}

GPIOD_CXX_API line_bulk::iterator::iterator(const ::std::vector<line>::iterator& it)
	: _m_iter(it)
{

}

GPIOD_CXX_API line_bulk::iterator& line_bulk::iterator::operator++(void)
{
	this->_m_iter++;

	return *this;
}

GPIOD_CXX_API const line& line_bulk::iterator::operator*(void) const
{
	return *this->_m_iter;
}

GPIOD_CXX_API const line* line_bulk::iterator::operator->(void) const
{
	return this->_m_iter.operator->();
}

GPIOD_CXX_API bool line_bulk::iterator::operator==(const iterator& rhs) const noexcept
{
	return this->_m_iter == rhs._m_iter;
}

GPIOD_CXX_API bool line_bulk::iterator::operator!=(const iterator& rhs) const noexcept
{
	return this->_m_iter != rhs._m_iter;
}

GPIOD_CXX_API line_bulk::iterator line_bulk::begin(void) noexcept
{
	return line_bulk::iterator(this->_m_bulk.begin());
}

GPIOD_CXX_API line_bulk::iterator line_bulk::end(void) noexcept
{
	return line_bulk::iterator(this->_m_bulk.end());
}

GPIOD_CXX_API void line_bulk::throw_if_empty(void) const
{
	if (this->_m_bulk.empty())
		throw ::std::logic_error("line_bulk not holding any GPIO lines");
}

GPIOD_CXX_API line_bulk::line_bulk_ptr line_bulk::make_line_bulk_ptr(void) const
{
	line_bulk_ptr bulk(::gpiod_line_bulk_new(this->size()));

	if (!bulk)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to allocate new bulk object");

	return bulk;
}

GPIOD_CXX_API line_bulk::line_bulk_ptr line_bulk::to_line_bulk(void) const
{
	line_bulk_ptr bulk = this->make_line_bulk_ptr();

	for (auto& it: this->_m_bulk)
		::gpiod_line_bulk_add_line(bulk.get(), it._m_line);

	return bulk;
}

GPIOD_CXX_API void line_bulk::line_bulk_deleter::operator()(::gpiod_line_bulk *bulk)
{
	::gpiod_line_bulk_free(bulk);
}

} /* namespace gpiod */
