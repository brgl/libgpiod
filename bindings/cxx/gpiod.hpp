/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#ifndef __LIBGPIOD_GPIOD_CXX_HPP__
#define __LIBGPIOD_GPIOD_CXX_HPP__

#include <bitset>
#include <chrono>
#include <gpiod.h>
#include <memory>
#include <string>
#include <vector>

namespace gpiod {

class line;
class line_bulk;
class line_event;
class line_iter;
class chip_iter;

/**
 * @defgroup __gpiod_cxx__ C++ bindings
 * @{
 */

/**
 * @brief Represents a GPIO chip.
 *
 * Internally this class holds a smart pointer to an open GPIO chip descriptor.
 * Multiple objects of this class can reference the same chip. The chip is
 * closed and all resources freed when the last reference is dropped.
 */
class chip
{
public:

	/**
	 * @brief Default constructor. Creates an empty GPIO chip object.
	 */
	GPIOD_API chip(void) = default;

	/**
	 * @brief Constructor. Opens the chip using chip::open.
	 * @param device String describing the GPIO chip.
	 * @param how Indicates how the chip should be opened.
	 */
	GPIOD_API chip(const ::std::string& device, int how = OPEN_LOOKUP);

	/**
	 * @brief Copy constructor. References the object held by other.
	 * @param other Other chip object.
	 */
	GPIOD_API chip(const chip& other) = default;

	/**
	 * @brief Move constructor. References the object held by other.
	 * @param other Other chip object.
	 */
	GPIOD_API chip(chip&& other) = default;

	/**
	 * @brief Assignment operator. References the object held by other.
	 * @param other Other chip object.
	 * @return Reference to this object.
	 */
	GPIOD_API chip& operator=(const chip& other) = default;

	/**
	 * @brief Move assignment operator. References the object held by other.
	 * @param other Other chip object.
	 * @return Reference to this object.
	 */
	GPIOD_API chip& operator=(chip&& other) = default;

	/**
	 * @brief Destructor. Unreferences the internal chip object.
	 */
	GPIOD_API ~chip(void) = default;

	/**
	 * @brief Open a GPIO chip.
	 * @param device String describing the GPIO chip.
	 * @param how Indicates how the chip should be opened.
	 *
	 * If the object already holds a reference to an open chip, it will be
	 * closed and the reference reset.
	 */
	GPIOD_API void open(const ::std::string &device, int how = OPEN_LOOKUP);

	/**
	 * @brief Reset the internal smart pointer owned by this object.
	 */
	GPIOD_API void reset(void) noexcept;

	/**
	 * @brief Return the name of the chip held by this object.
	 * @return Name of the GPIO chip.
	 */
	GPIOD_API ::std::string name(void) const;

	/**
	 * @brief Return the label of the chip held by this object.
	 * @return Label of the GPIO chip.
	 */
	GPIOD_API ::std::string label(void) const;

	/**
	 * @brief Return the number of lines exposed by this chip.
	 * @return Number of lines.
	 */
	GPIOD_API unsigned int num_lines(void) const;

	/**
	 * @brief Get the line exposed by this chip at given offset.
	 * @param offset Offset of the line.
	 * @return Line object.
	 */
	GPIOD_API line get_line(unsigned int offset) const;

	/**
	 * @brief Get the line exposed by this chip by name.
	 * @param name Line name.
	 * @return Line object.
	 */
	GPIOD_API line find_line(const ::std::string& name) const;

	/**
	 * @brief Get a set of lines exposed by this chip at given offsets.
	 * @param offsets Vector of line offsets.
	 * @return Set of lines held by a line_bulk object.
	 */
	GPIOD_API line_bulk get_lines(const ::std::vector<unsigned int>& offsets) const;

	/**
	 * @brief Get all lines exposed by this chip.
	 * @return All lines exposed by this chip held by a line_bulk object.
	 */
	GPIOD_API line_bulk get_all_lines(void) const;

