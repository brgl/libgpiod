/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#ifndef __LIBGPIOD_GPIOD_H__
#define __LIBGPIOD_GPIOD_H__

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @mainpage libgpiod public API
 *
 * This is the documentation of the public API exported by libgpiod.
 *
 * <p>These functions and data structures allow to use all the functionalities
 * exposed by the linux GPIO character device interface.
 */

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
 * @brief Marks a function argument or variable as potentially unused.
 */
#define GPIOD_UNUSED		__attribute__((unused))

/**
 * @brief Shift 1 by given offset.
 * @param nr Bit position.
 * @return 1 shifted by nr.
 */
#define GPIOD_BIT(nr)		(1UL << (nr))

/**
 * @}
 *
 * @defgroup __high_level__ High-level API
 * @{
 *
 * Simple high-level routines for straightforward GPIO manipulation without
 * the need to use the gpiod_* structures or to keep track of resources.
 */

/**
 * @brief Read current value from a single GPIO line.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offset GPIO line offset on the chip.
 * @param active_low The active state of this line - true if low.
 * @return 0 or 1 (GPIO value) if the operation succeeds, -1 on error.
 */
int gpiod_simple_get_value(const char *consumer, const char *device,
			   unsigned int offset, bool active_low) GPIOD_API;

/**
 * @brief Read current values from a set of GPIO lines.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offsets An array of offsets of lines whose values should be read.
 * @param values A buffer in which the values will be stored.
 * @param num_lines Number of lines, must be > 0.
 * @param active_low The active state of the lines - true if low.
 * @return 0 if the operation succeeds, -1 on error.
 */
int gpiod_simple_get_value_multiple(const char *consumer, const char *device,
				    const unsigned int *offsets, int *values,
				    unsigned int num_lines,
				    bool active_low) GPIOD_API;

/**
 * @brief Simple set value callback signature.
 */
typedef void (*gpiod_simple_set_value_cb)(void *);

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
int gpiod_simple_set_value(const char *consumer, const char *device,
			   unsigned int offset, int value, bool active_low,
			   gpiod_simple_set_value_cb cb, void *data) GPIOD_API;

/**
 * @brief Set values of a set of a set of GPIO lines.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offsets An array of offsets of lines whose values should be set.
 * @param values An array of integers containing new values.
 * @param num_lines Number of lines, must be > 0.
 * @param active_low The active state of the lines - true if low.
 * @param cb Callback function that will be called right after the values are
 *        set.
 * @param data User data that will be passed to the callback function.
 * @return 0 if the operation succeeds, -1 on error.
 */
int gpiod_simple_set_value_multiple(const char *consumer, const char *device,
				    const unsigned int *offsets,
				    const int *values, unsigned int num_lines,
				    bool active_low,
				    gpiod_simple_set_value_cb cb,
				    void *data) GPIOD_API;

/**
 * @brief Event types that can be passed to the simple event callback.
 */
enum {
	GPIOD_SIMPLE_EVENT_CB_TIMEOUT,
	/**< Waiting for events timed out. */
	GPIOD_SIMPLE_EVENT_CB_RISING_EDGE,
	/**< Rising edge event occured. */
	GPIOD_SIMPLE_EVENT_CB_FALLING_EDGE,
	/**< Falling edge event occured. */
};

/**
 * @brief Return status values that the simple event callback can return.
 */
enum {
	GPIOD_SIMPLE_EVENT_CB_RET_OK = 0,
	/**< Continue processing events. */
	GPIOD_SIMPLE_EVENT_CB_RET_STOP,
	/**< Stop processing events. */
};

/**
 * @brief Simple event callack signature.
 *
 * The callback function takes the following arguments: event type (int),
 * GPIO line offset (unsigned int), event timestamp (const struct timespec *)
 * and a pointer to user data (void *).
 */
typedef int (*gpiod_simple_event_handle_cb)(int, unsigned int,
					    const struct timespec *, void *);

/**
 * @brief Return status values that the simple event poll callback can return.
 */
enum {
	GPIOD_SIMPLE_EVENT_POLL_RET_ERR = -1,
	/**< Polling error occurred (the polling function should set errno). */
	GPIOD_SIMPLE_EVENT_POLL_RET_TIMEOUT = 0,
	/**< Poll timed out. */
	GPIOD_SIMPLE_EVENT_POLL_RET_EVENT = 1,
	/**< Line event occurred. */
	GPIOD_SIMPLE_EVENT_POLL_RET_STOP = 2,
	/**< The event loop should stop processing events. */
};

