/*
 * GPIO chardev utils for linux.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

/**
 * @mainpage libgpiod public API
 *
 * This is the documentation of the public API exported by libgpiod.
 *
 * <p>These functions and data structures allow to use all the functionalities
 * exposed by the linux GPIO character device interface.
 */

#ifndef __LIBGPIOD_GPIOD_H__
#define __LIBGPIOD_GPIOD_H__

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpiod_chip;
struct gpiod_line;
struct gpiod_chip_iter;

/**
 * @defgroup __common__ Common helper macros
 * @{
 *
 * Commonly used utility macros.
 */

/**
 * @brief Makes symbol visible.
 */
#define GPIOD_API		__attribute__((visibility("default")))

/**
 * @brief Shift 1 by given offset.
 * @param nr Bit position.
 * @return 1 shifted by nr.
 */
#define GPIOD_BIT(nr)		(1UL << (nr))

/**
 * @}
 *
 * @defgroup __error__ Error handling
 * @{
 *
 * Error handling functions and macros. This library uses a combination of
 * system-wide errno numbers and internal GPIO-specific errors. The routines
 * in this section should be used to access the information about any error
 * conditions that may occur.
 *
 * With some noted exceptions, all libgpiod functions set the last error
 * variable if an error occurs. Internally, the last_error variable has a
 * separate instance per thread making the library thread-safe.
 */

/**
 * @brief Private: offset for all libgpiod error numbers.
 */
#define __GPIOD_ERRNO_OFFSET	10000

/**
 * @brief libgpiod-specific error numbers.
 */
enum {
	GPIOD_ESUCCESS = __GPIOD_ERRNO_OFFSET,
	/**< No error. */
	GPIOD_EREQUEST,
	/**< The caller has no ownership of this line. */
	GPIOD_EEVREQUEST,
	/**< The caller has not configured any events on this line. */
	GPIOD_EBULKINCOH,
	/**< Not all lines in bulk belong to the same GPIO chip. */
	GPIOD_ELINEBUSY,
	/**< This line is currently in use. */
	GPIOD_ELINEMAX,
	/**< Number of lines in the request exceeds limit. */
	__GPIOD_MAX_ERR,
	/**< Private: number of libgpiod-specific error numbers. */
};

/**
 * @brief Return last error.
 * @return Number of the last error inside libgpiod.
 */
int gpiod_errno(void) GPIOD_API;

/**
 * @brief Convert error number to a human-readable string.
 * @param errnum Error number to convert.
 * @return Pointer to a null-terminated error description.
 */
const char * gpiod_strerror(int errnum) GPIOD_API;

/**
 * @brief Convert the last libgpiod error number to a human-readable string.
 * @return Pointer to a null-terminated error description.
 */
const char * gpiod_last_strerror(void) GPIOD_API;

/**
 * @}
 *
 * @defgroup __high_level__ High-level API
 * @{
 *
 * Simple high-level routines for straightforward GPIO manipulation.
 */

/**
 * @brief Read current values from a set of GPIO lines.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offsets An array of offsets of lines whose values should be read.
 * @param values A buffer in which the values will be stored.
 * @param num_lines Number of lines.
 * @param active_low The active state of the lines - true if low.
 * @return 0 if the operation succeeds, -1 on error.
 */
int gpiod_simple_get_value_multiple(const char *consumer, const char *device,
				    const unsigned int *offsets, int *values,
				    unsigned int num_lines,
				    bool active_low) GPIOD_API;

/**
 * @brief Read current value from a single GPIO line.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offset GPIO line offset on the chip.
 * @param active_low The active state of this line - true if low.
 * @return 0 or 1 (GPIO value) if the operation succeeds, -1 on error.
 */
static inline int gpiod_simple_get_value(const char *consumer,
					 const char *device,
					 unsigned int offset, bool active_low)
{
	int value, status;

	status = gpiod_simple_get_value_multiple(consumer, device, &offset,
						 &value, 1, active_low);
	if (status < 0)
		return status;

	return value;
}

/**
 * @brief Simple set value callback signature.
 */
