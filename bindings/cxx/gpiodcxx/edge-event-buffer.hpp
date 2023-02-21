/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file edge-event-buffer.hpp
 */

#ifndef __LIBGPIOD_CXX_EDGE_EVENT_BUFFER_HPP__
#define __LIBGPIOD_CXX_EDGE_EVENT_BUFFER_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>

namespace gpiod {

class edge_event;
class line_request;

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Object into which edge events are read for better performance.
 *
 * The edge_event_buffer allows reading edge_event objects into an existing
 * buffer which improves the performance by avoiding needless memory
 * allocations.
 */
class edge_event_buffer final
{
public:

	/**
	 * @brief Constant iterator for iterating over edge events stored in
	 *        the buffer.
	 */
	using const_iterator = ::std::vector<edge_event>::const_iterator;

	/**
	 * @brief Constructor. Creates a new edge event buffer with given
	 *        capacity.
	 * @param capacity Capacity of the new buffer.
	 */
	explicit edge_event_buffer(::std::size_t capacity = 64);

	edge_event_buffer(const edge_event_buffer& other) = delete;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	edge_event_buffer(edge_event_buffer&& other) noexcept;

	~edge_event_buffer();

	edge_event_buffer& operator=(const edge_event_buffer& other) = delete;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	edge_event_buffer& operator=(edge_event_buffer&& other) noexcept;

	/**
	 * @brief Get the constant reference to the edge event at given index.
	 * @param index Index of the event in the buffer.
	 * @return Constant reference to the edge event.
	 */
	const edge_event& get_event(unsigned int index) const;

	/**
	 * @brief Get the number of edge events currently stored in the buffer.
	 * @return Number of edge events in the buffer.
	 */
	::std::size_t num_events() const;

	/**
	 * @brief Maximum capacity of the buffer.
	 * @return Buffer capacity.
	 */
	::std::size_t capacity() const noexcept;

	/**
	 * @brief Get a constant iterator to the first edge event currently
	 *        stored in the buffer.
	 * @return Constant iterator to the first element.
	 */
	const_iterator begin() const noexcept;

	/**
	 * @brief Get a constant iterator to the element after the last edge
	 *        event in the buffer.
	 * @return Constant iterator to the element after the last edge event.
	 */
	const_iterator end() const noexcept;

private:

	struct impl;

	::std::unique_ptr<impl> _m_priv;

	friend line_request;
};

/**
 * @brief Stream insertion operator for GPIO edge event buffer objects.
 * @param out Output stream to write to.
 * @param buf GPIO edge event buffer object to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const edge_event_buffer& buf);

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_EDGE_EVENT_BUFFER_HPP__ */
