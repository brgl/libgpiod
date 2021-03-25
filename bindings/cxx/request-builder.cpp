// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

struct request_builder::impl
{
	impl(chip& parent)
		: line_cfg(),
		  req_cfg(),
		  parent(parent)
	{

	}

	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	line_config line_cfg;
	request_config req_cfg;
	chip parent;
};

GPIOD_CXX_API request_builder::request_builder(chip& chip)
	: _m_priv(new impl(chip))
{

}

GPIOD_CXX_API request_builder::request_builder(request_builder&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API request_builder::~request_builder()
{

}

GPIOD_CXX_API request_builder& request_builder::operator=(request_builder&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API request_builder& request_builder::set_request_config(request_config& req_cfg)
{
	this->_m_priv->req_cfg = req_cfg;

	return *this;
}

GPIOD_CXX_API const request_config& request_builder::get_request_config() const noexcept
{
	return this->_m_priv->req_cfg;
}

GPIOD_CXX_API request_builder&
request_builder::set_consumer(const ::std::string& consumer) noexcept
{
	this->_m_priv->req_cfg.set_consumer(consumer);

	return *this;
}

GPIOD_CXX_API request_builder&
request_builder::set_event_buffer_size(::std::size_t event_buffer_size) noexcept
{
	this->_m_priv->req_cfg.set_event_buffer_size(event_buffer_size);

	return *this;
}

GPIOD_CXX_API request_builder& request_builder::set_line_config(line_config &line_cfg)
{
	this->_m_priv->line_cfg = line_cfg;

	return *this;
}

GPIOD_CXX_API const line_config& request_builder::get_line_config() const noexcept
{
	return this->_m_priv->line_cfg;
}

GPIOD_CXX_API request_builder&
request_builder::add_line_settings(line::offset offset, const line_settings& settings)
{
	return this->add_line_settings(line::offsets({offset}), settings);
}

GPIOD_CXX_API request_builder&
request_builder::add_line_settings(const line::offsets& offsets, const line_settings& settings)
{
	this->_m_priv->line_cfg.add_line_settings(offsets, settings);

	return *this;
}

GPIOD_CXX_API line_request request_builder::do_request()
{
	line_request_ptr request(::gpiod_chip_request_lines(
					this->_m_priv->parent._m_priv->chip.get(),
					this->_m_priv->req_cfg._m_priv->config.get(),
					this->_m_priv->line_cfg._m_priv->config.get()));
	if (!request)
		throw_from_errno("error requesting GPIO lines");

	line_request ret;
	ret._m_priv.get()->set_request_ptr(request);

	return ret;
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const request_builder& builder)
{
	out << "gpiod::request_builder(request_config=" << builder._m_priv->req_cfg <<
	       ", line_config=" << builder._m_priv->line_cfg <<
	       ", parent=" << builder._m_priv->parent <<
	       ")";

	return out;
}

} /* namespace gpiod */
