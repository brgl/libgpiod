/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#ifndef __GPIOD_CXX_TEST_HPP__
#define __GPIOD_CXX_TEST_HPP__

#include <bitset>
#include <condition_variable>
#include <gpio-mockup.h>
#include <mutex>
#include <string>
#include <vector>
#include <thread>

namespace gpiod {
namespace test {

class mockup
{
public:

	static mockup& instance(void);

	mockup(const mockup& other) = delete;
	mockup(mockup&& other) = delete;
	mockup& operator=(const mockup& other) = delete;
	mockup& operator=(mockup&& other) = delete;

	void probe(const ::std::vector<unsigned int>& chip_sizes,
		   const ::std::bitset<32>& flags = 0);
	void remove(void);

	std::string chip_name(unsigned int idx) const;
	std::string chip_path(unsigned int idx) const;
	unsigned int chip_num(unsigned int idx) const;

	int chip_get_value(unsigned int chip_idx, unsigned int line_offset);
	void chip_set_pull(unsigned int chip_idx, unsigned int line_offset, int pull);

	static const ::std::bitset<32> FLAG_NAMED_LINES;

	class probe_guard
	{
	public:

		probe_guard(const ::std::vector<unsigned int>& chip_sizes,
			    const ::std::bitset<32>& flags = 0);
		~probe_guard(void);

		probe_guard(const probe_guard& other) = delete;
		probe_guard(probe_guard&& other) = delete;
		probe_guard& operator=(const probe_guard& other) = delete;
		probe_guard& operator=(probe_guard&& other) = delete;
	};

	class event_thread
	{
	public:

		event_thread(unsigned int chip_index, unsigned int line_offset, unsigned int period_ms);
		~event_thread(void);

		event_thread(const event_thread& other) = delete;
		event_thread(event_thread&& other) = delete;
		event_thread& operator=(const event_thread& other) = delete;
		event_thread& operator=(event_thread&& other) = delete;

	private:

		void event_worker(void);

		unsigned int _m_chip_index;
		unsigned int _m_line_offset;
		unsigned int _m_period_ms;

		bool _m_stop;

		::std::mutex _m_mutex;
		::std::condition_variable _m_cond;
		::std::thread _m_thread;
	};

private:

	mockup(void);
	~mockup(void);
	
	::gpio_mockup *_m_mockup;
};

} /* namespace test */
} /* namespace gpiod */

#endif /* __GPIOD_CXX_TEST_HPP__ */
