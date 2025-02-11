/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file request-builder.hpp
 */

#ifndef __LIBGPIOD_CXX_REQUEST_BUILDER_HPP__
#define __LIBGPIOD_CXX_REQUEST_BUILDER_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <memory>
#include <ostream>

namespace gpiod {

class chip;
class line_config;
class line_request;
class request_config;

/**
 * @brief Intermediate object storing the configuration for a line request.
 */
class request_builder final
{
public:

	request_builder(const request_builder& other) = delete;

	/**
	 * @brief Move constructor.
	 * @param other Object to be moved.
	 */
	request_builder(request_builder&& other) noexcept;

	~request_builder();

	request_builder& operator=(const request_builder& other) = delete;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to be moved.
	 * @return Reference to self.
	 */
	request_builder& operator=(request_builder&& other) noexcept;

	/**
	 * @brief Set the request config for the request.
	 * @param req_cfg Request config to use.
	 * @return Reference to self.
	 */
	request_builder& set_request_config(request_config& req_cfg);

	/**
	 * @brief Get the current request config.
	 * @return Const reference to the current request config stored by this
	 *         object.
	 */
	const request_config& get_request_config() const noexcept;

	/**
	 * @brief Set consumer in the request config stored by this object.
	 * @param consumer New consumer string.
	 * @return Reference to self.
	 */
	request_builder& set_consumer(const ::std::string& consumer) noexcept;

	/**
	 * @brief Set the event buffer size in the request config stored by
	 *        this object.
	 * @param event_buffer_size New event buffer size.
	 * @return Reference to self.
	 */
	request_builder& set_event_buffer_size(::std::size_t event_buffer_size) noexcept;

	/**
	 * @brief Set the line config for this request.
	 * @param line_cfg Line config to use.
	 * @return Reference to self.
	 */
	request_builder& set_line_config(line_config &line_cfg);

	/**
	 * @brief Get the current line config.
	 * @return Const reference to the current line config stored by this
	 *         object.
	 */
	const line_config& get_line_config() const noexcept;

	/**
	 * @brief Add line settings to the line config stored by this object
	 *        for a single offset.
	 * @param offset Offset for which to add settings.
	 * @param settings Line settings to use.
	 * @return Reference to self.
	 */
	request_builder& add_line_settings(line::offset offset, const line_settings& settings);

	/**
	 * @brief Add line settings to the line config stored by this object
	 *        for a set of offsets.
	 * @param offsets Offsets for which to add settings.
	 * @param settings Settings to add.
	 * @return Reference to self.
	 */
	request_builder& add_line_settings(const line::offsets& offsets, const line_settings& settings);

	/**
	 * @brief Set output values for a number of lines in the line config
	 *        stored by this object.
	 * @param values Buffer containing the output values.
	 * @return Reference to self.
	 */
	request_builder& set_output_values(const line::values& values);

	/**
	 * @brief Make the line request.
	 * @return New line_request object.
	 */
	line_request do_request();

private:

	struct impl;

	request_builder(chip& chip);

	::std::unique_ptr<impl> _m_priv;

	friend chip;
	friend ::std::ostream& operator<<(::std::ostream& out, const request_builder& builder);
};

/**
 * @brief Stream insertion operator for GPIO request builder objects.
 * @param out Output stream to write to.
 * @param builder Request builder object to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const request_builder& builder);

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_REQUEST_BUILDER_HPP__ */
