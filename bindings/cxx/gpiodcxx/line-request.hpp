/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file line-request.hpp
 */

#ifndef __LIBGPIOD_CXX_LINE_REQUEST_HPP__
#define __LIBGPIOD_CXX_LINE_REQUEST_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>

#include "misc.hpp"

namespace gpiod {

class chip;
class edge_event;
class edge_event_buffer;
class line_config;

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Stores the context of a set of requested GPIO lines.
 */
class line_request final
{
public:

	line_request(const line_request& other) = delete;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	line_request(line_request&& other) noexcept;

	~line_request();

	line_request& operator=(const line_request& other) = delete;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	line_request& operator=(line_request&& other) noexcept;

	/**
	 * @brief Check if this object is valid.
	 * @return True if this object's methods can be used, false otherwise.
	 *         False usually means the request was released. If the user
	 *         calls any of the methods of this class on an object for
	 *         which this operator returned false, a logic_error will be
	 *         thrown.
	 */
	explicit operator bool() const noexcept;

	/**
	 * @brief Release the GPIO chip and free all associated resources.
	 * @note The object can still be used after this method is called but
	 *       using any of the mutators will result in throwing
	 *       a logic_error exception.
	 */
	void release();

	/**
	 * @brief Get the number of requested lines.
	 * @return Number of lines in this request.
	 */
	::std::size_t num_lines() const;

	/**
	 * @brief Get the list of offsets of requested lines.
	 * @return List of hardware offsets of the lines in this request.
	 */
	line::offsets offsets() const;

	/**
	 * @brief Get the value of a single requested line.
	 * @param offset Offset of the line to read within the chip.
	 * @return Current line value.
	 */
	line::value get_value(line::offset offset);

	/**
	 * @brief Get the values of a subset of requested lines.
	 * @param offsets Vector of line offsets
	 * @return Vector of lines values with indexes of values corresponding
	 *         to those of the offsets.
	 */
	line::values get_values(const line::offsets& offsets);

	/**
	 * @brief Get the values of all requested lines.
	 * @return List of read values.
	 */
	line::values get_values();

	/**
	 * @brief Get the values of a subset of requested lines into a vector
	 *        supplied by the caller.
	 * @param offsets Vector of line offsets.
	 * @param values Vector for storing the values. Its size must be at
	 *               least that of the offsets vector. The indexes of read
	 *               values will correspond with those in the offsets
	 *               vector.
	 */
	void get_values(const line::offsets& offsets, line::values& values);

	/**
	 * @brief Get the values of all requested lines.
	 * @param values Array in which the values will be stored. Must hold
	 *               at least the number of elements returned by
	 *               line_request::num_lines.
	 */
	void get_values(line::values& values);

	/**
	 * @brief Set the value of a single requested line.
	 * @param offset Offset of the line to set within the chip.
	 * @param value New line value.
	 * @return Reference to self.
	 */
	line_request& set_value(line::offset offset, line::value value);

	/**
	 * @brief Set the values of a subset of requested lines.
	 * @param values Vector containing a set of offset->value mappings.
	 * @return Reference to self.
	 */
	line_request& set_values(const line::value_mappings& values);

	/**
	 * @brief Set the values of a subset of requested lines.
	 * @param offsets Vector containing the offsets of lines to set.
	 * @param values Vector containing new values with indexes
	 *               corresponding with those in the offsets vector.
	 * @return Reference to self.
	 */
	line_request& set_values(const line::offsets& offsets, const line::values& values);

	/**
	 * @brief Set the values of all requested lines.
	 * @param values Array of new line values. The size must be equal to
	 *               the value returned by line_request::num_lines.
	 * @return Reference to self.
	 */
	line_request& set_values(const line::values& values);

	/**
	 * @brief Apply new config options to requested lines.
	 * @param config New configuration.
	 * @return Reference to self.
	 */
	line_request& reconfigure_lines(const line_config& config);

	/**
	 * @brief Get the file descriptor number associated with this line
	 *        request.
	 * @return File descriptor number.
	 */
	int fd() const;

	/**
	 * @brief Wait for edge events on any of the lines requested with edge
	 *        detection enabled.
	 * @param timeout Wait time limit in nanoseconds.
	 * @return True if at least one event is ready to be read. False if the
	 *         wait timed out.
	 */
	bool wait_edge_events(const ::std::chrono::nanoseconds& timeout) const;

	/**
	 * @brief Read a number of edge events from this request up to the
	 *        maximum capacity of the buffer.
	 * @param buffer Edge event buffer to read events into.
	 * @return Number of events read.
	 */
	::std::size_t read_edge_events(edge_event_buffer& buffer);

	/**
	 * @brief Read a number of edge events from this request.
	 * @param buffer Edge event buffer to read events into.
	 * @param max_events Maximum number of events to read. Limited by the
	 *                   capacity of the buffer.
	 * @return Number of events read.
	 */
	::std::size_t read_edge_events(edge_event_buffer& buffer, ::std::size_t max_events);

private:

	line_request();

	struct impl;

	::std::unique_ptr<impl> _m_priv;

	friend request_builder;
};

/**
 * @brief Stream insertion operator for line requests.
 * @param out Output stream to write to.
 * @param request Line request object to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const line_request& request);

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_LINE_REQUEST_HPP__ */
