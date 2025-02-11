/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file chip-info.hpp
 */

#ifndef __LIBGPIOD_CXX_CHIP_INFO_HPP__
#define __LIBGPIOD_CXX_CHIP_INFO_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <memory>
#include <ostream>

namespace gpiod {

class chip;

/**
 * @brief Represents an immutable snapshot of GPIO chip information.
 */
class chip_info final
{
public:

	/**
	 * @brief Copy constructor.
	 * @param other Object to copy.
	 */
	chip_info(const chip_info& other);

	/**
	 * @brief Move constructor.
	 * @param other Object to move.
	 */
	chip_info(chip_info&& other) noexcept;

	~chip_info();

	/**
	 * @brief Assignment operator.
	 * @param other Object to copy.
	 * @return Reference to self.
	 */
	chip_info& operator=(const chip_info& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Object to move.
	 * @return Reference to self.
	 */
	chip_info& operator=(chip_info&& other) noexcept;

	/**
	 * @brief Get the name of this GPIO chip.
	 * @return String containing the chip name.
	 */
	::std::string name() const noexcept;

	/**
	 * @brief Get the label of this GPIO chip.
	 * @return String containing the chip name.
	 */
	::std::string label() const noexcept;

	/**
	 * @brief Return the number of lines exposed by this chip.
	 * @return Number of lines.
	 */
	::std::size_t num_lines() const noexcept;

private:

	chip_info();

	struct impl;

	::std::shared_ptr<impl> _m_priv;

	friend chip;
};

/**
 * @brief Stream insertion operator for GPIO chip objects.
 * @param out Output stream to write to.
 * @param chip GPIO chip to insert into the output stream.
 * @return Reference to out.
 */
::std::ostream& operator<<(::std::ostream& out, const chip_info& chip);

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_CHIP_INFO_HPP__ */