/**
 * @brief Simple event poll callback signature.
 *
 * The poll callback function takes the following arguments: number of lines
 * (unsigned int), an array of file descriptors on which input events should
 * be monitored (const int *), pointer to an integer which the function should
 * set to the offset in the fd array corresponding with the descriptor on which
 * an event occurred (int *), poll timeout (const struct timespec *) and a
 * pointer to user data (void *).
 *
 * The callback should poll for input events on the set of descriptors and
 * return an appropriate value that can be interpreted by the event loop
 * routine.
 */
typedef int (*gpiod_simple_event_poll_cb)(unsigned int,
					  const int *, unsigned int *,
					  const struct timespec *, void *);

/**
 * @brief Wait for events on a single GPIO line.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offset GPIO line offset to monitor.
 * @param active_low The active state of this line - true if low.
 * @param timeout Maximum wait time for each iteration.
 * @param poll_cb Callback function to call when waiting for events.
 * @param event_cb Callback function to call on event occurrence.
 * @param data User data passed to the callback.
 * @return 0 no errors were encountered, -1 if an error occurred.
 *
 * The poll callback can be NULL in which case the routine will fall back to
 * a basic, ppoll() based callback.
 */
int gpiod_simple_event_loop(const char *consumer, const char *device,
			    unsigned int offset, bool active_low,
			    const struct timespec *timeout,
			    gpiod_simple_event_poll_cb poll_cb,
			    gpiod_simple_event_handle_cb event_cb,
			    void *data) GPIOD_API;

/**
 * @brief Wait for events on multiple GPIO lines.
 * @param consumer Name of the consumer.
 * @param device Name, path or number of the gpiochip.
 * @param offsets Array of GPIO line offsets to monitor.
 * @param num_lines Number of lines to monitor.
 * @param active_low The active state of this line - true if low.
 * @param timeout Maximum wait time for each iteration.
 * @param poll_cb Callback function to call when waiting for events.
 * @param event_cb Callback function to call on event occurrence.
 * @param data User data passed to the callback.
 * @return 0 no errors were encountered, -1 if an error occurred.
 *
 * The callback functions work just like in the single line variant.
 */
int gpiod_simple_event_loop_multiple(const char *consumer, const char *device,
				     const unsigned int *offsets,
				     unsigned int num_lines, bool active_low,
				     const struct timespec *timeout,
				     gpiod_simple_event_poll_cb poll_cb,
				     gpiod_simple_event_handle_cb event_cb,
				     void *data) GPIOD_API;

/**
 * @brief Determine the chip name and line offset of a line with given name.
 * @param name The name of the GPIO line to lookup.
 * @param chipname Buffer in which the name of the GPIO chip will be stored.
 * @param chipname_size Size of the chip name buffer.
 * @param offset Pointer to an integer in which the line offset will be stored.
 * @return -1 on error, 0 if the line with given name doesn't exist and 1 if
 *         the line was found. In the first two cases the contents of chipname
 *         and offset remain unchanged.
 *
 * The chip name is truncated if the buffer can't hold its entire size.
 */
int gpiod_simple_find_line(const char *name, char *chipname,
			   size_t chipname_size,
			   unsigned int *offset) GPIOD_API;

/**
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
 * @brief Open a gpiochip by label.
 * @param label Label of the gpiochip to open.
 * @return GPIO chip handle or NULL if the chip with given label was not found
 *         or an error occured.
 */
struct gpiod_chip * gpiod_chip_open_by_label(const char *label) GPIOD_API;

/**
 * @brief Open a gpiochip based on the best guess what the path is.
 * @param descr String describing the gpiochip.
 * @return GPIO chip handle or NULL if an error occurred.
 *
 * This routine tries to figure out whether the user passed it the path to
 * the GPIO chip, its name, label or number as a string. Then it tries to
 * open it using one of the other gpiod_chip_open** routines.
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
 * @brief Find a GPIO line by name among lines associated with given GPIO chip.
 * @param chip The GPIO chip object.
 * @param name The name of the GPIO line.
 * @return Pointer to the GPIO line handle or NULL if the line could not be
 *         found or an error occurred.
 *
 * In case a line with given name is not associated with given chip, the
 * functions sets errno to ENOENT.
 */
