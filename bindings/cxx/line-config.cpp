// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <cstdlib>
#include <iterator>
#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

#include "internal.hpp"

namespace gpiod {

namespace {

::gpiod_line_config* make_line_config()
{
	::gpiod_line_config *config = ::gpiod_line_config_new();
	if (!config)
		throw_from_errno("Unable to allocate the line config object");

	return config;
}

struct malloc_deleter
{
	void operator()(void* ptr)
	{
		::free(ptr);
	}
};

} /* namespace */

line_config::impl::impl()
	: config(make_line_config())
{

}

GPIOD_CXX_API line_config::line_config()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API line_config::line_config(line_config&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API line_config::~line_config()
{

}

line_config& line_config::operator=(const line_config& other)
{
	this->_m_priv = other._m_priv;

	return *this;
}

GPIOD_CXX_API line_config& line_config::operator=(line_config&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API line_config& line_config::reset() noexcept
{
	::gpiod_line_config_reset(this->_m_priv->config.get());

	return *this;
}

GPIOD_CXX_API line_config& line_config::add_line_settings(line::offset offset,
							  const line_settings& settings)
{
	return this->add_line_settings(line::offsets({offset}), settings);
}

GPIOD_CXX_API line_config& line_config::add_line_settings(const line::offsets& offsets,
							  const line_settings& settings)
{
	::std::vector<unsigned int> raw_offsets(offsets.size());

	for (unsigned int i = 0; i < offsets.size(); i++)
		raw_offsets[i] = offsets[i];

	auto ret = ::gpiod_line_config_add_line_settings(this->_m_priv->config.get(),
							 raw_offsets.data(), raw_offsets.size(),
							 settings._m_priv->settings.get());
	if (ret)
		throw_from_errno("unable to add line settings");

	return *this;
}

GPIOD_CXX_API ::std::map<line::offset, line_settings> line_config::get_line_settings() const
{
	::std::map<line::offset, line_settings> settings_map;
	::std::size_t num_offsets;
	unsigned int *offsets_ptr;
	int ret;

	ret = ::gpiod_line_config_get_offsets(this->_m_priv->config.get(),
					      &num_offsets, &offsets_ptr);
	if (ret)
		throw_from_errno("unable to retrieve line offsets");

	if (num_offsets == 0)
		return settings_map;

	::std::unique_ptr<unsigned int, malloc_deleter> offsets(offsets_ptr);

	for (size_t i = 0; i < num_offsets; i++) {
		line_settings settings;

		settings._m_priv->settings.reset(::gpiod_line_config_get_line_settings(
							this->_m_priv->config.get(),
							offsets.get()[i]));
		if (!settings._m_priv->settings)
			throw_from_errno("unable to retrieve line settings");

		settings_map[offsets.get()[i]] = ::std::move(settings);
	}

	return settings_map;
}

GPIOD_CXX_API ::std::ostream&
operator<<(::std::ostream& out, const line_config& config)
{
	auto settings_map = config.get_line_settings();
	::std::vector<::std::string> vec;

	out << "gpiod::line_config(num_settings=" << settings_map.size();

	if (settings_map.size() == 0) {
		out << ")";
		return out;
	}

	for (const auto& [offset, settings]: settings_map) {
		::std::stringstream str;

		str << offset << ": " << settings;
		vec.push_back(str.str());
	}

	out << ", settings=[";
	::std::copy(vec.begin(), ::std::prev(vec.end()),
		    ::std::ostream_iterator<::std::string>(out, ", "));
	out << vec.back();
	out << "])";

	return out;
}

} /* namespace gpiod */
