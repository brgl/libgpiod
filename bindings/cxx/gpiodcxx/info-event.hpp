/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file gpiod.h
 */

#ifndef __LIBGPIOD_CXX_INFO_EVENT_HPP__
#define __LIBGPIOD_CXX_INFO_EVENT_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <cstdint>
#include <iostream>
#include <memory>

#include "timestamp.hpp"

namespace gpiod {

class chip;
class line_info;

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Immutable object containing data about a single line info event.
 */
class info_event final
{
public:

	/**
	 * @brief Types of info events.
	 */
	enum class event_type
	{
		LINE_REQUESTED = 1,
		/**< Line has been requested. */
		LINE_RELEASED,
		/**< Previously requested line has been released. */
		LINE_CONFIG_CHANGED,
		/**< Line configuration has changed. */
	};

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	info_event(const info_event& other);

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	info_event(info_event&& other) noexcept;

	~info_event();

	/**
	 * @brief Copy assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	info_event& operator=(const info_event& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	info_event& operator=(info_event&& other) noexcept;

	/**
	 * @brief Type of this event.
	 * @return Event type.
	 */
	event_type type() const;

	/**
	 * @brief Timestamp of the event as returned by the kernel.
	 * @return Timestamp as a 64-bit unsigned integer.
	 */
	::std::uint64_t timestamp_ns() const noexcept;

	/**
	 * @brief Get the new line information.
	 * @return Constant reference to the line info object containing the
	 *         line data as read at the time of the info event.
	 */
	const line_info& get_line_info() const noexcept;

private:

	info_event();

	struct impl;

	::std::shared_ptr<impl> _m_priv;

	friend chip;
};

/**
 * @brief Stream insertion operator for info events.
 * @param out Output stream to write to.
 * @param event GPIO line info event to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const info_event& event);

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_INFO_EVENT_HPP__ */
