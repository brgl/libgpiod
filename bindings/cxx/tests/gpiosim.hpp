/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __GPIOD_CXX_GPIOSIM_HPP__
#define __GPIOD_CXX_GPIOSIM_HPP__

#include <filesystem>
#include <memory>

namespace gpiosim {

class chip_builder;

class chip
{
public:
	enum class pull {
		PULL_UP = 1,
		PULL_DOWN
	};

	enum class value {
		INACTIVE = 0,
		ACTIVE = 1
	};

	chip(const chip& other) = delete;
	chip(chip&& other);
	~chip();

	chip& operator=(const chip& other) = delete;
	chip& operator=(chip&& other);

	::std::filesystem::path dev_path() const;
	::std::string name() const;

	value get_value(unsigned int offset);
	void set_pull(unsigned int offset, pull pull);

private:

	chip();

	struct impl;

	::std::unique_ptr<impl> _m_priv;

	friend chip_builder;
};

class chip_builder
{
public:
	enum class direction {
		INPUT = 1,
		OUTPUT_HIGH,
		OUTPUT_LOW
	};

	chip_builder();
	chip_builder(const chip_builder& other) = delete;
	chip_builder(chip_builder&& other);
	~chip_builder();

	chip_builder& operator=(const chip_builder& other) = delete;
	chip_builder& operator=(chip_builder&& other);

	chip_builder& set_num_lines(::std::size_t num_lines);
	chip_builder& set_label(const ::std::string& label);
	chip_builder& set_line_name(unsigned int offset, const ::std::string& name);
	chip_builder& set_hog(unsigned int offset, const ::std::string& name, direction direction);

	chip build();

private:

	struct impl;

	::std::unique_ptr<impl> _m_priv;
};

chip_builder make_sim();

} /* namespace gpiosim */

#endif /* __GPIOD_CXX_GPIOSIM_HPP__ */
