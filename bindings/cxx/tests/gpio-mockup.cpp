/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <system_error>

#include "gpio-mockup.hpp"

namespace gpiod {
namespace test {

const ::std::bitset<32> mockup::FLAG_NAMED_LINES("1");

mockup& mockup::instance(void)
{
	static mockup mockup;

	return mockup;
}

mockup::mockup(void) : _m_mockup(::gpio_mockup_new())
{
	if (!this->_m_mockup)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to create the gpio-mockup context");
}

mockup::~mockup(void)
{
	::gpio_mockup_unref(this->_m_mockup);
}

void mockup::probe(const ::std::vector<unsigned int>& chip_sizes,
		   const ::std::bitset<32>& flags)
{
	int ret, probe_flags = 0;

	if (flags.to_ulong() & FLAG_NAMED_LINES.to_ulong())
		probe_flags |= GPIO_MOCKUP_FLAG_NAMED_LINES;

	ret = ::gpio_mockup_probe(this->_m_mockup, chip_sizes.size(),
				  chip_sizes.data(), probe_flags);
	if (ret)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to probe gpio-mockup module");
}

void mockup::remove(void)
{
	int ret = ::gpio_mockup_remove(this->_m_mockup);
	if (ret)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to remove gpio-mockup module");
}

::std::string mockup::chip_name(unsigned int idx) const
{
	const char *name = ::gpio_mockup_chip_name(this->_m_mockup, idx);
	if (!name)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to retrieve the chip name");

	return ::std::string(name);
}

::std::string mockup::chip_path(unsigned int idx) const
{
	const char *path = ::gpio_mockup_chip_path(this->_m_mockup, idx);
	if (!path)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to retrieve the chip path");

	return ::std::string(path);
}

unsigned int mockup::chip_num(unsigned int idx) const
{
	int num = ::gpio_mockup_chip_num(this->_m_mockup, idx);
	if (num < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "unable to retrieve the chip number");

	return num;
}

int mockup::chip_get_value(unsigned int chip_idx, unsigned int line_offset)
{
	int val = ::gpio_mockup_get_value(this->_m_mockup, chip_idx, line_offset);
	if (val < 0)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error reading the line value");

	return val;
}

void mockup::chip_set_pull(unsigned int chip_idx, unsigned int line_offset, int pull)
{
	int ret = ::gpio_mockup_set_pull(this->_m_mockup, chip_idx, line_offset, pull);
	if (ret)
		throw ::std::system_error(errno, ::std::system_category(),
					  "error setting line pull");
}

mockup::probe_guard::probe_guard(const ::std::vector<unsigned int>& chip_sizes,
				 const ::std::bitset<32>& flags)
{
	mockup::instance().probe(chip_sizes, flags);
}

mockup::probe_guard::~probe_guard(void)
{
	mockup::instance().remove();
}

mockup::event_thread::event_thread(unsigned int chip_index,
				   unsigned int line_offset, unsigned int freq)
	: _m_chip_index(chip_index),
	  _m_line_offset(line_offset),
	  _m_freq(freq),
	  _m_stop(false),
	  _m_mutex(),
	  _m_cond(),
	  _m_thread(&event_thread::event_worker, this)
{

}

mockup::event_thread::~event_thread(void)
{
	::std::unique_lock<::std::mutex> lock(this->_m_mutex);
	this->_m_stop = true;
	this->_m_cond.notify_all();
	lock.unlock();
	this->_m_thread.join();
}

void mockup::event_thread::event_worker(void)
{
	for (unsigned int i = 0;; i++) {
		::std::unique_lock<::std::mutex> lock(this->_m_mutex);

		if (this->_m_stop)
			break;

		::std::cv_status status = this->_m_cond.wait_for(lock,
						std::chrono::milliseconds(this->_m_freq));
		if (status == ::std::cv_status::timeout)
			mockup::instance().chip_set_pull(this->_m_chip_index,
							 this->_m_line_offset, i % 2);
	}
}

} /* namespace test */
} /* namespace gpiod */
