/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org> */

#ifndef __GPIODGLIB_LINE_H__
#define __GPIODGLIB_LINE_H__

#if !defined(__INSIDE_GPIOD_GLIB_H__) && !defined(GPIODGLIB_COMPILATION)
#error "Only <gpiod-glib.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS


/**
 * GpiodglibLineValue:
 * @GPIODGLIB_LINE_VALUE_INACTIVE: Line is logically inactive.
 * @GPIODGLIB_LINE_VALUE_ACTIVE: Line is logically active.
 *
 * Logical line state.
 */
typedef enum {
	GPIODGLIB_LINE_VALUE_INACTIVE = 0,
	GPIODGLIB_LINE_VALUE_ACTIVE = 1,
} GpiodglibLineValue;

/**
 * GpiodglibLineDirection:
 * @GPIODGLIB_LINE_DIRECTION_AS_IS: Request the line(s), but don't change
 * direction.
 * @GPIODGLIB_LINE_DIRECTION_INPUT: Direction is input - for reading the value
 * of an externally driven GPIO line.
 * @GPIODGLIB_LINE_DIRECTION_OUTPUT: Direction is output - for driving the GPIO
 * line.
 *
 * Direction settings.
 */
typedef enum {
	GPIODGLIB_LINE_DIRECTION_AS_IS = 1,
	GPIODGLIB_LINE_DIRECTION_INPUT,
	GPIODGLIB_LINE_DIRECTION_OUTPUT,
} GpiodglibLineDirection;

/**
 * GpiodglibLineEdge
 * @GPIODGLIB_LINE_EDGE_NONE: Line edge detection is disabled.
 * @GPIODGLIB_LINE_EDGE_RISING: Line detects rising edge events.
 * @GPIODGLIB_LINE_EDGE_FALLING: Line detects falling edge events.
 * @GPIODGLIB_LINE_EDGE_BOTH: Line detects both rising and falling edge events.
 *
 * Edge detection settings.
 */
typedef enum {
	GPIODGLIB_LINE_EDGE_NONE = 1,
	GPIODGLIB_LINE_EDGE_RISING,
	GPIODGLIB_LINE_EDGE_FALLING,
	GPIODGLIB_LINE_EDGE_BOTH,
} GpiodglibLineEdge;

/**
 * GpiodglibLineBias:
 * @GPIODGLIB_LINE_BIAS_AS_IS: Don't change the bias setting when applying line
 * config.
 * @GPIODGLIB_LINE_BIAS_UNKNOWN: The internal bias state is unknown.
 * @GPIODGLIB_LINE_BIAS_DISABLED: The internal bias is disabled.
 * @GPIODGLIB_LINE_BIAS_PULL_UP: The internal pull-up bias is enabled.
 * @GPIODGLIB_LINE_BIAS_PULL_DOWN: The internal pull-down bias is enabled.
 *
 * Internal bias settings.
 */
typedef enum {
	GPIODGLIB_LINE_BIAS_AS_IS = 1,
	GPIODGLIB_LINE_BIAS_UNKNOWN,
	GPIODGLIB_LINE_BIAS_DISABLED,
	GPIODGLIB_LINE_BIAS_PULL_UP,
	GPIODGLIB_LINE_BIAS_PULL_DOWN,
} GpiodglibLineBias;

/**
 * GpiodglibLineDrive:
 * @GPIODGLIB_LINE_DRIVE_PUSH_PULL: Drive setting is push-pull.
 * @GPIODGLIB_LINE_DRIVE_OPEN_DRAIN: Line output is open-drain.
 * @GPIODGLIB_LINE_DRIVE_OPEN_SOURCE: Line output is open-source.
 *
 * Drive settings.
 */
typedef enum {
	GPIODGLIB_LINE_DRIVE_PUSH_PULL = 1,
	GPIODGLIB_LINE_DRIVE_OPEN_DRAIN,
	GPIODGLIB_LINE_DRIVE_OPEN_SOURCE,
} GpiodglibLineDrive;

/**
 * GpiodglibLineClock:
 * @GPIODGLIB_LINE_CLOCK_MONOTONIC: Line uses the monotonic clock for edge
 * event timestamps.
 * @GPIODGLIB_LINE_CLOCK_REALTIME: Line uses the realtime clock for edge event
 * timestamps.
 * @GPIODGLIB_LINE_CLOCK_HTE: Line uses the hardware timestamp engine for event
 * timestamps.
 *
 * Clock settings.
 */
typedef enum {
	GPIODGLIB_LINE_CLOCK_MONOTONIC = 1,
	GPIODGLIB_LINE_CLOCK_REALTIME,
	GPIODGLIB_LINE_CLOCK_HTE,
} GpiodglibLineClock;

G_END_DECLS

#endif /* __GPIODGLIB_LINE_H__ */