struct gpiod_line *
gpiod_chip_find_line(struct gpiod_chip *chip, const char *name) GPIOD_API;

/**
 * @}
 *
 * @defgroup __lines__ GPIO line operations
 * @{
 *
 * Functions and data structures dealing with GPIO lines.
 *
 * @defgroup __line_bulk__ Operating on multiple lines
 * @{
 */

/**
 * @brief Maximum number of GPIO lines that can be requested at once.
 */
#define GPIOD_LINE_BULK_MAX_LINES		64

/**
 * @brief Helper structure for storing a set of GPIO line objects.
 *
 * This structure is used in all operations involving sets of GPIO lines. If
 * a bulk object is being passed to a function while containing zero lines,
 * the result is undefined.
 */
struct gpiod_line_bulk {
	struct gpiod_line *lines[GPIOD_LINE_BULK_MAX_LINES];
	/**< Buffer for line pointers. */
	unsigned int num_lines;
	/**< Number of lines currently held in this structure. */
};

/**
 * @brief Static initializer for GPIO bulk objects.
 *
 * This macro simply sets the internally held number of lines to 0.
 */
#define GPIOD_LINE_BULK_INITIALIZER	{ { NULL }, 0 }

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
 * @brief Retrieve the line handle from a line bulk object at given offset.
 * @param bulk Line bulk object.
 * @param offset Line offset.
 * @return Line handle at given offset.
 */
static inline struct gpiod_line *
gpiod_line_bulk_get_line(struct gpiod_line_bulk *bulk, unsigned int offset)
{
	return bulk->lines[offset];
}

/**
 * @brief Retrieve the number of GPIO lines held by this line bulk object.
 * @param bulk Line bulk object.
 * @return Number of lines held by this line bulk.
 */
static inline unsigned int
gpiod_line_bulk_num_lines(struct gpiod_line_bulk *bulk)
{
	return bulk->num_lines;
}

/**
 * @brief Iterate over all line handles held by a line bulk object.
 * @param bulk Line bulk object.
 * @param line GPIO line handle. On each iteration, the subsequent line handle
 *             is assigned to this pointer.
 * @param lineptr Pointer to a GPIO line handle used to store the loop state.
 */
#define gpiod_line_bulk_foreach_line(bulk, line, lineptr)		\
	for ((lineptr) = (bulk)->lines, (line) = *(lineptr);		\
	     (lineptr) <= (bulk)->lines + ((bulk)->num_lines - 1);	\
	     (lineptr)++, (line) = *(lineptr))

/**
 * @}
 *
 * @defgroup __line_info__ Line info
 * @{
 */

/**
 * @brief Possible direction settings.
 */
enum {
	GPIOD_LINE_DIRECTION_INPUT,
	/**< Direction is input - we're reading the state of a GPIO line. */
	GPIOD_LINE_DIRECTION_OUTPUT,
	/**< Direction is output - we're driving the GPIO line. */
};

/**
 * @brief Possible active state settings.
 */
enum {
	GPIOD_LINE_ACTIVE_STATE_HIGH,
	/**< The active state of a GPIO is active-high. */
	GPIOD_LINE_ACTIVE_STATE_LOW,
	/**< The active state of a GPIO is active-low. */
};

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
 * @brief Check if the line is currently in use.
 * @param line GPIO line object.
 * @return True if the line is in use, false otherwise.
 *
 * The user space can't know exactly why a line is busy. It may have been
 * requested by another process or hogged by the kernel. It only matters that
 * the line is used and we can't request it.
 */
