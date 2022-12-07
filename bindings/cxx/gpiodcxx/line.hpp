/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file line.hpp
 */

#ifndef __LIBGPIOD_CXX_LINE_HPP__
#define __LIBGPIOD_CXX_LINE_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <ostream>
#include <utility>
#include <vector>

namespace gpiod {

/**
 * @brief Namespace containing various type definitions for GPIO lines.
 */
namespace line {

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Wrapper around unsigned int for representing line offsets.
 */
class offset
{
public:
	/**
	 * @brief Constructor with implicit conversion from unsigned int.
	 * @param off Line offset.
	 */
	offset(unsigned int off = 0) : _m_offset(off) {	}

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	offset(const offset& other) = default;

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	offset(offset&& other) = default;

	~offset() = default;

	/**
	 * @brief Assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	offset& operator=(const offset& other) = default;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	offset& operator=(offset&& other) noexcept = default;

	/**
	 * @brief Conversion operator to `unsigned int`.
	 */
	operator unsigned int() const noexcept
	{
		return this->_m_offset;
	}

private:
	unsigned int _m_offset;
};

/**
 * @brief Logical line states.
 */
enum class value
{
	INACTIVE = 0,
	/**< Line is inactive. */
	ACTIVE = 1,
	/**< Line is active. */
};

/**
 * @brief Direction settings.
 */
enum class direction
{
	AS_IS = 1,
	/**< Request the line(s), but don't change current direction. */
	INPUT,
	/**< Direction is input - we're reading the state of a GPIO line. */
	OUTPUT,
	/**< Direction is output - we're driving the GPIO line. */
};

/**
 * @brief Edge detection settings.
 */
enum class edge
{
	NONE = 1,
	/**< Line edge detection is disabled. */
	RISING,
	/**< Line detects rising edge events. */
	FALLING,
	/**< Line detect falling edge events. */
	BOTH,
	/**< Line detects both rising and falling edge events. */
};

/**
 * @brief Internal bias settings.
 */
enum class bias
{
	AS_IS = 1,
	/**< Don't change the bias setting when applying line config. */
	UNKNOWN,
	/**< The internal bias state is unknown. */
	DISABLED,
	/**< The internal bias is disabled. */
	PULL_UP,
	/**< The internal pull-up bias is enabled. */
	PULL_DOWN,
	/**< The internal pull-down bias is enabled. */
};

/**
 * @brief Drive settings.
 */
enum class drive
{
	PUSH_PULL = 1,
	/**< Drive setting is push-pull. */
	OPEN_DRAIN,
	/**< Line output is open-drain. */
	OPEN_SOURCE,
	/**< Line output is open-source. */
};

/**
 * @brief Event clock settings.
 */
enum class clock
{
	MONOTONIC = 1,
	/**< Line uses the monotonic clock for edge event timestamps. */
	REALTIME,
	/**< Line uses the realtime clock for edge event timestamps. */
	HTE,
	/*<< Line uses the hardware timestamp engine for event timestamps. */
};

/**
 * @brief Vector of line offsets.
 */
using offsets = ::std::vector<offset>;

/**
 * @brief Vector of line values.
 */
using values = ::std::vector<value>;

/**
 * @brief Represents a mapping of a line offset to line logical state.
 */
using value_mapping = ::std::pair<offset, value>;

/**
 * @brief Vector of offset->value mappings. Each mapping is defined as a pair
 *        of an unsigned and signed integers.
 */
using value_mappings = ::std::vector<value_mapping>;

/**
 * @brief Stream insertion operator for logical line values.
 * @param out Output stream.
 * @param val Value to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, value val);

/**
 * @brief Stream insertion operator for direction values.
 * @param out Output stream.
 * @param dir Value to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, direction dir);

/**
 * @brief Stream insertion operator for edge detection values.
 * @param out Output stream.
 * @param edge Value to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, edge edge);

/**
 * @brief Stream insertion operator for bias values.
 * @param out Output stream.
 * @param bias Value to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, bias bias);

/**
 * @brief Stream insertion operator for drive values.
 * @param out Output stream.
 * @param drive Value to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, drive drive);

/**
 * @brief Stream insertion operator for event clock values.
 * @param out Output stream.
 * @param clock Value to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, clock clock);

/**
 * @brief Stream insertion operator for the list of output values.
 * @param out Output stream.
 * @param vals Object to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const values& vals);

/**
 * @brief Stream insertion operator for the list of line offsets.
 * @param out Output stream.
 * @param offs Object to insert into the output stream in a human-readable form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const offsets& offs);

/**
 * @brief Stream insertion operator for the offset-to-value mapping.
 * @param out Output stream.
 * @param mapping Value to insert into the output stream in a human-readable
 *        form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const value_mapping& mapping);

/**
 * @brief Stream insertion operator for the list of offset-to-value mappings.
 * @param out Output stream.
 * @param mappings Object to insert into the output stream in a human-readable
 *        form.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const value_mappings& mappings);

/**
 * @}
 */

} /* namespace line */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_LINE_HPP__ */