	/**
	 * @brief Get a set of lines exposed by this chip by their names.
	 * @param names Vector of line names.
	 * @return Set of lines held by a line_bulk object.
	 */
	GPIOD_API line_bulk find_lines(const ::std::vector<::std::string>& names) const;

	/**
	 * @brief Equality operator.
	 * @param rhs Right-hand side of the equation.
	 * @return True if rhs references the same chip. False otherwise.
	 */
	GPIOD_API bool operator==(const chip& rhs) const noexcept;

	/**
	 * @brief Inequality operator.
	 * @param rhs Right-hand side of the equation.
	 * @return False if rhs references the same chip. True otherwise.
	 */
	GPIOD_API bool operator!=(const chip& rhs) const noexcept;

	/**
	 * @brief Check if this object holds a reference to a GPIO chip.
	 * @return True if this object references a GPIO chip, false otherwise.
	 */
	GPIOD_API explicit operator bool(void) const noexcept;

	/**
	 * @brief Check if this object doesn't hold a reference to a GPIO chip.
	 * @return False if this object references a GPIO chip, true otherwise.
	 */
	GPIOD_API bool operator!(void) const noexcept;

	/**
	 * @brief Affect the way in which chip::chip and chip::open will try
	 *        to open a GPIO chip character device.
	 */
	enum : int {
		OPEN_LOOKUP = 1,
		/**< Open based on the best guess what the supplied string is. */
		OPEN_BY_PATH,
		/**< Assume the string is a path to the GPIO chardev. */
		OPEN_BY_NAME,
		/**< Assume the string is the name of the chip */
		OPEN_BY_LABEL,
		/**< Assume the string is the label of the GPIO chip. */
		OPEN_BY_NUMBER,
		/**< Assume the string is the number of the GPIO chip. */
	};

private:

	chip(::gpiod_chip* chip);

	void throw_if_noref(void) const;

	::std::shared_ptr<::gpiod_chip> _m_chip;

	friend chip_iter;
	friend line_iter;
};

/**
 * @brief Stores the configuration for line requests.
 */
struct line_request
{
	/**
	 * @brief Request types.
	 */
	enum : int {
		DIRECTION_AS_IS = 1,
		/**< Request for values, don't change the direction. */
		DIRECTION_INPUT,
		/**< Request for reading line values. */
		DIRECTION_OUTPUT,
		/**< Request for driving the GPIO lines. */
		EVENT_FALLING_EDGE,
		/**< Listen for falling edge events. */
		EVENT_RISING_EDGE,
		/**< Listen for rising edge events. */
		EVENT_BOTH_EDGES,
		/**< Listen for all types of events. */
	};

	GPIOD_API static const ::std::bitset<32> FLAG_ACTIVE_LOW;
	/**< Set the active state to 'low' (high is the default). */
	GPIOD_API static const ::std::bitset<32> FLAG_OPEN_SOURCE;
	/**< The line is an open-source port. */
	GPIOD_API static const ::std::bitset<32> FLAG_OPEN_DRAIN;
	/**< The line is an open-drain port. */
	GPIOD_API static const ::std::bitset<32> FLAG_BIAS_DISABLE;
	/**< The line has neither pull-up nor pull-down resistor enabled. */
	GPIOD_API static const ::std::bitset<32> FLAG_BIAS_PULL_DOWN;
	/**< The line has a configurable pull-down resistor enabled. */
	GPIOD_API static const ::std::bitset<32> FLAG_BIAS_PULL_UP;
	/**< The line has a configurable pull-up resistor enabled. */

	::std::string consumer;
	/**< Consumer name to pass to the request. */
	int request_type;
	/**< Type of the request. */
	::std::bitset<32> flags;
	/**< Additional request flags. */
};

/**
 * @brief Represents a single GPIO line.
 *
 * Internally this class holds a raw pointer to a GPIO line descriptor and a
 * reference to the parent chip. All line resources are freed when the last
 * reference to the parent chip is dropped.
 */
class line
{
public:

	/**
	 * @brief Default constructor. Creates an empty line object.
	 */
	GPIOD_API line(void);

