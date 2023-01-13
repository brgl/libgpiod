/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file request-config.hpp
 */

#ifndef __LIBGPIOD_CXX_LINE_SETTINGS_HPP__
#define __LIBGPIOD_CXX_LINE_SETTINGS_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <chrono>
#include <memory>

#include "line.hpp"

namespace gpiod {

class line_config;

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Stores GPIO line settings.
 */
class line_settings
{
public:

	/**
	 * @brief Initializes the line_settings object with default values.
	 */
	line_settings();

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	line_settings(const line_settings& other);

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	line_settings(line_settings&& other) noexcept;

	~line_settings();

	/**
	 * @brief Copy assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	line_settings& operator=(const line_settings& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	line_settings& operator=(line_settings&& other);

	/**
	 * @brief Reset the line settings to default values.
	 * @return Reference to self.
	 */
	line_settings& reset(void) noexcept;

	/**
	 * @brief Set direction.
	 * @param direction New direction.
	 * @return Reference to self.
	 */
	line_settings& set_direction(line::direction direction);

	/**
	 * @brief Get direction.
	 * @return Current direction setting.
	 */
	line::direction direction() const;

	/**
	 * @brief Set edge detection.
	 * @param edge New edge detection setting.
	 * @return Reference to self.
	 */
	line_settings& set_edge_detection(line::edge edge);

	/**
	 * @brief Get edge detection.
	 * @return Current edge detection setting.
	 */
	line::edge edge_detection() const;

	/**
	 * @brief Set bias setting.
	 * @param bias New bias.
	 * @return Reference to self.
	 */
	line_settings& set_bias(line::bias bias);

	/**
	 * @brief Get bias setting.
	 * @return Current bias.
	 */
	line::bias bias() const;

	/**
	 * @brief Set drive setting.
	 * @param drive New drive.
	 * @return Reference to self.
	 */
	line_settings& set_drive(line::drive drive);

	/**
	 * @brief Get drive setting.
	 * @return Current drive.
	 */
	line::drive drive() const;

	/**
	 * @brief Set the active-low setting.
	 * @param active_low New active-low setting.
	 * @return Reference to self.
	 */
	line_settings& set_active_low(bool active_low);

	/**
	 * @brief Get the active-low setting.
	 * @return Current active-low setting.
	 */
	bool active_low() const noexcept;

	/**
	 * @brief Set debounce period.
	 * @param period New debounce period in microseconds.
	 * @return Reference to self.
	 */
	line_settings& set_debounce_period(const ::std::chrono::microseconds& period);

	/**
	 * @brief Get debounce period.
	 * @return Current debounce period.
	 */
	::std::chrono::microseconds debounce_period() const noexcept;

	/**
	 * @brief Set the event clock to use for edge event timestamps.
	 * @param event_clock Clock to use.
	 * @return Reference to self.
	 */
	line_settings& set_event_clock(line::clock event_clock);

	/**
	 * @brief Get the event clock used for edge event timestamps.
	 * @return Current event clock type.
	 */
	line::clock event_clock() const;

	/**
	 * @brief Set the output value.
	 * @param value New output value.
	 * @return Reference to self.
	 */
	line_settings& set_output_value(line::value value);

	/**
	 * @brief Get the output value.
	 * @return Current output value.
	 */
	line::value output_value() const;

private:

	struct impl;

	::std::unique_ptr<impl> _m_priv;

	friend line_config;
};

/**
 * @brief Stream insertion operator for line settings.
 * @param out Output stream to write to.
 * @param settings Line settings object to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const line_settings& settings);

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_LINE_SETTINGS_HPP__ */
