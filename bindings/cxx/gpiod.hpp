/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl> */

/**
 * @file gpiod.hpp
 */

#ifndef __LIBGPIOD_GPIOD_CXX_HPP__
#define __LIBGPIOD_GPIOD_CXX_HPP__

/**
 * @defgroup gpiod_cxx C++ bindings
 *
 * C++ bindings for libgpiod.
 */

/**
 * @cond
 */

/*
 * We don't make this symbol private because it needs to be accessible by
 * the declarations in exception.hpp in order to expose the symbols of classes
 * inheriting from standard exceptions.
 */
#ifdef GPIOD_CXX_BUILD
#define GPIOD_CXX_API __attribute__((visibility("default")))
#else
#define GPIOD_CXX_API __attribute__((visibility("hidden")))
#endif

#define __LIBGPIOD_GPIOD_CXX_INSIDE__
#include "gpiodcxx/chip.hpp"
#include "gpiodcxx/chip-info.hpp"
#include "gpiodcxx/edge-event.hpp"
#include "gpiodcxx/edge-event-buffer.hpp"
#include "gpiodcxx/exception.hpp"
#include "gpiodcxx/info-event.hpp"
#include "gpiodcxx/line.hpp"
#include "gpiodcxx/line-config.hpp"
#include "gpiodcxx/line-info.hpp"
#include "gpiodcxx/line-request.hpp"
#include "gpiodcxx/line-settings.hpp"
#include "gpiodcxx/request-builder.hpp"
#include "gpiodcxx/request-config.hpp"
#undef __LIBGPIOD_GPIOD_CXX_INSIDE__

/**
 * @endcond
 */

#endif /* __LIBGPIOD_GPIOD_CXX_HPP__ */