	/**
	 * @brief Copy constructor.
	 * @param other Other line object.
	 */
	GPIOD_API line(const line& other) = default;

	/**
	 * @brief Move constructor.
	 * @param other Other line object.
	 */
	GPIOD_API line(line&& other) = default;

	/**
	 * @brief Assignment operator.
	 * @param other Other line object.
	 * @return Reference to this object.
	 */
	GPIOD_API line& operator=(const line& other) = default;

	/**
	 * @brief Move assignment operator.
	 * @param other Other line object.
	 * @return Reference to this object.
	 */
	GPIOD_API line& operator=(line&& other) = default;

	/**
	 * @brief Destructor.
	 */
	GPIOD_API ~line(void) = default;

	/**
	 * @brief Get the offset of this line.
	 * @return Offet of this line.
	 */
	GPIOD_API unsigned int offset(void) const;

	/**
	 * @brief Get the name of this line (if any).
	 * @return Name of this line or an empty string if it is unnamed.
	 */
	GPIOD_API ::std::string name(void) const;

	/**
	 * @brief Get the consumer of this line (if any).
	 * @return Name of the consumer of this line or an empty string if it
	 *         is unused.
	 */
	GPIOD_API ::std::string consumer(void) const;

	/**
	 * @brief Get current direction of this line.
	 * @return Current direction setting.
	 */
	GPIOD_API int direction(void) const;

	/**
	 * @brief Get current active state of this line.
	 * @return Current active state setting.
	 */
	GPIOD_API int active_state(void) const;

	/**
	 * @brief Get current bias of this line.
	 * @return Current bias setting.
	 */
	GPIOD_API int bias(void) const;

	/**
	 * @brief Check if this line is used by the kernel or other user space
	 *        process.
	 * @return True if this line is in use, false otherwise.
	 */
	GPIOD_API bool is_used(void) const;

	/**
	 * @brief Check if this line represents an open-drain GPIO.
	 * @return True if the line is an open-drain GPIO, false otherwise.
	 */
	GPIOD_API bool is_open_drain(void) const;

	/**
	 * @brief Check if this line represents an open-source GPIO.
	 * @return True if the line is an open-source GPIO, false otherwise.
	 */
	GPIOD_API bool is_open_source(void) const;

	/**
	 * @brief Request this line.
	 * @param config Request config (see gpiod::line_request).
	 * @param default_val Default value - only matters for OUTPUT direction.
	 */
	GPIOD_API void request(const line_request& config, int default_val = 0) const;

	/**
	 * @brief Release the line if it was previously requested.
	 */
	GPIOD_API void release(void) const;

	/**
	 * @brief Check if this user has ownership of this line.
	 * @return True if the user has ownership of this line, false otherwise.
	 */
	GPIOD_API bool is_requested(void) const;

	/**
	 * @brief Read the line value.
	 * @return Current value (0 or 1).
	 */
	GPIOD_API int get_value(void) const;

	/**
	 * @brief Set the value of this line.
	 * @param val New value (0 or 1).
	 */
	GPIOD_API void set_value(int val) const;

	/**
	 * @brief Wait for an event on this line.
	 * @param timeout Time to wait before returning if no event occurred.
	 * @return True if an event occurred and can be read, false if the wait
	 *         timed out.
	 */
	GPIOD_API bool event_wait(const ::std::chrono::nanoseconds& timeout) const;

	/**
	 * @brief Read a line event.
	 * @return Line event object.
	 */
	GPIOD_API line_event event_read(void) const;

	/**
	 * @brief Get the event file descriptor associated with this line.
	 * @return File descriptor number.
	 */
	GPIOD_API int event_get_fd(void) const;

	/**
	 * @brief Get the reference to the parent chip.
	 * @return Reference to the parent chip object.
	 */
	GPIOD_API const chip& get_chip(void) const;

	/**
	 * @brief Re-read the line info from the kernel.
	 */
	GPIOD_API void update(void) const;

	/**
	 * @brief Reset the state of this object.
	 *
	 * This is useful when the user needs to e.g. keep the line_event object
	 * but wants to drop the reference to the GPIO chip indirectly held by
	 * the line being the source of the event.
	 */
	GPIOD_API void reset(void);

