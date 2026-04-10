/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com> */
/* SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com> */

/**
 * @file gpiotools.h
 */

#ifndef __GPIOTOOLS_H__
#define __GPIOTOOLS_H__

#include <gpiod.h>
#include <stdbool.h>

/**
 * @defgroup gpiotools GPIO tools helpers
 * @{
 *
 * Reusable chip and line resolution helpers extracted from the standard GPIO
 * command-line tools.  These functions allow users to build their own programs
 * on top of libgpiod using the same higher-level abstractions as the tools.
 *
 * Error handling: functions return negative errno values on failure unless
 * otherwise noted.
 */

/**
 * @brief Descriptor for a single resolved GPIO line.
 */
struct gpiotools_resolved_line {
	/** Identifier string from the command line. */
	const char *id;
	/**
	 * id parsed as an integer offset, or -1 if the line must be resolved
	 * by name.
	 */
	int id_as_offset;
	/** True once the line has been located on a chip. */
	bool resolved;
	/** True if strict mode found the name on more than one chip. */
	bool not_unique;
	/** Line info object; only valid once resolved. */
	struct gpiod_line_info *info;
	/** Index of the owning chip in the resolver's chips array. */
	int chip_num;
	/** Offset of the line on its chip. */
	unsigned int offset;
	/** Line value used by gpioget/gpioset. */
	int value;
	/** Reserved for future extensions. */
	char _padding[32];
};

/**
 * @brief Descriptor for a single GPIO chip referenced by a resolver.
 */
struct gpiotools_resolved_chip {
	/** Chip info object. */
	struct gpiod_chip_info *info;
	/** Path to the chip device file. */
	char *path;
	/** Reserved for future extensions. */
	char _padding[32];
};

/**
 * @brief Resolver mapping requested line names or offsets to GPIO lines.
 */
struct gpiotools_line_resolver {
	/** Number of chips the lines span; also the number of entries in chips. */
	int num_chips;
	/** Number of entries in lines. */
	int num_lines;
	/** Number of lines that have been found so far. */
	int num_found;
	/** If true, perform an exhaustive search to verify line name uniqueness. */
	bool strict;
	/** Array of chip descriptors for the chips that own the requested lines. */
	struct gpiotools_resolved_chip *chips;
	/** Reserved for future extensions. */
	char _padding[32];
	/** Flexible array of line descriptors for the requested lines. */
	struct gpiotools_resolved_line lines[];
};

/**
 * @brief Look up the path to a GPIO chip device.
 * @param id Chip identifier: a number ("0"), a name ("gpiochip0"), or a path
 *           ("/dev/gpiochip0").
 * @param path_ptr On success, set to a newly allocated string containing the
 *                 chip path.  The caller must free() it.
 * @return True if the chip was found, false otherwise (including on allocation
 *         failure).
 */
bool gpiotools_chip_path_lookup(const char *id, char **path_ptr);

/**
 * @brief Get the paths of GPIO chip devices matching an identifier.
 * @param id Chip identifier, or NULL to return all chips.
 * @param paths_ptr On success, set to a newly allocated array of path strings.
 *                  The caller must free() each element and the array itself.
 * @return Number of chips found (0 if none), or negative errno on error.
 */
int gpiotools_chip_paths(const char *id, char ***paths_ptr);

/**
 * @brief Get the paths of all GPIO chip devices on the system.
 * @param paths_ptr On success, set to a newly allocated array of path strings.
 *                  The caller must free() each element and the array itself.
 * @return Number of chips found (0 if none), or negative errno on error.
 */
int gpiotools_all_chip_paths(char ***paths_ptr);

/**
 * @brief Allocate and initialise a line resolver.
 * @param num_lines Number of lines to resolve.
 * @param lines Array of line identifiers (names or offset strings).
 * @param num_chips Number of chips to allocate space for.
 * @param strict If true, perform an exhaustive search to verify that line
 *               names are unique.
 * @param by_name If true, treat all identifiers as names; if false, try to
 *                parse them as numeric offsets first.
 * @return Pointer to the new resolver, or NULL on allocation failure.
 */