typedef void (*gpiod_set_value_cb)(void *);

/**
 * @brief Set values of a set of a set of GPIO lines.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offsets An array of offsets of lines whose values should be set.
 * @param values An array of integers containing new values.
 * @param num_lines Number of lines.
 * @param active_low The active state of the lines - true if low.
 * @param cb Callback function that will be called right after the values are
 *        set.
 * @param data User data that will be passed to the callback function.
 * @return 0 if the operation succeeds, -1 on error.
 */
int gpiod_simple_set_value_multiple(const char *consumer, const char *device,
				    const unsigned int *offsets,
				    const int *values, unsigned int num_lines,
				    bool active_low, gpiod_set_value_cb cb,
				    void *data) GPIOD_API;

/**
 * @brief Set value of a single GPIO line.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offset GPIO line offset on the chip.
 * @param value New value.
 * @param active_low The active state of this line - true if low.
 * @param cb Callback function that will be called right after the value is
 *        set. Users can use this, for example, to pause the execution after
 *        toggling a GPIO.
 * @param data User data that will be passed to the callback function.
 * @return 0 if the operation succeeds, -1 on error.
 */
static inline int gpiod_simple_set_value(const char *consumer,
					 const char *device,
					 unsigned int offset, int value,
					 bool active_low,
					 gpiod_set_value_cb cb, void *data)
{
	return gpiod_simple_set_value_multiple(consumer, device, &offset,
					       &value, 1, active_low,
					       cb, data);
}

/**
 * @brief Event types that can be passed to the simple event callback.
 */
enum {
	GPIOD_EVENT_CB_TIMEOUT,
	/**< Waiting for events timed out. */
	GPIOD_EVENT_CB_RISING_EDGE,
	/**< Rising edge event occured. */
	GPIOD_EVENT_CB_FALLING_EDGE,
	/**< Falling edge event occured. */
};

/**
 * @brief Return status values that the simple event callback can return.
 */
enum {
	GPIOD_EVENT_CB_OK = 0,
	/**< Continue processing events. */
	GPIOD_EVENT_CB_STOP,
	/**< Stop processing events. */
};

/**
 * @brief Simple event callack signature.
 */
typedef int (*gpiod_event_cb)(int, const struct timespec *, void *);

/**
 * @brief Wait for events on a single GPIO line.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offset GPIO line offset on the chip.
 * @param active_low The active state of this line - true if low.
 * @param timeout Maximum wait time for each iteration.
 * @param callback Callback function to call on event occurence.
 * @param cbdata User data passed to the callback.
 * @return 0 no errors were encountered, -1 if an error occured.
 *
 */
int gpiod_simple_event_loop(const char *consumer, const char *device,
			    unsigned int offset, bool active_low,
			    const struct timespec *timeout,
			    gpiod_event_cb callback, void *cbdata) GPIOD_API;

/**
 * @}
 *
 * @defgroup __lines__ GPIO line operations
 * @{
 *
 * Functions and data structures dealing with GPIO lines.
 */

/**
 * @brief Available direction settings.
 *
 * These values are used both when requesting lines and when retrieving
 * line info.
 */
enum {
	GPIOD_DIRECTION_AS_IS,
	/**< Only relevant for line requests - don't set the direction. */
	GPIOD_DIRECTION_INPUT,
	/**< Direction is input - we're reading the state of a GPIO line. */
	GPIOD_DIRECTION_OUTPUT,
	/**< Direction is output - we're driving the GPIO line. */
};

/**
 * @brief Available active state settings.
 */
enum {
	GPIOD_ACTIVE_STATE_HIGH,
	/**< The active state of a GPIO is active-high. */
	GPIOD_ACTIVE_STATE_LOW,
	/**< The active state of a GPIO is active-low. */
};

/**
 * @brief Miscellaneous GPIO flags.
 */
enum {
	GPIOD_REQUEST_OPEN_DRAIN	= GPIOD_BIT(0),
	/**< The line is an open-drain port. */
	GPIOD_REQUEST_OPEN_SOURCE	= GPIOD_BIT(1),
	/**< The line is an open-source port. */
};

