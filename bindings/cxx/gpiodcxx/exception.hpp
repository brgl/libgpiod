/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file exception.hpp
 */

#ifndef __LIBGPIOD_CXX_EXCEPTION_HPP__
#define __LIBGPIOD_CXX_EXCEPTION_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <stdexcept>
#include <string>

namespace gpiod {

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Exception thrown when an already closed chip is used.
 */
class GPIOD_CXX_API chip_closed final : public ::std::logic_error
{
public:

	/**
	 * @brief Constructor.
	 * @param what Human readable reason for error.
	 */
	explicit chip_closed(const ::std::string& what);

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy from.
	 */
	chip_closed(const chip_closed& other) noexcept;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	chip_closed(chip_closed&& other) noexcept;

	/**
	 * @brief Assignment operator.
	 * @param other Object to copy from.
	 * @return Reference to self.
	 */
	chip_closed& operator=(const chip_closed& other) noexcept;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	chip_closed& operator=(chip_closed&& other) noexcept;

	~chip_closed();
};

/**
 * @brief Exception thrown when an already released line request is used.
 */
class GPIOD_CXX_API request_released final : public ::std::logic_error
{
public:

	/**
	 * @brief Constructor.
	 * @param what Human readable reason for error.
	 */
	explicit request_released(const ::std::string& what);

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy from.
	 */
	request_released(const request_released& other) noexcept;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	request_released(request_released&& other) noexcept;

	/**
	 * @brief Assignment operator.
	 * @param other Object to copy from.
	 * @return Reference to self.
	 */
	request_released& operator=(const request_released& other) noexcept;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	request_released& operator=(request_released&& other) noexcept;

	~request_released();
};

/**
 * @brief Exception thrown when the core C library returns an invalid value
 *        for any of the line_info properties.
 */
class GPIOD_CXX_API bad_mapping final : public ::std::runtime_error
{
public:

	/**
	 * @brief Constructor.
	 * @param what Human readable reason for error.
	 */
	explicit bad_mapping(const ::std::string& what);

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy from.
	 */
	bad_mapping(const bad_mapping& other) noexcept;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	bad_mapping(bad_mapping&& other) noexcept;

	/**
	 * @brief Assignment operator.
	 * @param other Object to copy from.
	 * @return Reference to self.
	 */
	bad_mapping& operator=(const bad_mapping& other) noexcept;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	bad_mapping& operator=(bad_mapping&& other) noexcept;

	~bad_mapping();
};

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_EXCEPTION_HPP__ */
