/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file line-info.hpp
 */

#ifndef __LIBGPIOD_CXX_LINE_INFO_HPP__
#define __LIBGPIOD_CXX_LINE_INFO_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

namespace gpiod {

class chip;
class info_event;

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Contains an immutable snapshot of the line's state at the
 *        time when the object of this class was instantiated.
 */
class line_info final
{
public:

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	line_info(const line_info& other) noexcept;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	line_info(line_info&& other) noexcept;

	~line_info();

	/**
	 * @brief Copy assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	line_info& operator=(const line_info& other) noexcept;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	line_info& operator=(line_info&& other) noexcept;

	/**
	 * @brief Get the hardware offset of the line.
	 * @return Offset of the line within the parent chip.
	 */
	line::offset offset() const noexcept;

	/**
	 * @brief Get the GPIO line name.
	 * @return Name of the GPIO line as it is represented in the kernel.
	 *         This routine returns an empty string if the line is unnamed.
	 */
	::std::string name() const noexcept;

	/**
	 * @brief Check if the line is currently in use.
	 * @return True if the line is in use, false otherwise.
	 *
	 * The user space can't know exactly why a line is busy. It may have
	 * been requested by another process or hogged by the kernel. It only
	 * matters that the line is used and we can't request it.
	 */
	bool used() const noexcept;

	/**
	 * @brief Read the GPIO line consumer name.
	 * @return Name of the GPIO consumer name as it is represented in the
	 *         kernel. This routine returns an empty string if the line is
	 *         not used.
	 */
	::std::string consumer() const noexcept;

	/**
	 * @brief Read the GPIO line direction setting.
	 * @return Returns DIRECTION_INPUT or DIRECTION_OUTPUT.
	 */
	line::direction direction() const;

	/**
	 * @brief Read the current edge detection setting of this line.
	 * @return Returns EDGE_NONE, EDGE_RISING, EDGE_FALLING or EDGE_BOTH.
	 */
	line::edge edge_detection() const;

	/**
	 * @brief Read the GPIO line bias setting.
	 * @return Returns BIAS_PULL_UP, BIAS_PULL_DOWN, BIAS_DISABLE or
	 *         BIAS_UNKNOWN.
	 */
	line::bias bias() const;

	/**
	 * @brief Read the GPIO line drive setting.
	 * @return Returns DRIVE_PUSH_PULL, DRIVE_OPEN_DRAIN or
	 *         DRIVE_OPEN_SOURCE.
	 */
	line::drive drive() const;

	/**
	 * @brief Check if the signal of this line is inverted.
	 * @return True if this line is "active-low", false otherwise.
	 */
	bool active_low() const noexcept;

	/**
	 * @brief Check if this line is debounced (either by hardware or by the
	 *        kernel software debouncer).
	 * @return True if the line is debounced, false otherwise.
	 */
	bool debounced() const noexcept;

	/**
	 * @brief Read the current debounce period in microseconds.
	 * @return Current debounce period in microseconds, 0 if the line is
	 *         not debounced.
	 */
	::std::chrono::microseconds debounce_period() const noexcept;

	/**
	 * @brief Read the current event clock setting used for edge event
	 *        timestamps.
	 * @return Returns MONOTONIC, REALTIME or HTE.
	 */
	line::clock event_clock() const;

private:

	line_info();

	struct impl;

	::std::shared_ptr<impl> _m_priv;

	friend chip;
	friend info_event;
};

/**
 * @brief Stream insertion operator for GPIO line info objects.
 * @param out Output stream to write to.
 * @param info GPIO line info object to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const line_info& info);

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_LINE_INFO_HPP__ */
