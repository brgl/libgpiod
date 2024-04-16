// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <iterator>
#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

void line_request::impl::throw_if_released() const
{
	if (!this->request)
		throw request_released("GPIO lines have been released");
}

void line_request::impl::set_request_ptr(line_request_ptr& ptr)
{
	this->request = ::std::move(ptr);
	this->offset_buf.resize(::gpiod_line_request_get_num_requested_lines(this->request.get()));
}

void line_request::impl::fill_offset_buf(const line::offsets& offsets)
{
	for (unsigned int i = 0; i < offsets.size(); i++)
		this->offset_buf[i] = offsets[i];
}

line_request::line_request()
	: _m_priv(new impl)
{

}

GPIOD_CXX_API line_request::line_request(line_request&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API line_request::~line_request()
{

}

GPIOD_CXX_API line_request& line_request::operator=(line_request&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API line_request::operator bool() const noexcept
{
	return this->_m_priv->request.get() != nullptr;
}

GPIOD_CXX_API void line_request::release()
{
	this->_m_priv->throw_if_released();

	this->_m_priv->request.reset();
}

GPIOD_CXX_API ::std::string line_request::chip_name() const
{
	this->_m_priv->throw_if_released();

	return ::gpiod_line_request_get_chip_name(this->_m_priv->request.get());
}

GPIOD_CXX_API ::std::size_t line_request::num_lines() const
{
	this->_m_priv->throw_if_released();

	return ::gpiod_line_request_get_num_requested_lines(this->_m_priv->request.get());
}

GPIOD_CXX_API line::offsets line_request::offsets() const
{
	this->_m_priv->throw_if_released();

	auto num_lines = this->num_lines();
	::std::vector<unsigned int> buf(num_lines);
	line::offsets offsets(num_lines);

	::gpiod_line_request_get_requested_offsets(this->_m_priv->request.get(), buf.data(), buf.size());

	for (unsigned int i = 0; i < num_lines; i++)
		offsets[i] = buf[i];

	return offsets;
}

GPIOD_CXX_API line::value line_request::get_value(line::offset offset)
{
	return this->get_values({ offset }).front();
}

GPIOD_CXX_API line::values
line_request::get_values(const line::offsets& offsets)
{
	line::values vals(offsets.size());

	this->get_values(offsets, vals);

	return vals;
}

GPIOD_CXX_API line::values line_request::get_values()
{
	return this->get_values(this->offsets());
}

GPIOD_CXX_API void line_request::get_values(const line::offsets& offsets, line::values& values)
{
	this->_m_priv->throw_if_released();

	if (offsets.size() != values.size())
		throw ::std::invalid_argument("values must have the same size as the offsets");

	this->_m_priv->fill_offset_buf(offsets);

	int ret = ::gpiod_line_request_get_values_subset(
					this->_m_priv->request.get(),
					offsets.size(), this->_m_priv->offset_buf.data(),
					reinterpret_cast<::gpiod_line_value*>(values.data()));
	if (ret)
		throw_from_errno("unable to retrieve line values");
}

GPIOD_CXX_API void line_request::get_values(line::values& values)
{
	this->get_values(this->offsets(), values);
}

GPIOD_CXX_API line_request&
line_request::line_request::set_value(line::offset offset, line::value value)
{
	return this->set_values({ offset }, { value });
}

GPIOD_CXX_API line_request&
line_request::set_values(const line::value_mappings& values)
{
	line::offsets offsets(values.size());
	line::values vals(values.size());

	for (unsigned int i = 0; i < values.size(); i++) {
		offsets[i] = values[i].first;
		vals[i] = values[i].second;
	}

	return this->set_values(offsets, vals);
}

GPIOD_CXX_API line_request& line_request::set_values(const line::offsets& offsets,
					    const line::values& values)
{
	this->_m_priv->throw_if_released();

	if (offsets.size() != values.size())
		throw ::std::invalid_argument("values must have the same size as the offsets");

	this->_m_priv->fill_offset_buf(offsets);

	int ret = ::gpiod_line_request_set_values_subset(
					this->_m_priv->request.get(),
					offsets.size(), this->_m_priv->offset_buf.data(),
					reinterpret_cast<const ::gpiod_line_value*>(values.data()));
	if (ret)
		throw_from_errno("unable to set line values");

	return *this;
}

GPIOD_CXX_API line_request& line_request::set_values(const line::values& values)
{
	return this->set_values(this->offsets(), values);
}

GPIOD_CXX_API line_request& line_request::reconfigure_lines(const line_config& config)
{
	this->_m_priv->throw_if_released();

	int ret = ::gpiod_line_request_reconfigure_lines(this->_m_priv->request.get(),
							 config._m_priv->config.get());
	if (ret)
		throw_from_errno("unable to reconfigure GPIO lines");

	return *this;
}

GPIOD_CXX_API int line_request::fd() const
{
	this->_m_priv->throw_if_released();

	return ::gpiod_line_request_get_fd(this->_m_priv->request.get());
}

GPIOD_CXX_API bool line_request::wait_edge_events(const ::std::chrono::nanoseconds& timeout) const
{
	this->_m_priv->throw_if_released();

	int ret = ::gpiod_line_request_wait_edge_events(this->_m_priv->request.get(),
						       timeout.count());
	if (ret < 0)
		throw_from_errno("error waiting for edge events");

	return ret;
}

GPIOD_CXX_API ::std::size_t line_request::read_edge_events(edge_event_buffer& buffer)
{
	return this->read_edge_events(buffer, buffer.capacity());
}

GPIOD_CXX_API ::std::size_t
line_request::read_edge_events(edge_event_buffer& buffer, ::std::size_t max_events)
{
	this->_m_priv->throw_if_released();

	return buffer._m_priv->read_events(this->_m_priv->request, max_events);
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const line_request& request)
{
	if (!request)
		out << "gpiod::line_request(released)";
	else
		out << "gpiod::line_request(chip=\"" << request.chip_name() <<
		       "\", num_lines=" << request.num_lines() <<
		       ", line_offsets=" << request.offsets() <<
		       ", fd=" << request.fd() <<
		       ")";

	return out;
}

} /* namespace gpiod */
