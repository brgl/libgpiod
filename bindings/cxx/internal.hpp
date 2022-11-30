/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __LIBGPIOD_CXX_INTERNAL_HPP__
#define __LIBGPIOD_CXX_INTERNAL_HPP__

#include <gpiod.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gpiod.hpp"

#define GPIOD_CXX_UNUSED	__attribute__((unused))
#define GPIOD_CXX_NORETURN	__attribute__((noreturn))

namespace gpiod {

template<class cxx_enum_type, class c_enum_type> cxx_enum_type
map_enum_c_to_cxx(c_enum_type value, const ::std::map<c_enum_type, cxx_enum_type>& mapping)
{
	try {
		return mapping.at(value);
	} catch (const ::std::out_of_range& err) {
		/* FIXME Demangle the name. */
		throw bad_mapping(::std::string("invalid value for ") +
				  typeid(cxx_enum_type).name());
	}
}

void throw_from_errno(const ::std::string& what);

template<class T, void F(T*)> struct deleter
{
	void operator()(T* ptr)
	{
		F(ptr);
	}
};

using chip_deleter = deleter<::gpiod_chip, ::gpiod_chip_close>;
using chip_info_deleter = deleter<::gpiod_chip_info, ::gpiod_chip_info_free>;
using line_info_deleter = deleter<::gpiod_line_info, ::gpiod_line_info_free>;
using info_event_deleter = deleter<::gpiod_info_event, ::gpiod_info_event_free>;
using line_settings_deleter = deleter<::gpiod_line_settings, ::gpiod_line_settings_free>;
using line_config_deleter = deleter<::gpiod_line_config, ::gpiod_line_config_free>;
using request_config_deleter = deleter<::gpiod_request_config, ::gpiod_request_config_free>;
using line_request_deleter = deleter<::gpiod_line_request, ::gpiod_line_request_release>;
using edge_event_deleter = deleter<::gpiod_edge_event, ::gpiod_edge_event_free>;
using edge_event_buffer_deleter = deleter<::gpiod_edge_event_buffer,
					  ::gpiod_edge_event_buffer_free>;

using chip_ptr = ::std::unique_ptr<::gpiod_chip, chip_deleter>;
using chip_info_ptr = ::std::unique_ptr<::gpiod_chip_info, chip_info_deleter>;
using line_info_ptr = ::std::unique_ptr<::gpiod_line_info, line_info_deleter>;
using info_event_ptr = ::std::unique_ptr<::gpiod_info_event, info_event_deleter>;
using line_settings_ptr = ::std::unique_ptr<::gpiod_line_settings, line_settings_deleter>;
using line_config_ptr = ::std::unique_ptr<::gpiod_line_config, line_config_deleter>;
using request_config_ptr = ::std::unique_ptr<::gpiod_request_config, request_config_deleter>;
using line_request_ptr = ::std::unique_ptr<::gpiod_line_request, line_request_deleter>;
using edge_event_ptr = ::std::unique_ptr<::gpiod_edge_event, edge_event_deleter>;
using edge_event_buffer_ptr = ::std::unique_ptr<::gpiod_edge_event_buffer,
						edge_event_buffer_deleter>;

struct chip::impl
{
	impl(const ::std::filesystem::path& path);
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	void throw_if_closed() const;

	chip_ptr chip;
};

struct chip_info::impl
{
	impl() = default;
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	void set_info_ptr(chip_info_ptr& new_info);

	chip_info_ptr info;
};

struct line_info::impl
{
	impl() = default;
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	void set_info_ptr(line_info_ptr& new_info);

	line_info_ptr info;
};

struct info_event::impl
{
	impl() = default;
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	void set_info_event_ptr(info_event_ptr& new_event);

	info_event_ptr event;
	line_info info;
};

struct line_settings::impl
{
	impl();
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	line_settings_ptr settings;
};

struct line_config::impl
{
	impl();
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	line_config_ptr config;
};

struct request_config::impl
{
	impl();
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	request_config_ptr config;
};

struct line_request::impl
{
	impl() = default;
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	void throw_if_released() const;
	void set_request_ptr(line_request_ptr& ptr);
	void fill_offset_buf(const line::offsets& offsets);

	line_request_ptr request;

	/*
	 * Used when reading/setting the line values in order to avoid
	 * allocating a new buffer on every call. We're not doing it for
	 * offsets in the line & request config structures because they don't
	 * require high performance unlike the set/get value calls.
	 */
	::std::vector<unsigned int> offset_buf;
};

struct edge_event::impl
{
	impl() = default;
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	virtual ~impl() = default;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	virtual ::gpiod_edge_event* get_event_ptr() const noexcept = 0;
	virtual ::std::shared_ptr<impl> copy(const ::std::shared_ptr<impl>& self) const = 0;
};

struct edge_event::impl_managed : public edge_event::impl
{
	impl_managed() = default;
	impl_managed(const impl_managed& other) = delete;
	impl_managed(impl_managed&& other) = delete;
	virtual ~impl_managed() = default;
	impl_managed& operator=(const impl_managed& other) = delete;
	impl_managed& operator=(impl_managed&& other) = delete;

	::gpiod_edge_event* get_event_ptr() const noexcept override;
	::std::shared_ptr<impl> copy(const ::std::shared_ptr<impl>& self) const override;

	edge_event_ptr event;
};

struct edge_event::impl_external : public edge_event::impl
{
	impl_external();
	impl_external(const impl_external& other) = delete;
	impl_external(impl_external&& other) = delete;
	virtual ~impl_external() = default;
	impl_external& operator=(const impl_external& other) = delete;
	impl_external& operator=(impl_external&& other) = delete;

	::gpiod_edge_event* get_event_ptr() const noexcept override;
	::std::shared_ptr<impl> copy(const ::std::shared_ptr<impl>& self) const override;

	::gpiod_edge_event *event;
};

struct edge_event_buffer::impl
{
	impl(unsigned int capacity);
	impl(const impl& other) = delete;
	impl(impl&& other) = delete;
	impl& operator=(const impl& other) = delete;
	impl& operator=(impl&& other) = delete;

	int read_events(const line_request_ptr& request, unsigned int max_events);

	edge_event_buffer_ptr buffer;
	::std::vector<edge_event> events;
};

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_INTERNAL_HPP__ */
