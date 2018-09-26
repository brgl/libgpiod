// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/*
 * Misc tests/examples of the C++ API.
 *
 * These tests assume that at least one dummy gpiochip is present in the
 * system and that it's detected as gpiochip0.
 */

#include <gpiod.hpp>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <cstring>
#include <cerrno>
#include <functional>

#include <poll.h>

namespace {

using test_func = ::std::function<void (void)>;

::std::vector<::std::pair<::std::string, test_func>> test_funcs;

struct test_case
{
	test_case(const ::std::string& name, test_func& func)
	{
		test_funcs.push_back(::std::make_pair(name, func));
	}
};

#define TEST_CASE(_func)						\
	test_func _test_func_##_func(_func);				\
	test_case _test_case_##_func(#_func, _test_func_##_func)

std::string boolstr(bool b)
{
	return b ? "true" : "false";
}

void fire_line_event(const ::std::string& chip, unsigned int offset, bool rising)
{
	::std::string path;

	path = "/sys/kernel/debug/gpio-mockup-event/" + chip + "/" + ::std::to_string(offset);

	::std::ofstream out(path);
	if (!out)
		throw ::std::runtime_error("unable to open " + path);

	out << ::std::to_string(rising ? 1 : 0) << ::std::endl;
}

void print_event(const ::gpiod::line_event& event)
{
	::std::cerr << "type: "
		    << (event.event_type == ::gpiod::line_event::RISING_EDGE ? "rising" : "falling")
		    << "\n"
		    << "timestamp: "
		    << ::std::chrono::duration_cast<::std::chrono::seconds>(event.timestamp).count()
		    << "."
		    << event.timestamp.count() % 1000000000
		    << "\n"
		    << "source line offset: "
		    << event.source.offset()
		    << ::std::endl;	
}

void chip_open_default_lookup(void)
{
	::gpiod::chip by_name("gpiochip0");
	::gpiod::chip by_path("/dev/gpiochip0");
	::gpiod::chip by_label("gpio-mockup-A");
	::gpiod::chip by_number("0");

	::std::cerr << "all good" << ::std::endl;
}
TEST_CASE(chip_open_default_lookup);

void chip_open_different_modes(void)
{
	::gpiod::chip by_name("gpiochip0", ::gpiod::chip::OPEN_BY_NAME);
	::gpiod::chip by_path("/dev/gpiochip0", ::gpiod::chip::OPEN_BY_PATH);
	::gpiod::chip by_label("gpio-mockup-A", ::gpiod::chip::OPEN_BY_LABEL);
	::gpiod::chip by_number("0", ::gpiod::chip::OPEN_BY_NUMBER);

	::std::cerr << "all good" << ::std::endl;
}
TEST_CASE(chip_open_different_modes);	

void chip_open_nonexistent(void)
{
	try {
		::gpiod::chip chip("/nonexistent_gpiochip");
	} catch (const ::std::system_error& exc) {
		::std::cerr << "system_error thrown as expected: " << exc.what() << ::std::endl;
		return;
	}

	throw ::std::runtime_error("system_error should have been thrown");
}
TEST_CASE(chip_open_nonexistent);

void chip_open_method(void)
{
	::gpiod::chip chip;

	if (chip)
		throw ::std::runtime_error("chip should be empty");

	chip.open("gpiochip0");

	::std::cerr << "all good" << ::std::endl;
}
TEST_CASE(chip_open_method);

void chip_reset(void)
{
	::gpiod::chip chip("gpiochip0");

	if (!chip)
		throw ::std::runtime_error("chip object is not valid");

	chip.reset();

	if (chip)
		throw ::std::runtime_error("chip should be invalid");

	::std::cerr << "all good" << ::std::endl;
}
TEST_CASE(chip_reset);

void chip_info(void)
{
	::gpiod::chip chip("gpiochip0");

	::std::cerr << "chip name: " << chip.name() << ::std::endl;
	::std::cerr << "chip label: " << chip.label() << ::std::endl;
	::std::cerr << "number of lines " << chip.num_lines() << ::std::endl;
}
TEST_CASE(chip_info);

void chip_operators(void)
{
	::gpiod::chip chip1("gpiochip0");
	auto chip2 = chip1;

	if ((chip1 != chip2) || !(chip1 == chip2))
		throw ::std::runtime_error("chip objects should be equal");

	::std::cerr << "all good" << ::std::endl;
}
TEST_CASE(chip_operators);

void chip_line_ops(void)
{
	::gpiod::chip chip("gpiochip0");

	auto line = chip.get_line(3);
	::std::cerr << "Got line by offset: " << line.offset() << ::std::endl;

	line = chip.find_line("gpio-mockup-A-4");
	::std::cerr << "Got line by name: " << line.name() << ::std::endl;

	auto lines = chip.get_lines({ 1, 2, 3, 6, 6 });
	::std::cerr << "Got multiple lines by offset: ";
	for (auto& it: lines)
		::std::cerr << it.offset() << " ";
	::std::cerr << ::std::endl;

	lines = chip.find_lines({ "gpio-mockup-A-1", "gpio-mockup-A-4", "gpio-mockup-A-7" });
	::std::cerr << "Got multiple lines by name: ";
	for (auto& it: lines)
		::std::cerr << it.name() << " ";
	::std::cerr << ::std::endl;
}
TEST_CASE(chip_line_ops);

void chip_find_lines_nonexistent(void)
{
	::gpiod::chip chip("gpiochip0");

	auto lines = chip.find_lines({ "gpio-mockup-A-1", "nonexistent", "gpio-mockup-A-4" });
	if (lines)
		throw ::std::logic_error("line_bulk object should be invalid");

	::std::cerr << "line_bulk invalid as expected" << ::std::endl;
}
TEST_CASE(chip_find_lines_nonexistent);

void line_info(void)
{
	::gpiod::chip chip("gpiochip0");
	::gpiod::line line = chip.get_line(2);

	::std::cerr << "line offset: " << line.offset() << ::std::endl;
	::std::cerr << "line name: " << line.name() << ::std::endl;
	::std::cerr << "line direction: "
		    << (line.direction() == ::gpiod::line::DIRECTION_INPUT ?
			"input" : "output") << ::std::endl;
	::std::cerr << "line active state: "
		    << (line.active_state() == ::gpiod::line::ACTIVE_LOW ?
			"active low" : "active high") << :: std::endl;
}
TEST_CASE(line_info);

void empty_objects(void)
{
	::std::cerr << "Are initialized line & chip objects 'false'?" << ::std::endl;

	::gpiod::line line;
	if (line)
		throw ::std::logic_error("line built with a default constructor should be 'false'");

	::gpiod::chip chip;
	if (chip)
		throw ::std::logic_error("chip built with a default constructor should be 'false'");

	::std::cerr << "YES" << ::std::endl;
}
TEST_CASE(empty_objects);

void line_bulk_iterator(void)
{
	::std::cerr << "Checking line_bulk iterators" << ::std::endl;

	::gpiod::chip chip("gpiochip0");
	::gpiod::line_bulk bulk = chip.get_lines({ 0, 1, 2, 3, 4 });

	for (auto& it: bulk)
		::std::cerr << it.name() << ::std::endl;

	::std::cerr << "DONE" << ::std::endl;
}
TEST_CASE(line_bulk_iterator);

void single_line_test(void)
{
	const ::std::string line_name("gpio-mockup-A-4");

	::std::cerr << "Looking up a GPIO line by name (" << line_name << ")" << ::std::endl;

	auto line = ::gpiod::find_line(line_name);
	if (!line)
		throw ::std::runtime_error(line_name + " line not found");

	::std::cerr << "Requesting a single line" << ::std::endl;

	::gpiod::line_request conf;
	conf.consumer = "gpiod_cxx_tests";
	conf.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;

	line.request(conf, 1);
	::std::cerr << "Reading value" << ::std::endl;
	::std::cerr << line.get_value() << ::std::endl;
	::std::cerr << "Changing value to 0" << ::std::endl;
	line.set_value(0);
	::std::cerr << "Reading value again" << ::std::endl;
	::std::cerr << line.get_value() << ::std::endl;
}
TEST_CASE(single_line_test);

void line_flags(void)
{
	::gpiod::chip chip("gpiochip0");

	auto line = chip.get_line(0);

	::std::cerr << "line is used: " << boolstr(line.is_used()) << ::std::endl;
	::std::cerr << "line is requested: " << boolstr(line.is_requested()) << ::std::endl;

	::std::cerr << "requesting line" << ::std::endl;

	::gpiod::line_request config;
	config.consumer = "gpiod_cxx_tests";
	config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
	config.flags = ::gpiod::line_request::FLAG_OPEN_DRAIN;
	line.request(config);

	::std::cerr << "line is used: " << boolstr(line.is_used()) << ::std::endl;
	::std::cerr << "line is open drain: " << boolstr(line.is_open_drain()) << ::std::endl;
	::std::cerr << "line is open source: " << boolstr(line.is_open_source()) << ::std::endl;
	::std::cerr << "line is requested: " << boolstr(line.is_requested()) << ::std::endl;

	if (!line.is_open_drain())
		throw ::std::logic_error("line should be open-drain");
}
TEST_CASE(line_flags);

void multiple_lines_test(void)
{
	::gpiod::chip chip("gpiochip0");

	::std::cerr << "Getting multiple lines by offsets" << ::std::endl;

	auto lines = chip.get_lines({ 0, 2, 3, 4, 6 });

	::std::cerr << "Requesting them for output" << ::std::endl;

	::gpiod::line_request config;
	config.consumer = "gpiod_cxx_tests";
	config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;

	lines.request(config);

	::std::cerr << "Setting values" << ::std::endl;

	lines.set_values({ 0, 1, 1, 0, 1});

	::std::cerr << "Requesting the lines for input" << ::std::endl;

	config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
	lines.release();
	lines.request(config);

	::std::cerr << "Reading the values" << ::std::endl;

	auto vals = lines.get_values();

	for (auto& it: vals)
		::std::cerr << it << " ";
	::std::cerr << ::std::endl;
}
TEST_CASE(multiple_lines_test);

void chip_get_all_lines(void)
{
	::gpiod::chip chip("gpiochip0");

	::std::cerr << "Getting all lines from a chip" << ::std::endl;

	auto lines = chip.get_all_lines();

	for (auto& it: lines)
		::std::cerr << "Offset: " << it.offset() << ::std::endl;
}
TEST_CASE(chip_get_all_lines);

void line_event_single_line(void)
{
	::gpiod::chip chip("gpiochip0");

	auto line = chip.get_line(1);

	::gpiod::line_request config;
	config.consumer = "gpiod_cxx_tests";
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	::std::cerr << "requesting line for events" << ::std::endl;
	line.request(config);

	::std::cerr << "generating a line event" << ::std::endl;
	fire_line_event("gpiochip0", 1, true);

	if (!line.event_wait(::std::chrono::nanoseconds(1000000000)))
		throw ::std::runtime_error("waiting for event timed out");

	auto event = line.event_read();

	::std::cerr << "event received:" << ::std::endl;
	print_event(event);
}
TEST_CASE(line_event_single_line);

void line_event_multiple_lines(void)
{
	::gpiod::chip chip("gpiochip0");

	auto lines = chip.get_lines({ 1, 2, 3, 4, 5 });

	::gpiod::line_request config;
	config.consumer = "gpiod_cxx_tests";
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	::std::cerr << "requesting lines for events" << ::std::endl;
	lines.request(config);

	::std::cerr << "generating two line events" << ::std::endl;
	fire_line_event("gpiochip0", 1, true);
	fire_line_event("gpiochip0", 2, true);

	auto event_lines = lines.event_wait(::std::chrono::nanoseconds(1000000000));
	if (!event_lines || event_lines.size() != 2)
		throw ::std::runtime_error("expected two line events");

	::std::vector<::gpiod::line_event> events;
	for (auto& line: event_lines) {
		auto event = line.event_read();
		events.push_back(event);
	}

	::std::cerr << "events received:" << ::std::endl;
	for (auto& event: events)
		print_event(event);
}
TEST_CASE(line_event_multiple_lines);

void line_event_poll_fd(void)
{
	::std::map<int, ::gpiod::line> fd_line_map;

	::gpiod::chip chip("gpiochip0");
	auto lines = chip.get_lines({ 1, 2, 3, 4, 5, 6 });

	::gpiod::line_request config;
	config.consumer = "gpiod_cxx_tests";
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	::std::cerr << "requesting lines for events" << ::std::endl;
	lines.request(config);

	::std::cerr << "generating three line events" << ::std::endl;
	fire_line_event("gpiochip0", 2, true);
	fire_line_event("gpiochip0", 3, true);
	fire_line_event("gpiochip0", 5, false);

	fd_line_map[lines[1].event_get_fd()] = lines[1];
	fd_line_map[lines[2].event_get_fd()] = lines[2];
	fd_line_map[lines[4].event_get_fd()] = lines[4];

	pollfd fds[3];
	fds[0].fd = lines[1].event_get_fd();
	fds[1].fd = lines[2].event_get_fd();
	fds[2].fd = lines[4].event_get_fd();

	fds[0].events = fds[1].events = fds[2].events = POLLIN | POLLPRI;

	int rv = poll(fds, 3, 1000);
	if (rv < 0)
		throw ::std::runtime_error("error polling for events: "
						+ ::std::string(::strerror(errno)));
	else if (rv == 0)
		throw ::std::runtime_error("poll() timed out while waiting for events");

	if (rv != 3)
		throw ::std::runtime_error("expected three line events");

	::std::cerr << "events received:" << ::std::endl;
	for (unsigned int i = 0; i < 3; i++) {
		if (fds[i].revents)
			print_event(fd_line_map[fds[i].fd].event_read());
	}
}
TEST_CASE(line_event_poll_fd);

void chip_iterator(void)
{
	::std::cerr << "iterating over all GPIO chips in the system:" << ::std::endl;

	for (auto& it: ::gpiod::make_chip_iter())
		::std::cerr << it.name() << ::std::endl;
}
TEST_CASE(chip_iterator);

void line_iterator(void)
{
	::std::cerr << "iterating over all lines exposed by a GPIO chip:" << ::std::endl;

	::gpiod::chip chip("gpiochip0");

	for (auto& it: ::gpiod::line_iter(chip))
		::std::cerr << it.offset() << ": " << it.name() << ::std::endl;
}
TEST_CASE(line_iterator);

} /* namespace */

int main(int, char **)
{
	for (auto& it: test_funcs) {
		::std::cerr << "=================================================" << ::std::endl;
		::std::cerr << it.first << ":\n" << ::std::endl;
		it.second();
	}

	return EXIT_SUCCESS;
}