	/**
	 * @brief Check if two line objects reference the same GPIO line.
	 * @param rhs Right-hand side of the equation.
	 * @return True if both objects reference the same line, fale otherwise.
	 */
	GPIOD_API bool operator==(const line& rhs) const noexcept;

	/**
	 * @brief Check if two line objects reference different GPIO lines.
	 * @param rhs Right-hand side of the equation.
	 * @return False if both objects reference the same line, true otherwise.
	 */
	GPIOD_API bool operator!=(const line& rhs) const noexcept;

	/**
	 * @brief Check if this object holds a reference to any GPIO line.
	 * @return True if this object references a GPIO line, false otherwise.
	 */
	GPIOD_API explicit operator bool(void) const noexcept;

	/**
	 * @brief Check if this object doesn't reference any GPIO line.
	 * @return True if this object doesn't reference any GPIO line, true
	 *         otherwise.
	 */
	GPIOD_API bool operator!(void) const noexcept;

	/**
	 * @brief Possible direction settings.
	 */
	enum : int {
		DIRECTION_INPUT = 1,
		/**< Line's direction setting is input. */
		DIRECTION_OUTPUT,
		/**< Line's direction setting is output. */
	};

	/**
	 * @brief Possible active state settings.
	 */
	enum : int {
		ACTIVE_LOW = 1,
		/**< Line's active state is low. */
		ACTIVE_HIGH,
		/**< Line's active state is high. */
	};

	/**
	 * @brief Possible bias settings.
	 */
	enum : int {
		BIAS_AS_IS = 1,
		/**< Line's bias state is unknown. */
		BIAS_DISABLE,
		/**< Line's internal bias is disabled. */
		BIAS_PULL_UP,
		/**< Line's internal pull-up bias is enabled. */
		BIAS_PULL_DOWN,
		/**< Line's internal pull-down bias is enabled. */
	};

private:

	line(::gpiod_line* line, const chip& owner);

	void throw_if_null(void) const;

	::gpiod_line* _m_line;
	chip _m_chip;

	friend chip;
	friend line_bulk;
	friend line_iter;
};

/**
 * @brief Find a GPIO line by name. Search all GPIO chips present on the system.
 * @param name Name of the line.
 * @return Returns a line object - empty if the line was not found.
 */
GPIOD_API line find_line(const ::std::string& name);

/**
 * @brief Describes a single GPIO line event.
 */
struct line_event
{

	/**
	 * @brief Possible event types.
	 */
	enum : int {
		RISING_EDGE = 1,
		/**< Rising edge event. */
		FALLING_EDGE,
		/**< Falling edge event. */
	};

	::std::chrono::nanoseconds timestamp;
	/**< Best estimate of time of event occurrence in nanoseconds. */
	int event_type;
	/**< Type of the event that occurred. */
	line source;
	/**< Line object referencing the GPIO line on which the event occurred. */
};

/**
 * @brief Represents a set of GPIO lines.
 *
 * Internally an object of this class stores an array of line objects
 * owned by a single chip.
 */
class line_bulk
{
public:

	/**
	 * @brief Default constructor. Creates an empty line_bulk object.
	 */
	GPIOD_API line_bulk(void) = default;

	/**
	 * @brief Construct a line_bulk from a vector of lines.
	 * @param lines Vector of gpiod::line objects.
	 * @note All lines must be owned by the same GPIO chip.
	 */
	GPIOD_API line_bulk(const ::std::vector<line>& lines);

	/**
	 * @brief Copy constructor.
	 * @param other Other line_bulk object.
	 */
	GPIOD_API line_bulk(const line_bulk& other) = default;

	/**
	 * @brief Move constructor.
	 * @param other Other line_bulk object.
	 */
	GPIOD_API line_bulk(line_bulk&& other) = default;

	/**
	 * @brief Assignment operator.
	 * @param other Other line_bulk object.
	 * @return Reference to this object.
	 */
	GPIOD_API line_bulk& operator=(const line_bulk& other) = default;

