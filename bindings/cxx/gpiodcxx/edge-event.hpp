/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file edge-event.hpp
 */

#ifndef __LIBGPIOD_CXX_EDGE_EVENT_HPP__
#define __LIBGPIOD_CXX_EDGE_EVENT_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <cstdint>
#include <iostream>
#include <memory>

#include "timestamp.hpp"

namespace gpiod {

class edge_event_buffer;

/**
 * @brief Immutable object containing data about a single edge event.
 */
class edge_event final
{
public:

	/**
	 * @brief Edge event types.
	 */
	enum class event_type
	{
		RISING_EDGE = 1,
		/**< This is a rising edge event. */
		FALLING_EDGE,
		/**< This is falling edge event. */
	};

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	edge_event(const edge_event& other);

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	edge_event(edge_event&& other) noexcept;

	~edge_event();

	/**
	 * @brief Copy assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	edge_event& operator=(const edge_event& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	edge_event& operator=(edge_event&& other) noexcept;

	/**
	 * @brief Retrieve the event type.
	 * @return Event type (rising or falling edge).
	 */
	event_type type() const;

	/**
	 * @brief Retrieve the event time-stamp.
	 * @return Time-stamp in nanoseconds as registered by the kernel using
	 *         the configured edge event clock.
	 */
	timestamp timestamp_ns() const noexcept;

	/**
	 * @brief Read the offset of the line on which this event was
	 *        registered.
	 * @return Line offset.
	 */
	line::offset line_offset() const noexcept;

	/**
	 * @brief Get the global sequence number of this event.
	 * @return Sequence number of the event relative to all lines in the
	 *         associated line request.
	 */
	unsigned long global_seqno() const noexcept;

	/**
	 * @brief Get the event sequence number specific to the concerned line.
	 * @return Sequence number of the event relative to this line within
	 *         the lifetime of the associated line request.
	 */
	unsigned long line_seqno() const noexcept;

private:

	edge_event();

	struct impl;
	struct impl_managed;
	struct impl_external;

	::std::shared_ptr<impl> _m_priv;

	friend edge_event_buffer;
};

/**
 * @brief Stream insertion operator for edge events.
 * @param out Output stream to write to.
 * @param event Edge event to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const edge_event& event);

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_EDGE_EVENT_HPP__ */
