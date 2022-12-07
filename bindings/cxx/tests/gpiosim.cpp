/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#include <map>
#include <system_error>
#include <utility>

#include "gpiosim.h"
#include "gpiosim.hpp"

namespace gpiosim {

namespace {

const ::std::map<chip::pull, gpiosim_pull> pull_mapping = {
	{ chip::pull::PULL_UP,		GPIOSIM_PULL_UP },
	{ chip::pull::PULL_DOWN,	GPIOSIM_PULL_DOWN },
};

const ::std::map<chip_builder::direction, gpiosim_direction> hog_dir_mapping = {
	{ chip_builder::direction::INPUT,	GPIOSIM_DIRECTION_INPUT },
	{ chip_builder::direction::OUTPUT_HIGH,	GPIOSIM_DIRECTION_OUTPUT_HIGH },
	{ chip_builder::direction::OUTPUT_LOW,	GPIOSIM_DIRECTION_OUTPUT_LOW },
};

const ::std::map<gpiosim_value, chip::value> value_mapping = {
	{ GPIOSIM_VALUE_INACTIVE,	chip::value::INACTIVE },
	{ GPIOSIM_VALUE_ACTIVE,		chip::value::ACTIVE },
};

template<class gpiosim_type, void free_func(gpiosim_type*)> struct deleter
{
	void operator()(gpiosim_type* ptr)
	{
		free_func(ptr);
	}
};

using ctx_deleter = deleter<::gpiosim_ctx, ::gpiosim_ctx_unref>;
using dev_deleter = deleter<::gpiosim_dev, ::gpiosim_dev_unref>;
using bank_deleter = deleter<::gpiosim_bank, ::gpiosim_bank_unref>;

using ctx_ptr = ::std::unique_ptr<::gpiosim_ctx, ctx_deleter>;
using dev_ptr = ::std::unique_ptr<::gpiosim_dev, dev_deleter>;
using bank_ptr = ::std::unique_ptr<::gpiosim_bank, bank_deleter>;

ctx_ptr sim_ctx;

class sim_ctx_initializer
{
public:
	sim_ctx_initializer()
	{
		sim_ctx.reset(gpiosim_ctx_new());
		if (!sim_ctx)
			throw ::std::system_error(errno, ::std::system_category(),
						  "unable to create the GPIO simulator context");
	}
};

dev_ptr make_sim_dev()
{
	static sim_ctx_initializer ctx_initializer;

	dev_ptr dev(::gpiosim_dev_new(sim_ctx.get()));
	if (!dev)
		throw ::std::system_error(errno, ::std::system_category(),
					  "failed to create a new GPIO simulator device");

	return dev;
}

bank_ptr make_sim_bank(const dev_ptr& dev)
{
	bank_ptr bank(::gpiosim_bank_new(dev.get()));
	if (!bank)
		throw ::std::system_error(errno, ::std::system_category(),
					  "failed to create a new GPIO simulator bank");

	return bank;
}

} /* namespace */

struct chip::impl
{
	impl()
		: dev(make_sim_dev()),
		  bank(make_sim_bank(this->dev))
	{

	}

	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	~impl() = default;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	dev_ptr dev;
	bank_ptr bank;
};

chip::chip()
	: _m_priv(new impl)
{

}

chip::chip(chip&& other)
	: _m_priv(::std::move(other._m_priv))
{

}

chip::~chip()
{

}

chip& chip::operator=(chip&& other)
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

::std::filesystem::path chip::dev_path() const
{
	return ::gpiosim_bank_get_dev_path(this->_m_priv->bank.get());
}

::std::string chip::name() const
{
	return ::gpiosim_bank_get_chip_name(this->_m_priv->bank.get());
}

chip::value chip::get_value(unsigned int offset)
{
	auto val = ::gpiosim_bank_get_value(this->_m_priv->bank.get(), offset);
	if (val == GPIOSIM_VALUE_ERROR)
		throw ::std::system_error(errno, ::std::system_category(),
					  "failed to read the simulated GPIO line value");

	return value_mapping.at(val);
}

void chip::set_pull(unsigned int offset, pull pull)
{
	auto ret = ::gpiosim_bank_set_pull(this->_m_priv->bank.get(),
					   offset, pull_mapping.at(pull));
	if (ret)
		throw ::std::system_error(errno, ::std::system_category(),
					  "failed to set the pull of simulated GPIO line");
}

struct chip_builder::impl
{
	impl()
		: num_lines(0),
		  label(),
		  line_names(),
		  hogs()
	{

	}

	::std::size_t num_lines;
	::std::string label;
	::std::map<unsigned int, ::std::string> line_names;
	::std::map<unsigned int, ::std::pair<::std::string, direction>> hogs;
};

chip_builder::chip_builder()
	: _m_priv(new impl)
{

}

chip_builder::chip_builder(chip_builder&& other)
	: _m_priv(::std::move(other._m_priv))
{

}

chip_builder::~chip_builder()
{

}

chip_builder& chip_builder::operator=(chip_builder&& other)
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

chip_builder& chip_builder::set_num_lines(::std::size_t num_lines)
{
	this->_m_priv->num_lines = num_lines;

	return *this;
}

chip_builder& chip_builder::set_label(const ::std::string& label)
{
	this->_m_priv->label = label;

	return *this;
}

chip_builder& chip_builder::set_line_name(unsigned int offset, const ::std::string& name)
{
	this->_m_priv->line_names[offset] = name;

	return *this;
}

chip_builder& chip_builder::set_hog(unsigned int offset, const ::std::string& name, direction direction)
{
	this->_m_priv->hogs[offset] = { name, direction };

	return *this;
}

chip chip_builder::build()
{
	chip sim;
	int ret;

	if (this->_m_priv->num_lines) {
		ret = ::gpiosim_bank_set_num_lines(sim._m_priv->bank.get(),
						   this->_m_priv->num_lines);
		if (ret)
			throw ::std::system_error(errno, ::std::system_category(),
						  "failed to set number of lines");
	}

	if (!this->_m_priv->label.empty()) {
		ret = ::gpiosim_bank_set_label(sim._m_priv->bank.get(),
					       this->_m_priv->label.c_str());
		if (ret)
			throw ::std::system_error(errno, ::std::system_category(),
						  "failed to set the chip label");
	}

	for (const auto& name: this->_m_priv->line_names) {
		ret = ::gpiosim_bank_set_line_name(sim._m_priv->bank.get(),
						 name.first, name.second.c_str());
		if (ret)
			throw ::std::system_error(errno, ::std::system_category(),
						  "failed to set the line name");
	}

	for (const auto& hog: this->_m_priv->hogs) {
		ret = ::gpiosim_bank_hog_line(sim._m_priv->bank.get(), hog.first,
					      hog.second.first.c_str(),
					      hog_dir_mapping.at(hog.second.second));
		if (ret)
			throw ::std::system_error(errno, ::std::system_category(),
						  "failed to hog the line");
	}

	ret = ::gpiosim_dev_enable(sim._m_priv->dev.get());
	if (ret)
		throw ::std::system_error(errno, ::std::system_category(),
					  "failed to enable the simulated GPIO device");

	return sim;
}

chip_builder make_sim()
{
	return chip_builder();
}

} /* namespace gpiosim */