/**
 * @brief Maximum number of GPIO lines that can be requested at once.
 */
#define GPIOD_REQUEST_MAX_LINES		64

/**
 * @brief Helper structure for storing a set of GPIO line objects.
 *
 * This structure is used in all operations involving sets of GPIO lines.
 */
struct gpiod_line_bulk {
	struct gpiod_line *lines[GPIOD_REQUEST_MAX_LINES];
	/**< Buffer for line pointers. */
	unsigned int num_lines;
	/**< Number of lines currently held in this structure. */
};

/**
 * @brief Static initializer for GPIO bulk objects.
 *
 * This macro simply sets the internally held number of lines to 0.
 */
#define GPIOD_LINE_BULK_INITIALIZER	{ .num_lines = 0, }

/**
 * @brief Initialize a GPIO bulk object.
 * @param bulk Line bulk object.
 *
 * This routine simply sets the internally held number of lines to 0.
 */
static inline void gpiod_line_bulk_init(struct gpiod_line_bulk *bulk)
{
	bulk->num_lines = 0;
}

/**
 * @brief Add a single line to a GPIO bulk object.
 * @param bulk Line bulk object.
 * @param line Line to add.
 */
static inline void gpiod_line_bulk_add(struct gpiod_line_bulk *bulk,
				       struct gpiod_line *line)
{
	bulk->lines[bulk->num_lines++] = line;
}

/**
 * @brief Read the GPIO line offset.
 * @param line GPIO line object.
 * @return Line offset.
 */
