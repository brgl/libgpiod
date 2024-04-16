// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021 Bartosz Golaszewski <brgl@bgdev.pl>

#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

namespace {

request_config_ptr make_request_config()
{
	request_config_ptr config(::gpiod_request_config_new());
	if (!config)
		throw_from_errno("Unable to allocate the request config object");

	return config;
}

} /* namespace */

request_config::impl::impl()
	: config(make_request_config())
{

}

GPIOD_CXX_API request_config::request_config()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API request_config::request_config(request_config&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API request_config::~request_config()
{

}

request_config& request_config::operator=(const request_config& other)
{
	this->_m_priv = other._m_priv;

	return *this;
}

GPIOD_CXX_API request_config& request_config::operator=(request_config&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API request_config&
request_config::set_consumer(const ::std::string& consumer) noexcept
{
	::gpiod_request_config_set_consumer(this->_m_priv->config.get(), consumer.c_str());

	return *this;
}

GPIOD_CXX_API ::std::string request_config::consumer() const noexcept
{
	const char* consumer = ::gpiod_request_config_get_consumer(this->_m_priv->config.get());

	return consumer ?: "";
}

GPIOD_CXX_API request_config&
request_config::set_event_buffer_size(::std::size_t event_buffer_size) noexcept
{
	::gpiod_request_config_set_event_buffer_size(this->_m_priv->config.get(),
						     event_buffer_size);

	return *this;
}

GPIOD_CXX_API ::std::size_t request_config::event_buffer_size() const noexcept
{
	return ::gpiod_request_config_get_event_buffer_size(this->_m_priv->config.get());
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const request_config& config)
{
	::std::string consumer;

	consumer = config.consumer().empty() ? "N/A" : ::std::string("'") + config.consumer() + "'";

	out << "gpiod::request_config(consumer=" << consumer <<
	       ", event_buffer_size=" << config.event_buffer_size() <<
	       ")";

	return out;
}

} /* namespace gpiod */
