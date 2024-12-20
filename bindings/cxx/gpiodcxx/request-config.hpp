/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file request-config.hpp
 */

#ifndef __LIBGPIOD_CXX_REQUEST_CONFIG_HPP__
#define __LIBGPIOD_CXX_REQUEST_CONFIG_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

#include "line.hpp"

namespace gpiod {

class chip;

/**
 * @brief Stores a set of options passed to the kernel when making a line
 *        request.
 */
class request_config final
{
public:

	/**
	 * @brief Constructor.
	 */
	request_config();

	request_config(const request_config& other) = delete;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	request_config(request_config&& other) noexcept;

	~request_config();

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	request_config& operator=(request_config&& other) noexcept;

	/**
	 * @brief Set the consumer name.
	 * @param consumer New consumer name.
	 * @return Reference to self.
	 */
	request_config& set_consumer(const ::std::string& consumer) noexcept;

	/**
	 * @brief Get the consumer name.
	 * @return Currently configured consumer name. May be an empty string.
	 */
	::std::string consumer() const noexcept;

	/**
	 * @brief Set the size of the kernel event buffer.
	 * @param event_buffer_size New event buffer size.
	 * @return Reference to self.
	 * @note The kernel may adjust the value if it's too high. If set to 0,
	 *       the default value will be used.
	 */
	request_config& set_event_buffer_size(::std::size_t event_buffer_size) noexcept;

	/**
	 * @brief Get the edge event buffer size from this request config.
	 * @return Current edge event buffer size setting.
	 */
	::std::size_t event_buffer_size() const noexcept;

private:

	struct impl;

	::std::shared_ptr<impl> _m_priv;

	request_config& operator=(const request_config& other);

	friend request_builder;
};

/**
 * @brief Stream insertion operator for request_config objects.
 * @param out Output stream to write to.
 * @param config request_config to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const request_config& config);

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_REQUEST_CONFIG_HPP__ */
