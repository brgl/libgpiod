/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file line-config.hpp
 */

#ifndef __LIBGPIOD_CXX_LINE_CONFIG_HPP__
#define __LIBGPIOD_CXX_LINE_CONFIG_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <map>
#include <memory>

namespace gpiod {

class chip;
class line_request;
class line_settings;

/**
 * @brief Contains a set of line config options used in line requests and
 *        reconfiguration.
 */
class line_config final
{
public:


	line_config();

	line_config(const line_config& other) = delete;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	line_config(line_config&& other) noexcept;

	~line_config();

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	line_config& operator=(line_config&& other) noexcept;

	/**
	 * @brief Reset the line config object.
	 * @return Reference to self.
	 */
	line_config& reset() noexcept;

	/**
	 * @brief Add line settings for a single offset.
	 * @param offset Offset for which to add settings.
	 * @param settings Line settings to add.
	 * @return Reference to self.
	 */
	line_config& add_line_settings(line::offset offset, const line_settings& settings);

	/**
	 * @brief Add line settings for a set of offsets.
	 * @param offsets Offsets for which to add settings.
	 * @param settings Line settings to add.
	 * @return Reference to self.
	 */
	line_config& add_line_settings(const line::offsets& offsets, const line_settings& settings);

	/**
	 * @brief Set output values for a number of lines.
	 * @param values Buffer containing the output values.
	 * @return Reference to self.
	 */
	line_config& set_output_values(const line::values& values);

	/**
	 * @brief Get a mapping of offsets to line settings stored by this
	 *        object.
	 * @return Map in which keys represent line offsets and values are
	 *         the settings corresponding with them.
	 */
	::std::map<line::offset, line_settings> get_line_settings() const;

private:

	struct impl;

	::std::shared_ptr<impl> _m_priv;

	line_config& operator=(const line_config& other);

	friend line_request;
	friend request_builder;
};

/**
 * @brief Stream insertion operator for GPIO line config objects.
 * @param out Output stream to write to.
 * @param config Line config object to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const line_config& config);

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_LINE_CONFIG_HPP__ */
