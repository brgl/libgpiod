// SPDX-License-Identifier: LGPL-3.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <ostream>
#include <utility>

#include "internal.hpp"

namespace gpiod {

namespace {

chip_ptr open_chip(const ::std::filesystem::path& path)
{
	chip_ptr chip(::gpiod_chip_open(path.c_str()));
	if (!chip)
		throw_from_errno("unable to open the GPIO device " + path.string());

	return chip;
}

} /* namespace */

chip::impl::impl(const ::std::filesystem::path& path)
	: chip(open_chip(path))
{

}

void chip::impl::throw_if_closed() const
{
	if (!this->chip)
		throw chip_closed("GPIO chip has been closed");
}

GPIOD_CXX_API chip::chip(const ::std::filesystem::path& path)
	: _m_priv(new impl(path))
{

}

chip::chip(const chip& other)
	: _m_priv(other._m_priv)
{

}

GPIOD_CXX_API chip::chip(chip&& other) noexcept
	: _m_priv(::std::move(other._m_priv))
{

}

GPIOD_CXX_API chip::~chip()
{

}

GPIOD_CXX_API chip& chip::operator=(chip&& other) noexcept
{
	this->_m_priv = ::std::move(other._m_priv);

	return *this;
}

GPIOD_CXX_API chip::operator bool() const noexcept
{
	return this->_m_priv->chip.get() != nullptr;
}

GPIOD_CXX_API void chip::close()
{
	this->_m_priv->throw_if_closed();

	this->_m_priv->chip.reset();
}

GPIOD_CXX_API ::std::filesystem::path chip::path() const
{
	this->_m_priv->throw_if_closed();

	return ::gpiod_chip_get_path(this->_m_priv->chip.get());
}

GPIOD_CXX_API chip_info chip::get_info() const
{
	this->_m_priv->throw_if_closed();

	chip_info_ptr info(::gpiod_chip_get_info(this->_m_priv->chip.get()));
	if (!info)
		throw_from_errno("failed to retrieve GPIO chip info");

	chip_info ret;

	ret._m_priv->set_info_ptr(info);

	return ret;
}

GPIOD_CXX_API line_info chip::get_line_info(line::offset offset) const
{
	this->_m_priv->throw_if_closed();

	line_info_ptr info(::gpiod_chip_get_line_info(this->_m_priv->chip.get(), offset));
	if (!info)
		throw_from_errno("unable to retrieve GPIO line info");

	line_info ret;

	ret._m_priv->set_info_ptr(info);

	return ret;
}

GPIOD_CXX_API line_info chip::watch_line_info(line::offset offset) const
{
	this->_m_priv->throw_if_closed();

	line_info_ptr info(::gpiod_chip_watch_line_info(this->_m_priv->chip.get(), offset));
	if (!info)
		throw_from_errno("unable to start watching GPIO line info changes");

	line_info ret;

	ret._m_priv->set_info_ptr(info);

	return ret;
}

GPIOD_CXX_API void chip::unwatch_line_info(line::offset offset) const
{
	this->_m_priv->throw_if_closed();

	int ret = ::gpiod_chip_unwatch_line_info(this->_m_priv->chip.get(), offset);
	if (ret)
		throw_from_errno("unable to unwatch line status changes");
}

GPIOD_CXX_API int chip::fd() const
{
	this->_m_priv->throw_if_closed();

	return ::gpiod_chip_get_fd(this->_m_priv->chip.get());
}

GPIOD_CXX_API bool chip::wait_info_event(const ::std::chrono::nanoseconds& timeout) const
{
	this->_m_priv->throw_if_closed();

	int ret = ::gpiod_chip_wait_info_event(this->_m_priv->chip.get(), timeout.count());
	if (ret < 0)
		throw_from_errno("error waiting for info events");

	return ret;
}

GPIOD_CXX_API info_event chip::read_info_event() const
{
	this->_m_priv->throw_if_closed();

	info_event_ptr event(gpiod_chip_read_info_event(this->_m_priv->chip.get()));
	if (!event)
		throw_from_errno("error reading the line info event_handle");

	info_event ret;
	ret._m_priv->set_info_event_ptr(event);

	return ret;
}

GPIOD_CXX_API int chip::get_line_offset_from_name(const ::std::string& name) const
{
	this->_m_priv->throw_if_closed();

	int ret = ::gpiod_chip_get_line_offset_from_name(this->_m_priv->chip.get(), name.c_str());
	if (ret < 0) {
		if (errno == ENOENT)
			return -1;

		throw_from_errno("error looking up line by name");
	}

	return ret;
}

GPIOD_CXX_API request_builder chip::prepare_request()
{
	return request_builder(*this);
}

GPIOD_CXX_API ::std::ostream& operator<<(::std::ostream& out, const chip& chip)
{
	if (!chip)
		out << "gpiod::chip(closed)";
	else
		out << "gpiod::chip(path=" << chip.path() <<
		       ", info=" << chip.get_info() << ")";

	return out;
}

} /* namespace gpiod */