	/**
	 * @brief Move assignment operator.
	 * @param other Other line_bulk object.
	 * @return Reference to this object.
	 */
	GPIOD_API line_bulk& operator=(line_bulk&& other) = default;

	/**
	 * @brief Destructor.
	 */
	GPIOD_API ~line_bulk(void) = default;

	/**
	 * @brief Add a line to this line_bulk object.
	 * @param new_line Line to add.
	 * @note The new line must be owned by the same chip as all the other
	 *       lines already held by this line_bulk object.
	 */
	GPIOD_API void append(const line& new_line);

	/**
	 * @brief Get the line at given offset.
	 * @param offset Offset of the line to get.
	 * @return Reference to the line object.
	 */
	GPIOD_API line& get(unsigned int offset);

	/**
	 * @brief Get the line at given offset without bounds checking.
	 * @param offset Offset of the line to get.
	 * @return Reference to the line object.
	 * @note No bounds checking is performed.
	 */
	GPIOD_API line& operator[](unsigned int offset);

	/**
	 * @brief Get the number of lines currently held by this object.
	 * @return Number of elements in this line_bulk.
	 */
	GPIOD_API unsigned int size(void) const noexcept;

	/**
	 * @brief Check if this line_bulk doesn't hold any lines.
	 * @return True if this object is empty, false otherwise.
	 */
	GPIOD_API bool empty(void) const noexcept;

	/**
	 * @brief Remove all lines from this object.
	 */
	GPIOD_API void clear(void);

	/**
	 * @brief Request all lines held by this object.
	 * @param config Request config (see gpiod::line_request).
	 * @param default_vals Vector of default values. Only relevant for
	 *                     output direction requests.
	 */
	GPIOD_API void request(const line_request& config,
			       const std::vector<int> default_vals = std::vector<int>()) const;

	/**
	 * @brief Release all lines held by this object.
	 */
	GPIOD_API void release(void) const;

	/**
	 * @brief Read values from all lines held by this object.
	 * @return Vector containing line values the order of which corresponds
	 *         with the order of lines in the internal array.
	 */
	GPIOD_API ::std::vector<int> get_values(void) const;

	/**
	 * @brief Set values of all lines held by this object.
	 * @param values Vector of values to set. Must be the same size as the
	 *        number of lines held by this line_bulk.
	 */
	GPIOD_API void set_values(const ::std::vector<int>& values) const;

	/**
	 * @brief Poll the set of lines for line events.
	 * @param timeout Number of nanoseconds to wait before returning an
	 *        empty line_bulk.
	 * @return Returns a line_bulk object containing lines on which events
	 *         occurred.
	 */
	GPIOD_API line_bulk event_wait(const ::std::chrono::nanoseconds& timeout) const;

	/**
	 * @brief Check if this object holds any lines.
	 * @return True if this line_bulk holds at least one line, false otherwise.
	 */
	GPIOD_API explicit operator bool(void) const noexcept;

	/**
	 * @brief Check if this object doesn't hold any lines.
	 * @return True if this line_bulk is empty, false otherwise.
	 */
	GPIOD_API bool operator!(void) const noexcept;

	/**
	 * @brief Max number of lines that this object can hold.
	 */
	GPIOD_API static const unsigned int MAX_LINES;

	/**
	 * @brief Iterator for iterating over lines held by line_bulk.
	 */
	class iterator
	{
	public:

		/**
		 * @brief Default constructor. Builds an empty iterator object.
		 */
		GPIOD_API iterator(void) = default;

		/**
		 * @brief Copy constructor.
		 * @param other Other line_bulk iterator.
		 */
		GPIOD_API iterator(const iterator& other) = default;

		/**
		 * @brief Move constructor.
		 * @param other Other line_bulk iterator.
		 */
		GPIOD_API iterator(iterator&& other) = default;

		/**
		 * @brief Assignment operator.
		 * @param other Other line_bulk iterator.
		 * @return Reference to this iterator.
		 */
		GPIOD_API iterator& operator=(const iterator& other) = default;

