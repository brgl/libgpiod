/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com> */
/* SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com> */

#ifndef __GPIOD_TOOLS_COMMON_H__
#define __GPIOD_TOOLS_COMMON_H__

#include <gpiod.h>

/*
 * Various helpers for the GPIO tools.
 *
 * NOTE: This is not a stable interface - it's only to avoid duplicating
 * common code.
 */

#define NORETURN		__attribute__((noreturn))
#define PRINTF(fmt, arg)	__attribute__((format(printf, fmt, arg)))

#define GETOPT_NULL_LONGOPT	NULL, 0, NULL, 0

struct resolved_line {
	/* from the command line */
	const char *id;

	/*
	 * id parsed as int, if that is an option, or -1 if line must be
	 * resolved by name
	 */
	int id_as_offset;

	/* line has been located on a chip */
	bool resolved;

	/* remaining fields only valid once resolved... */

	/* info for the line */
	struct gpiod_line_info *info;

	/* num of relevant chip in line_resolver */
	int chip_num;

	/* offset of line on chip */
	unsigned int offset;

	/* line value for gpioget/set */
	int value;
};

struct resolved_chip {
	/* info of the relevant chips */
	struct gpiod_chip_info *info;

	/* path to the chip */
	char *path;
};

/* a resolver from requested line names/offsets to lines on the system */
struct line_resolver {
	/*
	 * number of chips the lines span, and number of entries in chips
	 */
	int num_chips;

	/* number of lines in lines */
	int num_lines;

	/* number of lines found */
	int num_found;

	/* perform exhaustive search to check line names are unique */
	bool strict;

	/* details of the relevant chips */
	struct resolved_chip *chips;

	/* descriptors for the requested lines */
	struct resolved_line lines[];
};

const char *get_progname(void);
void print_error(const char *fmt, ...) PRINTF(1, 2);
void print_perror(const char *fmt, ...) PRINTF(1, 2);
void die(const char *fmt, ...) NORETURN PRINTF(1, 2);
void die_perror(const char *fmt, ...) NORETURN PRINTF(1, 2);
void print_version(void);
int parse_bias_or_die(const char *option);
int parse_period(const char *option);
unsigned int parse_period_or_die(const char *option);
int parse_uint(const char *option);
unsigned int parse_uint_or_die(const char *option);
void print_bias_help(void);
void print_chip_help(void);
void print_period_help(void);
void print_event_time(uint64_t evtime, int format);
void print_line_attributes(struct gpiod_line_info *info, bool unquoted_strings);
void print_line_id(struct line_resolver *resolver, int chip_num,
		   unsigned int offset, const char *chip_id, bool unquoted);
bool chip_path_lookup(const char *id, char **path_ptr);
int chip_paths(const char *id, char ***paths_ptr);
int all_chip_paths(char ***paths_ptr);
struct line_resolver *resolve_lines(int num_lines, char **lines,
				    const char *chip_id, bool strict,
				    bool by_name);
struct line_resolver *resolver_init(int num_lines, char **lines, int num_chips,
				    bool strict, bool by_name);
bool resolve_lines_by_offset(struct line_resolver *resolver,
			     unsigned int num_lines);
bool resolve_done(struct line_resolver *resolver);
void validate_resolution(struct line_resolver *resolver, const char *chip_id);
void free_line_resolver(struct line_resolver *resolver);
int get_line_offsets_and_values(struct line_resolver *resolver, int chip_num,
				unsigned int *offsets,
				enum gpiod_line_value *values);
const char *get_chip_name(struct line_resolver *resolver, int chip_num);
const char *get_line_name(struct line_resolver *resolver, int chip_num,
			  unsigned int offset);
void set_line_values(struct line_resolver *resolver, int chip_num,
		     enum gpiod_line_value *values);

#endif /* __GPIOD_TOOLS_COMMON_H__ */
