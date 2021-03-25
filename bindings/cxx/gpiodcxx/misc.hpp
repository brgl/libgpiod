/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file misc.hpp
 */

#ifndef __LIBGPIOD_CXX_MISC_HPP__
#define __LIBGPIOD_CXX_MISC_HPP__

#if !defined(__LIBGPIOD_GPIOD_CXX_INSIDE__)
#error "Only gpiod.hpp can be included directly."
#endif

#include <string>

namespace gpiod {

/**
 * @ingroup gpiod_cxx
 * @{
 */

/**
 * @brief Check if the file pointed to by path is a GPIO chip character device.
 * @param path Path to check.
 * @return True if the file exists and is a GPIO chip character device or a
 *         symbolic link to it.
 */
bool is_gpiochip_device(const ::std::filesystem::path& path);

/**
 * @brief Get the human readable version string for libgpiod API
 * @return String containing the library version.
 */
const ::std::string& version_string();

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_CXX_MISC_HPP__ */