		/**
		 * @brief Move assignment operator.
		 * @param other Other line_bulk iterator.
		 * @return Reference to this iterator.
		 */
		GPIOD_API iterator& operator=(iterator&& other) = default;

		/**
		 * @brief Destructor.
		 */
		GPIOD_API ~iterator(void) = default;

		/**
		 * @brief Advance the iterator by one element.
		 * @return Reference to this iterator.
		 */
		GPIOD_API iterator& operator++(void);

		/**
		 * @brief Dereference current element.
		 * @return Current GPIO line by reference.
		 */
		GPIOD_API const line& operator*(void) const;

		/**
		 * @brief Member access operator.
		 * @return Current GPIO line by pointer.
		 */
		GPIOD_API const line* operator->(void) const;

		/**
		 * @brief Check if this operator points to the same element.
		 * @param rhs Right-hand side of the equation.
		 * @return True if this iterator points to the same GPIO line,
		 *         false otherwise.
		 */
		GPIOD_API bool operator==(const iterator& rhs) const noexcept;

		/**
		 * @brief Check if this operator doesn't point to the same element.
		 * @param rhs Right-hand side of the equation.
		 * @return True if this iterator doesn't point to the same GPIO
		 *         line, false otherwise.
		 */
		GPIOD_API bool operator!=(const iterator& rhs) const noexcept;

	private:

		iterator(const ::std::vector<line>::iterator& it);

		::std::vector<line>::iterator _m_iter;

		friend line_bulk;
	};

	/**
	 * @brief Returns an iterator to the first line.
	 * @return A line_bulk iterator.
	 */
	GPIOD_API iterator begin(void) noexcept;

	/**
	 * @brief Returns an iterator to the element following the last line.
	 * @return A line_bulk iterator.
	 */
	GPIOD_API iterator end(void) noexcept;

private:

	void throw_if_empty(void) const;
	void to_line_bulk(::gpiod_line_bulk* bulk) const;

	::std::vector<line> _m_bulk;
};

/**
 * @brief Create a new chip_iter.
 * @return New chip iterator object pointing to the first GPIO chip on the system.
 * @note This function is needed as we already use the default constructor of
 *       gpiod::chip_iter as the return value of gpiod::end.
 */
GPIOD_API chip_iter make_chip_iter(void);

/**
 * @brief Support for range-based loops for chip iterators.
 * @param iter A chip iterator.
 * @return Iterator unchanged.
 */
GPIOD_API chip_iter begin(chip_iter iter) noexcept;

/**
 * @brief Support for range-based loops for chip iterators.
 * @param iter A chip iterator.
 * @return New end iterator.
 */
GPIOD_API chip_iter end(const chip_iter& iter) noexcept;

/**
 * @brief Allows to iterate over all GPIO chips present on the system.
 */
class chip_iter
{
public:

	/**
	 * @brief Default constructor. Creates the end iterator.
	 */
	GPIOD_API chip_iter(void) = default;

	/**
	 * @brief Copy constructor.
	 * @param other Other chip_iter.
	 */
	GPIOD_API chip_iter(const chip_iter& other) = default;

	/**
	 * @brief Move constructor.
	 * @param other Other chip_iter.
	 */
	GPIOD_API chip_iter(chip_iter&& other) = default;

	/**
	 * @brief Assignment operator.
	 * @param other Other chip_iter.
	 * @return Reference to this iterator.
	 */
	GPIOD_API chip_iter& operator=(const chip_iter& other) = default;

	/**
	 * @brief Move assignment operator.
	 * @param other Other chip_iter.
	 * @return Reference to this iterator.
	 */
	GPIOD_API chip_iter& operator=(chip_iter&& other) = default;

	/**
	 * @brief Destructor.
	 */
	GPIOD_API ~chip_iter(void) = default;

	/**
	 * @brief Advance the iterator by one element.
	 * @return Reference to this iterator.
	 */
	GPIOD_API chip_iter& operator++(void);

