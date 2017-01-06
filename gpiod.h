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

#ifndef __GPIOD__
#define __GPIOD__

#include <stdlib.h>
#include <stdbool.h>

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
	GPIOD_ENOTREQUESTED,
	/**< GPIO line not requested. */
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
 * @}
 *
 * @defgroup __high_level__ High-level API
 * @{
 *
 * Simple high-level routines for straightforward GPIO manipulation.
 */

/**
 * @brief Read current value from a single GPIO line.
 * @param device Name, path or number of the gpiochip.
 * @param offset GPIO line offset on the chip.
 * @return 0 or 1 if the operation succeeds. On error this routine returns -1
 *         and sets the last error number.
 */
int gpiod_simple_get_value(const char *device, unsigned int offset) GPIOD_API;

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
 * @brief Available polarity settings.
 */
enum {
	GPIOD_POLARITY_ACTIVE_HIGH,
	/**< The  polarity of a GPIO is active-high. */
	GPIOD_POLARITY_ACTIVE_LOW,
	/**< The polarity of a GPIO is active-low. */
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
 * @param line_bulk Line bulk object.
 *
 * This routine simply sets the internally held number of lines to 0.
 */
static inline void gpiod_line_bulk_init(struct gpiod_line_bulk *line_bulk)
{
	line_bulk->num_lines = 0;
}

/**
 * @brief Add a single line to a GPIO bulk object.
 * @param line_bulk Line bulk object.
 * @param line Line to add.
 */
static inline void gpiod_line_bulk_add(struct gpiod_line_bulk *line_bulk,
				       struct gpiod_line *line)
{
	line_bulk->lines[line_bulk->num_lines++] = line;
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
 * @brief Read the GPIO line polarity setting.
 * @param line GPIO line object.
 * @return Returns GPIOD_POLARITY_ACTIVE_HIGH or GPIOD_POLARITY_ACTIVE_LOW.
 */
int gpiod_line_polarity(struct gpiod_line *line) GPIOD_API;

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
	int polarity;
	/**< Requested polarity configuration. */
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
 * Is this routine succeeds, the caller takes posession of the GPIO line until
 * it's released.
 */
int gpiod_line_request(struct gpiod_line *line,
		       const struct gpiod_line_request_config *config,
		       int default_val) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines.
 * @param line_bulk Set of GPIO lines to reserve.
 * @param config Request options.
 * @param default_vals Default line values - only relevant if we're setting
 *        the direction to output.
 * @return 0 if the all lines were properly requested. In case of an error
 *         this routine returns -1 and sets the last error number.
 */
int gpiod_line_request_bulk(struct gpiod_line_bulk *line_bulk,
			    const struct gpiod_line_request_config *config,
			    int *default_vals) GPIOD_API;

/**
 * @brief Release a previously reserved line.
 * @param line GPIO line object.
 */
void gpiod_line_release(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Release a set of previously reserved lines.
 * @param line_bulk Set of GPIO lines to release.
 */
void gpiod_line_release_bulk(struct gpiod_line_bulk *line_bulk) GPIOD_API;

/**
 * @brief Check if the line is reserved by the calling user.
 * @param line GPIO line object.
 * @return True if given line was requested, false otherwise.
 */
bool gpiod_line_is_reserved(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read current value of a single GPIO line.
 * @param line GPIO line object.
 * @return 0 or 1 if the operation succeeds. On error this routine returns -1
 *         and sets the last error number.
 */
int gpiod_line_get_value(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Read current values of a set of GPIO lines.
 * @param line_bulk Set of GPIO lines to reserve.
 * @param values An array big enough to hold line_bulk->num_lines values.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 *
 * If succeeds, this routine fills the values array with a set of values in
 * the same order, the lines are added to line_bulk.
 */
int gpiod_line_get_value_bulk(struct gpiod_line_bulk *line_bulk,
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
 * @param line_bulk Set of GPIO lines to reserve.
 * @param values An array holding line_bulk->num_lines new values for lines.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
int gpiod_line_set_value_bulk(struct gpiod_line_bulk *line_bulk,
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
	int polarity;
	/**< GPIO line polarity. */
	int line_flags;
	/**< Misc line flags - same as for line requests. */
};

/**
 * @brief Structure holding event info.
 */
struct gpiod_line_event {
	struct gpiod_line *line;
	/**< Line on which the event occurred. */
	struct timespec ts;
	/**< Best estimate of time of event occurrence. */
	int event_type;
	/**< Type of the event that occurred. */
};

/**
 * @brief Request event notifications for a single line.
 * @param line GPIO line object.
 * @param config Event request configuration.
 * @return 0 is the operation succeeds. In case of an error this routine
 *         returns -1 and sets the last error number.
 */
int gpiod_line_event_request(struct gpiod_line *line,
			     struct gpiod_line_evreq_config *config) GPIOD_API;

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
 * @param index The position of the line on which an event occured is stored
 *              in this variable. Can be NULL, in which case the index will
 *              not be stored.
 * @return 0 if wait timed out, -1 if an error occurred, 1 if an event
 *         occurred.
 */
int gpiod_line_event_wait_bulk(struct gpiod_line_bulk *bulk,
			       const struct timespec *timeout,
			       unsigned int *index) GPIOD_API;

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
 * @brief Release all data allocated for a gpiochip iterator.
 * @param iter The gpiochip iterator object.
 */
void gpiod_chip_iter_free(struct gpiod_chip_iter *iter) GPIOD_API;

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
	     (chip);							\
	     (chip) = gpiod_chip_iter_next(iter))

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
};

/**
 * @brief Static initializer for line iterators.
 * @param chip The gpiochip object whose lines we want to iterate over.
 */
#define GPIOD_LINE_ITER_INITIALIZER(chip)	{ 0, (chip) }

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
}

/**
 * @brief Get the next GPIO line handle.
 * @param iter The GPIO line iterator object.
 * @return Pointer to the next GPIO line handle or NULL if no more lines or
 *         and error occured.
 */
static inline struct gpiod_line *
gpiod_line_iter_next(struct gpiod_line_iter *iter)
{
	if (iter->offset >= gpiod_chip_num_lines(iter->chip))
		return NULL;

	return gpiod_chip_get_line(iter->chip, iter->offset++);
}

/**
 * @brief Iterate over all GPIO lines of a single chip.
 * @param iter An initialized GPIO line iterator.
 * @param line Pointer to a GPIO line handle - on each iteration, the
 *             next GPIO line will be assigned to this argument.
 */
#define gpiod_foreach_line(iter, line)					\
	for ((line) = gpiod_line_iter_next(iter);			\
	     (line);							\
	     (line) = gpiod_line_iter_next(iter))

/**
 * @}
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GPIOD__ */
