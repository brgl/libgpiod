/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file timestamp.hpp
 */

#ifndef __LIBGPIOD_CXX_TIMESTAMP_HPP__
#define __LIBGPIOD_CXX_TIMESTAMP_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <chrono>
#include <cstdint>

namespace gpiod {

/**
 * @brief Stores the edge and info event timestamps as returned by the kernel
 *        and allows to convert them to std::chrono::time_point.
 */
class timestamp final
{
public:

	/**
	 * @brief Monotonic time_point.
	 */
	using time_point_monotonic = ::std::chrono::time_point<::std::chrono::steady_clock>;

	/**
	 * @brief Real-time time_point.
	 */
	using time_point_realtime = ::std::chrono::time_point<::std::chrono::system_clock,
							      ::std::chrono::nanoseconds>;

	/**
	 * @brief Constructor with implicit  conversion from `uint64_t`.
	 * @param ns Timestamp in nanoseconds.
	 */
	timestamp(::std::uint64_t ns) : _m_ns(ns) { }

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	timestamp(const timestamp& other) noexcept = default;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	timestamp(timestamp&& other) noexcept = default;

	/**
	 * @brief Assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	timestamp& operator=(const timestamp& other) noexcept = default;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	timestamp& operator=(timestamp&& other) noexcept = default;

	~timestamp() = default;

	/**
	 * @brief Conversion operator to `std::uint64_t`.
	 */
	operator ::std::uint64_t() noexcept
	{
		return this->ns();
	}

	/**
	 * @brief Get the timestamp in nanoseconds.
	 * @return Timestamp in nanoseconds.
	 */
	::std::uint64_t ns() const noexcept
	{
		return this->_m_ns;
	}

	/**
	 * @brief Convert the timestamp to a monotonic time_point.
	 * @return time_point associated with the steady clock.
	 */
	time_point_monotonic to_time_point_monotonic() const
	{
		return time_point_monotonic(::std::chrono::nanoseconds(this->ns()));
	}

	/**
	 * @brief Convert the timestamp to a real-time time_point.
	 * @return time_point associated with the system clock.
	 */
	time_point_realtime to_time_point_realtime() const
	{
		return time_point_realtime(::std::chrono::nanoseconds(this->ns()));
	}

private:
	::std::uint64_t _m_ns;
};

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_TIMESTAMP_HPP__ */