unsigned int gpiod_line_offset(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read the GPIO line name.
 * @param line GPIO line object.
 * @return Name of the GPIO line as it is represented in the kernel. This
 *         routine returns a pointer to a null-terminated string or NULL if
 *         the line is unnamed.
 */
const char * gpiod_line_name(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read the GPIO line consumer name.
 * @param line GPIO line object.
 * @return Name of the GPIO consumer name as it is represented in the
 *         kernel. This routine returns a pointer to a null-terminated string
 *         or NULL if the line is not used.
 */
const char * gpiod_line_consumer(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read the GPIO line direction setting.
 * @param line GPIO line object.
 * @return Returns GPIOD_DIRECTION_INPUT or GPIOD_DIRECTION_OUTPUT.
 */
int gpiod_line_direction(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read the GPIO line active state setting.
 * @param line GPIO line object.
 * @return Returns GPIOD_ACTIVE_STATE_HIGH or GPIOD_ACTIVE_STATE_LOW.
 */
int gpiod_line_active_state(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if the line is used by the kernel.
 * @param line GPIO line object.
 * @return True if the line is used by the kernel, false otherwise.
 */
bool gpiod_line_is_used_by_kernel(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if the line is an open-drain GPIO.
 * @param line GPIO line object.
 * @return True if the line is an open-drain GPIO, false otherwise.
 */
bool gpiod_line_is_open_drain(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if the line is an open-source GPIO.
 * @param line GPIO line object.
 * @return True if the line is an open-source GPIO, false otherwise.
 */
bool gpiod_line_is_open_source(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Re-read the line info.
 * @param line GPIO line object.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 *
 * The line info is initially retrieved from the kernel by
 * gpiod_chip_get_line(). Users can use this line to manually re-read the line
 * info.
 */
int gpiod_line_update(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if the line info needs to be updated.
 * @param line GPIO line object.
 * @return Returns false if the line is up-to-date. True otherwise.
 *
 * The line is updated by calling gpiod_line_update() from within
 * gpiod_chip_get_line() and on every line request/release. However: an error
 * returned from gpiod_line_update() only breaks the execution of the former.
 * The request/release routines only set the internal up-to-date flag to false
 * and continue their execution. This routine allows to check if a line info
 * update failed at some point and we should call gpiod_line_update()
 * explicitly.
 */
bool gpiod_line_needs_update(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Structure holding configuration of a line request.
 */
struct gpiod_line_request_config {
	const char *consumer;
	/**< Name of the consumer. */
	int direction;
	/**< Requested direction. */
	int active_state;
	/**< Requested active state configuration. */
	int flags;
	/**< Other configuration flags. */
};

/**
 * @brief Reserve a single line.
 * @param line GPIO line object.
 * @param config Request options.
 * @param default_val Default line value - only relevant if we're setting
 *        the direction to output.
 * @return 0 if the line was properly reserved. In case of an error this
 *         routine returns -1 and sets the last error number.
 *
 * Is this routine succeeds, the caller takes ownership of the GPIO line until
 * it's released.
 */
int gpiod_line_request(struct gpiod_line *line,
		       const struct gpiod_line_request_config *config,
		       int default_val) GPIOD_API;

/**
 * @brief Reserve a single line, set the direction to input.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the line (true if low).
 * @return 0 if the line was properly reserved, -1 on failure.
 */
static inline int gpiod_line_request_input(struct gpiod_line *line,
					   const char *consumer,
					   bool active_low)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_INPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request(line, &config, 0);
}

/**
 * @brief Reserve a single line, set the direction to output.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the line (true if low).
 * @param default_val Default line value.
 * @return 0 if the line was properly reserved, -1 on failure.
 */
static inline int gpiod_line_request_output(struct gpiod_line *line,
					    const char *consumer,
					    bool active_low, int default_val)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_OUTPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request(line, &config, default_val);
}

/**
 * @brief Reserve a set of GPIO lines.
 * @param bulk Set of GPIO lines to reserve.
 * @param config Request options.
 * @param default_vals Default line values - only relevant if we're setting
 *        the direction to output.
 * @return 0 if the all lines were properly requested. In case of an error
 *         this routine returns -1 and sets the last error number.
 *
 * Is this routine succeeds, the caller takes ownership of the GPIO line until
 * it's released.
 */
int gpiod_line_request_bulk(struct gpiod_line_bulk *bulk,
			    const struct gpiod_line_request_config *config,
			    const int *default_vals) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines, set the direction to input.
 * @param bulk Set of GPIO lines to reserve.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the lines (true if low).
 * @return 0 if the lines were properly reserved, -1 on failure.
 */
static inline int gpiod_line_request_bulk_input(struct gpiod_line_bulk *bulk,
						const char *consumer,
						bool active_low)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_INPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

/**
 * @brief Reserve a set of GPIO lines, set the direction to output.
 * @param bulk Set of GPIO lines to reserve.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the lines (true if low).
 * @param default_vals Default line values.
 * @return 0 if the lines were properly reserved, -1 on failure.
 */
static inline int gpiod_line_request_bulk_output(struct gpiod_line_bulk *bulk,
						 const char *consumer,
						 bool active_low,
						 const int *default_vals)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_OUTPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request_bulk(bulk, &config, default_vals);
}

/**
 * @brief Release a previously reserved line.
 * @param line GPIO line object.
 */
void gpiod_line_release(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Release a set of previously reserved lines.
 * @param bulk Set of GPIO lines to release.
 */
void gpiod_line_release_bulk(struct gpiod_line_bulk *bulk) GPIOD_API;

/**
 * @brief Check if the calling user has ownership of this line.
 * @param line GPIO line object.
 * @return True if given line was requested, false otherwise.
 */
bool gpiod_line_is_reserved(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if the calling user has neither requested ownership of this
 *        line nor configured any event notifications.
 * @param line GPIO line object.
 * @return True if given line is free, false otherwise.
 */
bool gpiod_line_is_free(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read current value of a single GPIO line.
 * @param line GPIO line object.
 * @return 0 or 1 if the operation succeeds. On error this routine returns -1
 *         and sets the last error number.
 */
int gpiod_line_get_value(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read current values of a set of GPIO lines.
 * @param bulk Set of GPIO lines to reserve.
 * @param values An array big enough to hold line_bulk->num_lines values.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 *
 * If succeeds, this routine fills the values array with a set of values in
 * the same order, the lines are added to line_bulk.
 */
int gpiod_line_get_value_bulk(struct gpiod_line_bulk *bulk,
			      int *values) GPIOD_API;

/**
 * @brief Set the value of a single GPIO line.
 * @param line GPIO line object.
 * @param value New value.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
int gpiod_line_set_value(struct gpiod_line *line, int value) GPIOD_API;

/**
 * @brief Set the values of a set of GPIO lines.
 * @param bulk Set of GPIO lines to reserve.
 * @param values An array holding line_bulk->num_lines new values for lines.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
int gpiod_line_set_value_bulk(struct gpiod_line_bulk *bulk,
			      int *values) GPIOD_API;

/**
 * @brief Find a GPIO line by its name.
 * @param name Name of the GPIO line.
 * @return Returns the GPIO line handle if the line exists in the system or
 *         NULL if it couldn't be located.
 *
 * If this routine succeeds, the user must manually close the GPIO chip owning
 * this line to avoid memory leaks.
 */
struct gpiod_line * gpiod_line_find_by_name(const char *name) GPIOD_API;

/**
 * @brief Get the handle to the GPIO chip controlling this line.
 * @param line The GPIO line object.
 * @return Pointer to the GPIO chip handle controlling this line.
 */
struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line) GPIOD_API;

/**
 * @defgroup __line_events__ Line event operations
 * @{
 *
 * Functions and data structures for requesting and reading GPIO line events.
 */

/**
 * @brief Event types.
 */
enum {
	GPIOD_EVENT_RISING_EDGE,
	/**< Rising edge event. */
	GPIOD_EVENT_FALLING_EDGE,
	/**< Falling edge event. */
	GPIOD_EVENT_BOTH_EDGES,
	/**< Rising or falling edge event: only relevant for event requests. */
};

/**
 * @brief Structure holding configuration of a line event request.
 */
struct gpiod_line_evreq_config {
	const char *consumer;
	/**< Name of the consumer. */
	int event_type;
	/**< Type of the event we want to be notified about. */
	int active_state;
	/**< GPIO line active state. */
	int line_flags;
	/**< Misc line flags - same as for line requests. */
};

/**
 * @brief Structure holding event info.
 */
struct gpiod_line_event {
	struct timespec ts;
	/**< Best estimate of time of event occurrence. */
	int event_type;
	/**< Type of the event that occurred. */
};

/**
 * @brief Request event notifications for a single line.
 * @param line GPIO line object.
 * @param config Event request configuration.
 * @return 0 if the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
int gpiod_line_event_request(struct gpiod_line *line,
			     struct gpiod_line_evreq_config *config) GPIOD_API;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
static inline int _gpiod_line_event_request_type(struct gpiod_line *line,
						 const char *consumer,
						 bool active_low,
						 int type)
{
	struct gpiod_line_evreq_config config = {
		.consumer = consumer,
		.event_type = type,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_event_request(line, &config);
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @brief Request rising edge event notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the line - true if low.
 * @return 0 if the operation succeeds, -1 on failure.
 */
static inline int gpiod_line_event_request_rising(struct gpiod_line *line,
						  const char *consumer,
						  bool active_low)
{
	return _gpiod_line_event_request_type(line, consumer, active_low,
					      GPIOD_EVENT_RISING_EDGE);
}

/**
 * @brief Request falling edge event notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the line - true if low.
 * @return 0 if the operation succeeds, -1 on failure.
 */
static inline int gpiod_line_event_request_falling(struct gpiod_line *line,
						   const char *consumer,
						   bool active_low)
{
	return _gpiod_line_event_request_type(line, consumer, active_low,
					      GPIOD_EVENT_FALLING_EDGE);
}

/**
 * @brief Request all event type notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param active_low Active state of the line - true if low.
 * @return 0 if the operation succeeds, -1 on failure.
 */
static inline int gpiod_line_event_request_all(struct gpiod_line *line,
					       const char *consumer,
					       bool active_low)
{
	return _gpiod_line_event_request_type(line, consumer, active_low,
					      GPIOD_EVENT_BOTH_EDGES);
}

/**
 * @brief Stop listening for events and release the line.
 * @param line GPIO line object.
 */
void gpiod_line_event_release(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if event notifications are configured on this line.
 * @param line GPIO line object.
 * @return True if event notifications are configured. False otherwise.
 */
bool gpiod_line_event_configured(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Wait for an event on a single line.
 * @param line GPIO line object.
 * @param timeout Wait time limit.
 * @return 0 if wait timed out, -1 if an error occurred, 1 if an event
 *         occurred.
 */
int gpiod_line_event_wait(struct gpiod_line *line,
			  const struct timespec *timeout) GPIOD_API;

/**
 * @brief Wait for the first event on a set of lines.
 * @param bulk Set of GPIO lines to monitor.
 * @param timeout Wait time limit.
 * @param line The handle of the line on which an event occurs is stored
 *             in this variable. Can be NULL.
 * @return 0 if wait timed out, -1 if an error occurred, 1 if an event
 *         occurred.
 */
int gpiod_line_event_wait_bulk(struct gpiod_line_bulk *bulk,
			       const struct timespec *timeout,
			       struct gpiod_line **line) GPIOD_API;

/**
 * @brief Read the last event from the GPIO line.
 * @param line GPIO line object.
 * @param event Buffer to which the event data will be copied.
 * @return 0 if the event was read correctly, -1 on error.
 */
int gpiod_line_event_read(struct gpiod_line *line,
			  struct gpiod_line_event *event) GPIOD_API;

/**
 * @brief Get the event file descriptor.
 * @param line GPIO line object.
 * @return Number of the event file descriptor or -1 on error.
 *
 * Users may want to poll the event file descriptor on their own. This routine
 * allows to access it.
 */
int gpiod_line_event_get_fd(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read the last GPIO event directly from a file descriptor.
 * @param fd File descriptor.
 * @param event Buffer to which the event data will be copied.
 * @return 0 if the event was read correctly, -1 on error.
 *
 * Users who directly poll the file descriptor for incoming events can also
 * directly read the event data from it using this routine. This function
 * translates the kernel representation of the event to the libgpiod format.
 */
int gpiod_line_event_read_fd(int fd, struct gpiod_line_event *event) GPIOD_API;

/**
 * @}
 *
 * @}
 *
 * @defgroup __chips__ GPIO chip operations
 * @{
 *
 * Functions and data structures dealing with GPIO chips.
 */

/**
 * @brief Open a gpiochip by path.
 * @param path Path to the gpiochip device file.
 * @return GPIO chip handle or NULL if an error occurred.
 */
struct gpiod_chip * gpiod_chip_open(const char *path) GPIOD_API;

/**
 * @brief Open a gpiochip by name.
 * @param name Name of the gpiochip to open.
 * @return GPIO chip handle or NULL if an error occurred.
 *
 * This routine appends name to '/dev/' to create the path.
 */
struct gpiod_chip * gpiod_chip_open_by_name(const char *name) GPIOD_API;

/**
 * @brief Open a gpiochip by number.
 * @param num Number of the gpiochip.
 * @return GPIO chip handle or NULL if an error occurred.
 *
 * This routine appends num to '/dev/gpiochip' to create the path.
 */
struct gpiod_chip * gpiod_chip_open_by_number(unsigned int num) GPIOD_API;

/**
 * @brief Open a gpiochip based on the best guess what the path is.
 * @param descr String describing the gpiochip.
 * @return GPIO chip handle or NULL if an error occurred.
 *
 * This routine tries to figure out whether the user passed it the path to
 * the GPIO chip, its name or number as a string. Then it tries to open it
 * using one of the other gpiod_chip_open** routines.
 */
struct gpiod_chip * gpiod_chip_open_lookup(const char *descr) GPIOD_API;

/**
 * @brief Close a GPIO chip handle and release all allocated resources.
 * @param chip The GPIO chip object.
 */
void gpiod_chip_close(struct gpiod_chip *chip) GPIOD_API;

/**
 * @brief Get the GPIO chip name as represented in the kernel.
 * @param chip The GPIO chip object.
 * @return Pointer to a human-readable string containing the chip name.
 */
const char * gpiod_chip_name(struct gpiod_chip *chip) GPIOD_API;

/**
 * @brief Get the GPIO chip label as represented in the kernel.
 * @param chip The GPIO chip object.
 * @return Pointer to a human-readable string containing the chip label.
 */
const char * gpiod_chip_label(struct gpiod_chip *chip) GPIOD_API;

/**
 * @brief Get the number of GPIO lines exposed by this chip.
 * @param chip The GPIO chip object.
 * @return Number of GPIO lines.
 */
unsigned int gpiod_chip_num_lines(struct gpiod_chip *chip) GPIOD_API;

/**
 * @brief Get the handle to the GPIO line at given offset.
 * @param chip The GPIO chip object.
 * @param offset The offset of the GPIO line.
 * @return Pointer to the GPIO line handle or NULL if an error occured.
 */
struct gpiod_line *
gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) GPIOD_API;

/**
 * @}
 *
 * @defgroup __iterators__ Iterators for GPIO chips and lines
 * @{
 *
 * These functions and data structures allow easy iterating over GPIO
 * chips and lines.
 */

/**
 * @brief Create a new gpiochip iterator.
 * @return Pointer to a new chip iterator object or NULL if an error occurred.
 */
struct gpiod_chip_iter * gpiod_chip_iter_new(void) GPIOD_API;

/**
 * @brief Release all resources allocated for the gpiochip iterator and close
 *        the most recently opened gpiochip (if any).
 * @param iter The gpiochip iterator object.
 */
void gpiod_chip_iter_free(struct gpiod_chip_iter *iter) GPIOD_API;

/**
 * @brief Release all resources allocated for the gpiochip iterator but
 *        don't close the most recently opened gpiochip (if any).
 * @param iter The gpiochip iterator object.
 *
 * Users may want to break the loop when iterating over gpiochips and keep
 * the most recently opened chip active while freeing the iterator data.
 * This routine enables that.
 */
void gpiod_chip_iter_free_noclose(struct gpiod_chip_iter *iter) GPIOD_API;

/**
 * @brief Get the next gpiochip handle.
 * @param iter The gpiochip iterator object.
 * @return Pointer to an open gpiochip handle or NULL if the next chip can't
 *         be accessed.
 *
 * Internally this routine scans the /dev/ directory, storing current state
 * in the chip iterator structure, and tries to open the next /dev/gpiochipX
 * device file. If an error occurs or no more chips are present, the function
 * returns NULL.
 */
struct gpiod_chip *
gpiod_chip_iter_next(struct gpiod_chip_iter *iter) GPIOD_API;

/**
 * @brief Iterate over all gpiochip present in the system.
 * @param iter An initialized GPIO chip iterator.
 * @param chip Pointer to a GPIO chip handle. On each iteration the newly
 *             opened chip handle is assigned to this argument.
 *
 * The user must not close the GPIO chip manually - instead the previous chip
 * handle is closed automatically on the next iteration. The last chip to be
 * opened is closed internally by gpiod_chip_iter_free().
 */
#define gpiod_foreach_chip(iter, chip)					\
	for ((chip) = gpiod_chip_iter_next(iter);			\
	     !gpiod_chip_iter_done(iter);				\
	     (chip) = gpiod_chip_iter_next(iter))

/**
 * @brief Check if we're done iterating over gpiochips on this iterator.
 * @param iter The gpiochip iterator object.
 * @return True if we've iterated over all chips, false otherwise.
 */
bool gpiod_chip_iter_done(struct gpiod_chip_iter *iter) GPIOD_API;

/**
 * @brief Check if we've encountered an error condition while opening a
 *        gpiochip.
 * @param iter The gpiochip iterator object.
 * @return True if there was an error opening a gpiochip device file,
 *         false otherwise.
 */
bool gpiod_chip_iter_err(struct gpiod_chip_iter *iter) GPIOD_API;

/**
 * @brief Get the name of the gpiochip that we failed to access.
 * @param iter The gpiochip iterator object.
 * @return If gpiod_chip_iter_iserr() returned true, this function returns a
 *         pointer to the name of the gpiochip that we failed to access.
 *         If there was no error this function returns NULL.
 *
 * NOTE: this function will return NULL if the internal memory allocation
 * fails.
 */
const char *
gpiod_chip_iter_failed_chip(struct gpiod_chip_iter *iter) GPIOD_API;

/**
 * @brief Possible states of a line iterator.
 */
enum {
	GPIOD_LINE_ITER_INIT = 0,
	/**< Line iterator is initiated or iterating over lines. */
	GPIOD_LINE_ITER_DONE,
	/**< Line iterator is done with all lines on this chip. */
	GPIOD_LINE_ITER_ERR,
	/**< There was an error retrieving info for a line. */
};

/**
 * @brief GPIO line iterator structure.
 *
 * This structure is used in conjunction with gpiod_line_iter_next() to
 * iterate over all GPIO lines of a single GPIO chip.
 */
struct gpiod_line_iter {
	unsigned int offset;
	/**< Current line offset. */
	struct gpiod_chip *chip;
	/**< GPIO chip whose line we're iterating over. */
	int state;
	/**< Current state of the iterator. */
};

/**
 * @brief Static initializer for line iterators.
 * @param chip The gpiochip object whose lines we want to iterate over.
 */
#define GPIOD_LINE_ITER_INITIALIZER(chip) 				\
	{								\
		.offset = 0,						\
		.chip = (chip),						\
		.state = GPIOD_LINE_ITER_INIT,				\
	}

/**
 * @brief Initialize a GPIO line iterator.
 * @param iter Pointer to a GPIO line iterator.
 * @param chip The gpiochip object whose lines we want to iterate over.
 */
static inline void gpiod_line_iter_init(struct gpiod_line_iter *iter,
					struct gpiod_chip *chip)
{
	iter->offset = 0;
	iter->chip = chip;
	iter->state = GPIOD_LINE_ITER_INIT;
}

/**
 * @brief Get the next GPIO line handle.
 * @param iter The GPIO line iterator object.
 * @return Pointer to the next GPIO line handle or NULL if no more lines or
 *         and error occured.
 */
struct gpiod_line *
gpiod_line_iter_next(struct gpiod_line_iter *iter) GPIOD_API;

/**
 * @brief Check if we're done iterating over lines on this iterator.
 * @param iter The GPIO line iterator object.
 * @return True if we've iterated over all lines, false otherwise.
 */
static inline bool gpiod_line_iter_done(const struct gpiod_line_iter *iter)
{
	return iter->state == GPIOD_LINE_ITER_DONE;
}

/**
 * @brief Check if we've encountered an error condition while retrieving
 *        info for a line.
 * @param iter The GPIO line iterator object.
 * @return True if there was an error retrieving info about a GPIO line,
 *         false otherwise.
 */
static inline bool gpiod_line_iter_err(const struct gpiod_line_iter *iter)
{
	return iter->state == GPIOD_LINE_ITER_ERR;
}

/**
 * @brief Get the offset of the last line we tried to open.
 * @param iter The GPIO line iterator object.
 * @return The offset of the last line we tried to open - whether we failed
 *         or succeeded to do so.
 */
static inline unsigned int
gpiod_line_iter_last_offset(const struct gpiod_line_iter *iter)
{
	return iter->offset - 1;
}

/**
 * @brief Iterate over all GPIO lines of a single chip.
 * @param iter An initialized GPIO line iterator.
 * @param line Pointer to a GPIO line handle - on each iteration, the
 *             next GPIO line will be assigned to this argument.
 */
#define gpiod_foreach_line(iter, line)					\
	for ((line) = gpiod_line_iter_next(iter);			\
	     !gpiod_line_iter_done(iter);				\
	     (line) = gpiod_line_iter_next(iter))

/**
 * @}
 *
 * @defgroup __misc__ Stuff that didn't fit anywhere else
 * @{
 *
 * Various libgpiod-related functions.
 */

/**
 * @brief Get the version of the library as a human-readable string.
 * @return Human-readable string containing the library version.
 */
const char * gpiod_version_string(void) GPIOD_API;

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LIBGPIOD_GPIOD_H__ */