struct gpiotools_line_resolver *gpiotools_resolver_init(int num_lines,
							char **lines,
							int num_chips,
							bool strict,
							bool by_name);

/**
 * @brief Resolve lines by numeric offset.
 * @param resolver Resolver to update.
 * @param num_lines Number of lines on the chip.
 *
 * Marks lines whose identifier was successfully parsed as a numeric offset as
 * resolved.  Only applies to the first chip (chip_num == 0).
 *
 * @return True if any line was resolved, false otherwise.
 */
bool gpiotools_resolve_lines_by_offset(struct gpiotools_line_resolver *resolver,
				       unsigned int num_lines);

/**
 * @brief Check whether line resolution is complete.
 * @param resolver Resolver to check.
 *
 * In non-strict mode, resolution is considered done when all requested lines
 * have been found.  In strict mode this always returns false so that the
 * caller performs an exhaustive search.
 *
 * @return True if resolution is complete, false otherwise.
 */
bool gpiotools_resolve_done(struct gpiotools_line_resolver *resolver);

/**
 * @brief Resolve line names or offsets to GPIO lines on the system.
 * @param num_lines Number of lines to resolve.
 * @param lines Array of line identifiers (names or offset strings).
 * @param chip_id Chip identifier to restrict the search, or NULL to search
 *                all chips.
 * @param strict If true, verify that line names are unique across all chips.
 * @param by_name If true, treat all identifiers as names.
 * @return Pointer to the populated resolver, or NULL on error with errno set.
 */
struct gpiotools_line_resolver *gpiotools_resolve_lines(int num_lines,
							char **lines,
							const char *chip_id,
							bool strict,
							bool by_name);

/**
 * @brief Verify that all requested lines were found and are unique.
 * @param resolver Resolver to validate.
 * @return 0 if all lines were found and are unique, or negative errno if any
 *         line was not found or if two identifiers refer to the same line.
 */
int gpiotools_validate_resolution(struct gpiotools_line_resolver *resolver);

/**
 * @brief Free a line resolver and all memory it owns.
 * @param resolver Resolver to free.  May be NULL.
 */
void gpiotools_free_line_resolver(struct gpiotools_line_resolver *resolver);

/**
 * @brief Extract offsets and optionally values for lines on a specific chip.
 * @param resolver Resolver containing the resolved lines.
 * @param chip_num Index of the chip to query.
 * @param offsets Pre-allocated array to receive the line offsets.
 * @param values Pre-allocated array to receive the line values, or NULL.
 * @return Number of lines belonging to the specified chip.
 */
int gpiotools_get_line_offsets_and_values(struct gpiotools_line_resolver *resolver,
					  int chip_num, unsigned int *offsets,
					  enum gpiod_line_value *values);

/**
 * @brief Get the name of a chip referenced by the resolver.
 * @param resolver Resolver containing the chip.
 * @param chip_num Index of the chip.
 * @return Pointer to the chip name string owned by the resolver.
 */
const char *gpiotools_get_chip_name(struct gpiotools_line_resolver *resolver,
				    int chip_num);

/**
 * @brief Get the name of a resolved line.
 * @param resolver Resolver containing the line.
 * @param chip_num Index of the chip the line belongs to.
 * @param offset Offset of the line on the chip.
 * @return Pointer to the line name string owned by the resolver, or NULL if
 *         the line has no name.
 */
const char *gpiotools_get_line_name(struct gpiotools_line_resolver *resolver,
				    int chip_num, unsigned int offset);

/**
 * @brief Update line values stored in the resolver for a specific chip.
 * @param resolver Resolver to update.
 * @param chip_num Index of the chip whose lines should be updated.
 * @param values Array of values to store, one per line on the chip in the
 *               order they appear in the resolver.
 */
void gpiotools_set_line_values(struct gpiotools_line_resolver *resolver,
			       int chip_num, enum gpiod_line_value *values);

/**
 * @}
 */

#endif /* __GPIOTOOLS_H__ */
