// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch.hpp>
#include <filesystem>
#include <gpiod.hpp>
#include <string>
#include <regex>
#include <unistd.h>

#include "gpiosim.hpp"
#include "helpers.hpp"

using ::gpiosim::make_sim;

namespace {

class symlink_guard
{
public:
	symlink_guard(const ::std::filesystem::path& target,
		      const ::std::filesystem::path& link)
		: _m_link(link)
	{
		::std::filesystem::create_symlink(target, this->_m_link);
	}

	~symlink_guard()
	{
		::std::filesystem::remove(this->_m_link);
	}

private:
	::std::filesystem::path _m_link;
};

TEST_CASE("is_gpiochip_device() works", "[misc][chip]")
{
	SECTION("is_gpiochip_device() returns false for /dev/null")
	{
		REQUIRE_FALSE(::gpiod::is_gpiochip_device("/dev/null"));
	}

	SECTION("is_gpiochip_device() returns false for nonexistent file")
	{
		REQUIRE_FALSE(::gpiod::is_gpiochip_device("/dev/nonexistent"));
	}

	SECTION("is_gpiochip_device() returns true for a GPIO chip")
	{
		auto sim = make_sim().build();

		REQUIRE(::gpiod::is_gpiochip_device(sim.dev_path()));
	}

	SECTION("is_gpiochip_device() can resolve a symlink")
	{
		auto sim = make_sim().build();
		::std::string link("/tmp/gpiod-cxx-tmp-link.");

		link += ::std::to_string(::getpid());

		symlink_guard link_guard(sim.dev_path(), link);

		REQUIRE(::gpiod::is_gpiochip_device(link));
	}
}

TEST_CASE("api_version() returns a valid API version", "[misc]")
{
	SECTION("check api_version() format")
	{
		REQUIRE_THAT(::gpiod::api_version(),
			     regex_matcher("^[0-9][1-9]?\\.[0-9][1-9]?([\\.0-9]?|\\-devel|\\-rc[0-9])$"));
	}
}

} /* namespace */