	/**
	 * @brief Dereference current element.
	 * @return Current GPIO chip by reference.
	 */
	GPIOD_API const chip& operator*(void) const;

	/**
	 * @brief Member access operator.
	 * @return Current GPIO chip by pointer.
	 */
	GPIOD_API const chip* operator->(void) const;

	/**
	 * @brief Check if this operator points to the same element.
	 * @param rhs Right-hand side of the equation.
	 * @return True if this iterator points to the same chip_iter,
	 *         false otherwise.
	 */
	GPIOD_API bool operator==(const chip_iter& rhs) const noexcept;

	/**
	 * @brief Check if this operator doesn't point to the same element.
	 * @param rhs Right-hand side of the equation.
	 * @return True if this iterator doesn't point to the same chip_iter,
	 *         false otherwise.
	 */
	GPIOD_API bool operator!=(const chip_iter& rhs) const noexcept;

private:

	chip_iter(::gpiod_chip_iter* iter);

	::std::shared_ptr<::gpiod_chip_iter> _m_iter;
	chip _m_current;

	friend chip_iter make_chip_iter(void);
};

/**
 * @brief Support for range-based loops for line iterators.
 * @param iter A line iterator.
 * @return Iterator unchanged.
 */
GPIOD_API line_iter begin(line_iter iter) noexcept;

/**
 * @brief Support for range-based loops for line iterators.
 * @param iter A line iterator.
 * @return New end iterator.
 */
GPIOD_API line_iter end(const line_iter& iter) noexcept;

/**
 * @brief Allows to iterate over all lines owned by a GPIO chip.
 */
class line_iter
{
public:

	/**
	 * @brief Default constructor. Creates the end iterator.
	 */
	GPIOD_API line_iter(void) = default;

	/**
	 * @brief Constructor. Creates the begin iterator.
	 * @param owner Chip owning the GPIO lines over which we want to iterate.
	 */
	GPIOD_API line_iter(const chip& owner);

	/**
	 * @brief Copy constructor.
	 * @param other Other line iterator.
	 */
	GPIOD_API line_iter(const line_iter& other) = default;

	/**
	 * @brief Move constructor.
	 * @param other Other line iterator.
	 */
	GPIOD_API line_iter(line_iter&& other) = default;

	/**
	 * @brief Assignment operator.
	 * @param other Other line iterator.
	 * @return Reference to this line_iter.
	 */
	GPIOD_API line_iter& operator=(const line_iter& other) = default;

	/**
	 * @brief Move assignment operator.
	 * @param other Other line iterator.
	 * @return Reference to this line_iter.
	 */
	GPIOD_API line_iter& operator=(line_iter&& other) = default;

	/**
	 * @brief Destructor.
	 */
	GPIOD_API ~line_iter(void) = default;

	/**
	 * @brief Advance the iterator by one element.
	 * @return Reference to this iterator.
	 */
	GPIOD_API line_iter& operator++(void);

	/**
	 * @brief Dereference current element.
	 * @return Current GPIO line by reference.
	 */
	GPIOD_API const line& operator*(void) const;

	/**
	 * @brief Member access operator.
	 * @return Current GPIO line by pointer.
	 */
	GPIOD_API const line* operator->(void) const;

	/**
	 * @brief Check if this operator points to the same element.
	 * @param rhs Right-hand side of the equation.
	 * @return True if this iterator points to the same line_iter,
	 *         false otherwise.
	 */
	GPIOD_API bool operator==(const line_iter& rhs) const noexcept;

	/**
	 * @brief Check if this operator doesn't point to the same element.
	 * @param rhs Right-hand side of the equation.
	 * @return True if this iterator doesn't point to the same line_iter,
	 *         false otherwise.
	 */
	GPIOD_API bool operator!=(const line_iter& rhs) const noexcept;

private:

	::std::shared_ptr<::gpiod_line_iter> _m_iter;
	line _m_current;
};

/**
 * @}
 */

} /* namespace gpiod */

#endif /* __LIBGPIOD_GPIOD_CXX_HPP__ */
