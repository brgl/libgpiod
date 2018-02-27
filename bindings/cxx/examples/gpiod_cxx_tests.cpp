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

/* Misc tests/examples of the C++ API. */

#include <gpiod.hpp>

#include <stdexcept>
#include <cstdlib>
#include <iostream>

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

void chip_info(void)
{
	::gpiod::chip chip("gpiochip0");

	::std::cout << "chip name: " << chip.name() << ::std::endl;
	::std::cout << "chip label: " << chip.label() << ::std::endl;
	::std::cout << "number of lines " << chip.num_lines() << ::std::endl;
}
TEST_CASE(chip_info);

void chip_open_different_modes(void)
{
	::gpiod::chip by_name("gpiochip0", ::gpiod::chip::OPEN_BY_NAME);
	::gpiod::chip by_path("/dev/gpiochip0", ::gpiod::chip::OPEN_BY_PATH);
	::gpiod::chip by_label("gpio-mockup-A", ::gpiod::chip::OPEN_BY_LABEL);
	::gpiod::chip by_number("0", ::gpiod::chip::OPEN_BY_NUMBER);
}
TEST_CASE(chip_open_different_modes);

void chip_line_ops(void)
{
	::gpiod::chip chip("gpiochip0");

	::gpiod::line line = chip.get_line(3);
	::std::cout << "Got line by offset: " << line.offset() << ::std::endl;

	line = chip.find_line("gpio-mockup-A-4");
	::std::cout << "Got line by name: " << line.name() << ::std::endl;

	::gpiod::line_bulk lines = chip.get_lines({ 1, 2, 3, 6, 6 });
	::std::cout << "Got multiple lines by offset: ";
	for (auto& it: lines)
		::std::cout << it.offset() << " ";
	::std::cout << ::std::endl;

	lines = chip.find_lines({ "gpio-mockup-A-1", "gpio-mockup-A-4", "gpio-mockup-A-7" });
	::std::cout << "Got multiple lines by name: ";
	for (auto& it: lines)
		::std::cout << it.name() << " ";
	::std::cout << ::std::endl;
}
TEST_CASE(chip_line_ops);

void line_info(void)
{
	::gpiod::chip chip("gpiochip0");
	::gpiod::line line = chip.get_line(2);

	::std::cout << "line offset: " << line.offset() << ::std::endl;
	::std::cout << "line name: " << line.name() << ::std::endl;
	::std::cout << "line direction: "
		    << (line.direction() == ::gpiod::line::DIRECTION_INPUT ?
			"input" : "output") << ::std::endl;
}
TEST_CASE(line_info);

void empty_objects(void)
{
	::std::cout << "Are initialized line & chip objects 'false'?" << ::std::endl;

	::gpiod::line line;
	if (line)
		throw ::std::logic_error("line built with a default constructor should be 'false'");

	::gpiod::chip chip;
	if (chip)
		throw ::std::logic_error("chip built with a default constructor should be 'false'");

	::std::cout << "YES" << ::std::endl;
}
TEST_CASE(empty_objects);

void line_bulk_iterator(void)
{
	::std::cout << "Checking line_bulk iterators" << ::std::endl;

	::gpiod::chip chip("gpiochip0");
	::gpiod::line_bulk bulk = chip.get_lines({ 0, 1, 2, 3, 4 });

	for (auto& it: bulk)
		::std::cout << it.name() << ::std::endl;

	::std::cout << "DONE" << ::std::endl;
}
TEST_CASE(line_bulk_iterator);

void single_line_test(void)
{
	const ::std::string line_name("gpio-mockup-A-4");

	::std::cout << "Looking up a GPIO line by name (" << line_name << ")" << ::std::endl;

	::gpiod::line line = ::gpiod::find_line(line_name);
	if (!line)
		throw ::std::runtime_error(line_name + " line not found");

	::std::cout << "Requesting a single line" << ::std::endl;

	::gpiod::line_request conf;
	conf.consumer = "gpiod_cxx_tests";
	conf.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;

	line.request(conf, 1);
	::std::cout << "Reading value" << ::std::endl;
	::std::cout << line.get_value() << ::std::endl;
	::std::cout << "Changing value to 0" << ::std::endl;
	line.set_value(0);
	::std::cout << "Reading value again" << ::std::endl;
	::std::cout << line.get_value() << ::std::endl;
}
TEST_CASE(single_line_test);

void multiple_lines_test(void)
{
	::gpiod::chip chip("gpiochip0");

	::std::cout << "Getting multiple lines by offsets" << ::std::endl;

	::gpiod::line_bulk lines = chip.get_lines({ 0, 2, 3, 4, 6 });

	::std::cout << "Requesting them for output" << ::std::endl;

	::gpiod::line_request config;
	config.consumer = "gpiod_cxx_tests";
	config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;

	lines.request(config);

	::std::cout << "Setting values" << ::std::endl;

	lines.set_values({ 0, 1, 1, 0, 1});

	::std::cout << "Requesting the lines for input" << ::std::endl;

	config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
	lines.release();
	lines.request(config);

	::std::cout << "Reading the values" << ::std::endl;

	auto vals = lines.get_values();

	for (auto& it: vals)
		::std::cout << it << " ";
	::std::cout << ::std::endl;
}
TEST_CASE(multiple_lines_test);

void get_all_lines(void)
{
	::gpiod::chip chip("gpiochip0");

	::std::cout << "Getting all lines from a chip" << ::std::endl;

	auto lines = chip.get_all_lines();

	for (auto& it: lines)
		::std::cout << "Offset: " << it.offset() << ::std::endl;
}
TEST_CASE(get_all_lines);

} /* namespace */

int main(int, char **)
{
	for (auto& it: test_funcs) {
		::std::cout << "=================================================" << ::std::endl;
		::std::cout << it.first << ":\n" << ::std::endl;
		it.second();
	}

	return EXIT_SUCCESS;
}