bool gpiod_line_is_used(struct gpiod_line *line) GPIOD_API;

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
 * @}
 *
 * @defgroup __line_request__ Line requests
 * @{
 */

/**
 * @brief Available types of requests.
 */
enum {
	GPIOD_LINE_REQUEST_DIRECTION_AS_IS,
	/**< Request the line(s), but don't change current direction. */
	GPIOD_LINE_REQUEST_DIRECTION_INPUT,
	/**< Request the line(s) for reading the GPIO line state. */
	GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
	/**< Request the line(s) for setting the GPIO line state. */
	GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE,
	/**< Monitor both types of events. */
	GPIOD_LINE_REQUEST_EVENT_RISING_EDGE,
	/**< Only watch rising edge events. */
	GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES,
	/**< Only watch falling edge events. */
};

/**
 * @brief Miscellaneous GPIO request flags.
 */
enum {
	GPIOD_LINE_REQUEST_OPEN_DRAIN	= GPIOD_BIT(0),
	/**< The line is an open-drain port. */
	GPIOD_LINE_REQUEST_OPEN_SOURCE	= GPIOD_BIT(1),
	/**< The line is an open-source port. */
	GPIOD_LINE_REQUEST_ACTIVE_LOW	= GPIOD_BIT(2),
	/**< The active state of the line is low (high is the default). */
};

/**
 * @brief Structure holding configuration of a line request.
 */
struct gpiod_line_request_config {
	const char *consumer;
	/**< Name of the consumer. */
	int request_type;
	/**< Request type. */
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
 * If this routine succeeds, the caller takes ownership of the GPIO line until
 * it's released.
 */
int gpiod_line_request(struct gpiod_line *line,
		       const struct gpiod_line_request_config *config,
		       int default_val) GPIOD_API;

/**
 * @brief Reserve a single line, set the direction to input.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @return 0 if the line was properly reserved, -1 on failure.
 */
int gpiod_line_request_input(struct gpiod_line *line,
			     const char *consumer) GPIOD_API;

/**
 * @brief Reserve a single line, set the direction to output.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param default_val Default line value.
 * @return 0 if the line was properly reserved, -1 on failure.
 */
int gpiod_line_request_output(struct gpiod_line *line,
			      const char *consumer, int default_val) GPIOD_API;

/**
 * @brief Request rising edge event notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_rising_edge_events(struct gpiod_line *line,
					  const char *consumer) GPIOD_API;

/**
 * @brief Request falling edge event notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_falling_edge_events(struct gpiod_line *line,
					   const char *consumer) GPIOD_API;

/**
 * @brief Request all event type notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_both_edges_events(struct gpiod_line *line,
					 const char *consumer) GPIOD_API;

/**
 * @brief Reserve a single line, set the direction to input.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the line was properly reserved, -1 on failure.
 */
int gpiod_line_request_input_flags(struct gpiod_line *line,
				   const char *consumer, int flags) GPIOD_API;

/**
 * @brief Reserve a single line, set the direction to output.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @param default_val Default line value.
 * @return 0 if the line was properly reserved, -1 on failure.
 */
int gpiod_line_request_output_flags(struct gpiod_line *line,
				    const char *consumer, int flags,
				    int default_val) GPIOD_API;

/**
 * @brief Request rising edge event notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_rising_edge_events_flags(struct gpiod_line *line,
						const char *consumer,
						int flags) GPIOD_API;

/**
 * @brief Request falling edge event notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_falling_edge_events_flags(struct gpiod_line *line,
						 const char *consumer,
						 int flags) GPIOD_API;

/**
 * @brief Request all event type notifications on a single line.
 * @param line GPIO line object.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_both_edges_events_flags(struct gpiod_line *line,
					       const char *consumer,
					       int flags) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines.
 * @param bulk Set of GPIO lines to reserve.
 * @param config Request options.
 * @param default_vals Default line values - only relevant if we're setting
 *        the direction to output.
 * @return 0 if the all lines were properly requested. In case of an error
 *         this routine returns -1 and sets the last error number.
 *
 * If this routine succeeds, the caller takes ownership of the GPIO lines
 * until they're released. All the requested lines must be prodivided by the
 * same gpiochip.
 */
int gpiod_line_request_bulk(struct gpiod_line_bulk *bulk,
			    const struct gpiod_line_request_config *config,
			    const int *default_vals) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines, set the direction to input.
 * @param bulk Set of GPIO lines to reserve.
 * @param consumer Name of the consumer.
 * @return 0 if the lines were properly reserved, -1 on failure.
 */
int gpiod_line_request_bulk_input(struct gpiod_line_bulk *bulk,
				  const char *consumer) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines, set the direction to output.
 * @param bulk Set of GPIO lines to reserve.
 * @param consumer Name of the consumer.
 * @param default_vals Default line values.
 * @return 0 if the lines were properly reserved, -1 on failure.
 */
int gpiod_line_request_bulk_output(struct gpiod_line_bulk *bulk,
				   const char *consumer,
				   const int *default_vals) GPIOD_API;

/**
 * @brief Request rising edge event notifications on a set of lines.
 * @param bulk Set of GPIO lines to request.
 * @param consumer Name of the consumer.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_bulk_rising_edge_events(struct gpiod_line_bulk *bulk,
					       const char *consumer) GPIOD_API;

/**
 * @brief Request falling edge event notifications on a set of lines.
 * @param bulk Set of GPIO lines to request.
 * @param consumer Name of the consumer.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_bulk_falling_edge_events(struct gpiod_line_bulk *bulk,
						const char *consumer) GPIOD_API;

/**
 * @brief Request all event type notifications on a set of lines.
 * @param bulk Set of GPIO lines to request.
 * @param consumer Name of the consumer.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_bulk_both_edges_events(struct gpiod_line_bulk *bulk,
					      const char *consumer) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines, set the direction to input.
 * @param bulk Set of GPIO lines to reserve.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the lines were properly reserved, -1 on failure.
 */
int gpiod_line_request_bulk_input_flags(struct gpiod_line_bulk *bulk,
					const char *consumer,
					int flags) GPIOD_API;

/**
 * @brief Reserve a set of GPIO lines, set the direction to output.
 * @param bulk Set of GPIO lines to reserve.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @param default_vals Default line values.
 * @return 0 if the lines were properly reserved, -1 on failure.
 */
int gpiod_line_request_bulk_output_flags(struct gpiod_line_bulk *bulk,
					 const char *consumer, int flags,
					 const int *default_vals) GPIOD_API;

/**
 * @brief Request rising edge event notifications on a set of lines.
 * @param bulk Set of GPIO lines to request.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_bulk_rising_edge_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer,
					int flags) GPIOD_API;

/**
 * @brief Request falling edge event notifications on a set of lines.
 * @param bulk Set of GPIO lines to request.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_bulk_falling_edge_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer,
					int flags) GPIOD_API;

/**
 * @brief Request all event type notifications on a set of lines.
 * @param bulk Set of GPIO lines to request.
 * @param consumer Name of the consumer.
 * @param flags Additional request flags.
 * @return 0 if the operation succeeds, -1 on failure.
 */
int gpiod_line_request_bulk_both_edges_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer,
					int flags) GPIOD_API;

/**
 * @brief Release a previously reserved line.
 * @param line GPIO line object.
 */
void gpiod_line_release(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Release a set of previously reserved lines.
 * @param bulk Set of GPIO lines to release.
 *
 * If the lines were not previously requested together, the behavior is
 * undefined.
 */
void gpiod_line_release_bulk(struct gpiod_line_bulk *bulk) GPIOD_API;

/**
 * @brief Check if the calling user has ownership of this line.
 * @param line GPIO line object.
 * @return True if given line was requested, false otherwise.
 */
bool gpiod_line_is_requested(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Check if the calling user has neither requested ownership of this
 *        line nor configured any event notifications.
 * @param line GPIO line object.
 * @return True if given line is free, false otherwise.
 */
bool gpiod_line_is_free(struct gpiod_line *line) GPIOD_API;

/**
 * @}
 *
 * @defgroup __line_value__ Reading & setting line values
 * @{
 */

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
 * the same order, the lines are added to line_bulk. If the lines were not
 * previously requested together, the behavior is undefined.
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
 *
 * If the lines were not previously requested together, the behavior is
 * undefined.
 */
int gpiod_line_set_value_bulk(struct gpiod_line_bulk *bulk,
			      int *values) GPIOD_API;

/**
 * @}
 *
 * @defgroup __line_event__ Line events handling
 * @{
 */

/**
 * @brief Event types.
 */
enum {
	GPIOD_LINE_EVENT_RISING_EDGE,
	/**< Rising edge event. */
	GPIOD_LINE_EVENT_FALLING_EDGE,
	/**< Falling edge event. */
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
 * @brief Wait for an event on a single line.
 * @param line GPIO line object.
 * @param timeout Wait time limit.
 * @return 0 if wait timed out, -1 if an error occurred, 1 if an event
 *         occurred.
 */
int gpiod_line_event_wait(struct gpiod_line *line,
			  const struct timespec *timeout) GPIOD_API;

/**
 * @brief Wait for events on a set of lines.
 * @param bulk Set of GPIO lines to monitor.
 * @param timeout Wait time limit.
 * @param event_bulk Bulk object in which to store the line handles on which
 *        events occurred. Can be NULL.
 * @return 0 if wait timed out, -1 if an error occurred, 1 if at least one
 *         event occurred.
 */
int gpiod_line_event_wait_bulk(struct gpiod_line_bulk *bulk,
			       const struct timespec *timeout,
			       struct gpiod_line_bulk *event_bulk) GPIOD_API;

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
 * @defgroup __line_misc__ Misc line functions
 * @{
 */

/**
 * @brief Get a GPIO line handle by GPIO chip description and offset.
 * @param device String describing the gpiochip.
 * @param offset The offset of the GPIO line.
 * @return GPIO line handle or NULL if an error occurred.
 *
 * This routine provides a shorter alternative to calling
 * ::gpiod_chip_open_lookup and ::gpiod_chip_get_line.
 *
 * If this function succeeds, the caller is responsible for closing the
 * associated GPIO chip.
 */
struct gpiod_line *
gpiod_line_get(const char *device, unsigned int offset) GPIOD_API;

/**
 * @brief Find a GPIO line by its name.
 * @param name Name of the GPIO line.
 * @return Returns the GPIO line handle if the line exists in the system or
 *         NULL if it couldn't be located or an error occurred.
 *
 * If this routine succeeds, the user must manually close the GPIO chip owning
 * this line to avoid memory leaks. If the line could not be found, this
 * functions sets errno to ENOENT.
 */
struct gpiod_line * gpiod_line_find(const char *name) GPIOD_API;

/**
 * @brief Close a GPIO chip owning this line and release all resources.
 * @param line GPIO line object
 *
 * After this function returns, the line must no longer be used.
 */
void gpiod_line_close_chip(struct gpiod_line *line) GPIOD_API;

/**
 * @brief Get the handle to the GPIO chip controlling this line.
 * @param line The GPIO line object.
 * @return Pointer to the GPIO chip handle controlling this line.
 */
struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line) GPIOD_API;

/**
 * @}
 *
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
 * @brief Get the next gpiochip handle without closing the previous one.
 * @param iter The gpiochip iterator object.
 * @return Pointer to an open gpiochip handle or NULL if the next chip can't
 *         be accessed.
 *
 * Internally this routine scans the /dev/ directory, storing current state
 * in the chip iterator structure, and tries to open the next /dev/gpiochipX
 * device file. If an error occurs or no more chips are present, the function
 * returns NULL.
 * Note: The user is responsible for closing the chips after use.
 */
struct gpiod_chip *
gpiod_chip_iter_next_noclose(struct gpiod_chip_iter *iter) GPIOD_API;

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
 * @brief Iterate over all gpiochip present in the system without closing them.
 * @param iter An initialized GPIO chip iterator.
 * @param chip Pointer to a GPIO chip handle. On each iteration the newly
 *             opened chip handle is assigned to this argument.
 *
 * The user must close the GPIO chip manually after use, until then, the chip
 * remains open. Free the iterator by calling gpiod_chip_iter_free_noclose to
 * avoid closing the last chip automatically.
 */
#define gpiod_foreach_chip_noclose(iter, chip)				\
	for ((chip) = gpiod_chip_iter_next_noclose(iter);		\
	     !gpiod_chip_iter_done(iter);				\
	     (chip) = gpiod_chip_iter_next_noclose(iter))

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
	GPIOD_LINE_ITER_STATE_INIT = 0,
	/**< Line iterator is initiated or iterating over lines. */
	GPIOD_LINE_ITER_STATE_DONE,
	/**< Line iterator is done with all lines on this chip. */
	GPIOD_LINE_ITER_STATE_ERR,
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
#define GPIOD_LINE_ITER_INITIALIZER(chip)				\
	{ 0, (chip), GPIOD_LINE_ITER_STATE_INIT }

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
	iter->state = GPIOD_LINE_ITER_STATE_INIT;
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
	return iter->state == GPIOD_LINE_ITER_STATE_DONE;
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
	return iter->state == GPIOD_LINE_ITER_STATE_ERR;
}

/**
 * @brief Get the offset of the last line we tried to open.
 * @param iter The GPIO line iterator object.
 * @return The offset of the last line we tried to open - whether we failed
 *         or succeeded to do so.
 *
 * If this function is called before gpiod_line_iter_next() is called at least
 * once, the results are undefined.
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
