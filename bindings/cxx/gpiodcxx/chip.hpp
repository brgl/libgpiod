/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file chip.hpp
 */

#ifndef __LIBGPIOD_CXX_CHIP_HPP__
#define __LIBGPIOD_CXX_CHIP_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <chrono>
#include <cstddef>
#include <iostream>
#include <filesystem>
#include <memory>

#include "line.hpp"

namespace gpiod {

class chip_info;
class info_event;
class line_config;
class line_info;
class line_request;
class request_builder;
class request_config;

/**
 * @brief Represents a GPIO chip.
 */
class chip final
{
public:

	/**
	 * @brief Instantiates a new chip object by opening the device file
	 *        indicated by the \p path argument.
	 * @param path Path to the device file to open.
	 */
	explicit chip(const ::std::filesystem::path& path);

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	chip(chip&& other) noexcept;

	~chip();

	chip& operator=(const chip& other) = delete;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	chip& operator=(chip&& other) noexcept;

	/**
	 * @brief Check if this object is valid.
	 * @return True if this object's methods can be used, false otherwise.
	 *         False usually means the chip was closed. If the user calls
	 *         any of the methods of this class on an object for which this
	 *         operator returned false, a logic_error will be thrown.
	 */
	explicit operator bool() const noexcept;

	/**
	 * @brief Close the GPIO chip device file and free associated resources.
	 * @note The chip object can live after calling this method but any of
	 *       the chip's mutators will throw a logic_error exception.
	 */
	void close();

	/**
	 * @brief Get the filesystem path that was used to open this GPIO chip.
	 * @return Path to the underlying character device file.
	 */
	::std::filesystem::path path() const;

	/**
	 * @brief Get information about the chip.
	 * @return New chip_info object.
	 */
	chip_info get_info() const;

	/**
	 * @brief Retrieve the current snapshot of line information for a
	 *        single line.
	 * @param offset Offset of the line to get the info for.
	 * @return New ::gpiod::line_info object.
	 */
	line_info get_line_info(line::offset offset) const;

	/**
	 * @brief Wrapper around ::gpiod::chip::get_line_info that retrieves
	 *        the line info and starts watching the line for changes.
	 * @param offset Offset of the line to get the info for.
	 * @return New ::gpiod::line_info object.
	 */
	line_info watch_line_info(line::offset offset) const;

	/**
	 * @brief Stop watching the line at given offset for info events.
	 * @param offset Offset of the line to get the info for.
	 */
	void unwatch_line_info(line::offset offset) const;

	/**
	 * @brief Get the file descriptor associated with this chip.
	 * @return File descriptor number.
	 */
	int fd() const;

	/**
	 * @brief Wait for line status events on any of the watched lines
	 *        exposed by this chip.
	 * @param timeout Wait time limit in nanoseconds. If set to 0, the
	 *                function returns immediately. If set to a negative
	 *                number, the function blocks indefinitely until an
	 *                event becomes available.
	 * @return True if at least one event is ready to be read. False if the
	 *         wait timed out.
	 */
	bool wait_info_event(const ::std::chrono::nanoseconds& timeout) const;

	/**
	 * @brief Read a single line status change event from this chip.
	 * @return New info_event object.
	 */
	info_event read_info_event() const;

	/**
	 * @brief Map a GPIO line's name to its offset within the chip.
	 * @param name Name of the GPIO line to map.
	 * @return Offset of the line within the chip or -1 if the line with
	 *         given name is not exposed by this chip.
	 */
	int get_line_offset_from_name(const ::std::string& name) const;

	/**
	 * @brief Create a request_builder associated with this chip.
	 * @return New request_builder object.
	 */
	request_builder prepare_request();

private:

	struct impl;

	::std::shared_ptr<impl> _m_priv;

	chip(const chip& other);

	friend request_builder;
};

/**
 * @brief Stream insertion operator for GPIO chip objects.
 * @param out Output stream to write to.
 * @param chip GPIO chip to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const chip& chip);

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_CHIP_HPP__ */
