// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

/* Reusable GPIO tool helpers - shared library implementation. */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <gpiod.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "gpiotools.h"

#define GT_API		__attribute__((visibility("default")))

static bool isuint(const char *str)
{
	for (; *str && isdigit(*str); str++)
		;

	return *str == '\0';
}

static int parse_uint(const char *option)
{
	unsigned long o;
	char *end;

	o = strtoul(option, &end, 10);
	if (*end == '\0' && o <= INT_MAX)
		return o;

	return -1;
}

static int chip_dir_filter(const struct dirent *entry)
{
	struct stat sb;
	char *path;
	int ret;

	ret = asprintf(&path, "/dev/%s", entry->d_name);
	if (ret < 0)
		return 0;

	ret = 0;
	if ((lstat(path, &sb) == 0) && (!S_ISLNK(sb.st_mode)) &&
	    gpiod_is_gpiochip_device(path))
		ret = 1;

	free(path);

	return ret;
}

GT_API bool gpiotools_chip_path_lookup(const char *id, char **path_ptr)
{
	char *path;
	int ret;

	if (isuint(id)) {
		/* by number */
		ret = asprintf(&path, "/dev/gpiochip%s", id);
	} else if (strchr(id, '/')) {
		/* by path */
		ret = asprintf(&path, "%s", id);
	} else {
		/* by device name */
		ret = asprintf(&path, "/dev/%s", id);
	}

	if (ret < 0)
		return false;

	if (!gpiod_is_gpiochip_device(path)) {
		free(path);
		return false;
	}

	*path_ptr = path;

	return true;
}

GT_API int gpiotools_all_chip_paths(char ***paths_ptr)
{
	int i, j, num_chips, saved_errno;
	struct dirent **entries;
	char **paths;
	int ret;

	num_chips = scandir("/dev/", &entries, chip_dir_filter, versionsort);
	if (num_chips < 0)
		return -1;

	paths = calloc(num_chips, sizeof(*paths));
	if (paths == NULL) {
		saved_errno = errno;
		for (i = 0; i < num_chips; i++)
			free(entries[i]);
		free(entries);
		errno = saved_errno;
		return -1;
	}

	for (i = 0; i < num_chips; i++) {
		ret = asprintf(&paths[i], "/dev/%s", entries[i]->d_name);
		if (ret < 0) {
			saved_errno = errno;
			for (j = 0; j < i; j++)
				free(paths[j]);
			free(paths);
			for (j = 0; j < num_chips; j++)
				free(entries[j]);
			free(entries);
			errno = saved_errno;
			return -1;
		}
	}

	*paths_ptr = paths;

	for (i = 0; i < num_chips; i++)
		free(entries[i]);

	free(entries);

	return num_chips;
}

GT_API int gpiotools_chip_paths(const char *id, char ***paths_ptr)
{
	char **paths;
	char *path;

	if (id == NULL)
		return gpiotools_all_chip_paths(paths_ptr);

	if (!gpiotools_chip_path_lookup(id, &path))
		return 0;

	paths = malloc(sizeof(*paths));
	if (paths == NULL) {
		free(path);
		return -1;
	}

	paths[0] = path;
	*paths_ptr = paths;

	return 1;
}

GT_API struct gpiotools_line_resolver *gpiotools_resolver_init(int num_lines,
							       char **lines,
							       int num_chips,
							       bool strict,
							       bool by_name)
{
	struct gpiotools_line_resolver *resolver;
	struct gpiotools_resolved_line *line;
	size_t resolver_size;
	int i;

	resolver_size = sizeof(*resolver) + num_lines * sizeof(*line);
	resolver = malloc(resolver_size);
	if (resolver == NULL)
		return NULL;

	memset(resolver, 0, resolver_size);

	resolver->chips = calloc(num_chips,
				 sizeof(struct gpiotools_resolved_chip));
	if (resolver->chips == NULL) {
		free(resolver);
		return NULL;
	}

	resolver->num_lines = num_lines;
	resolver->strict = strict;
	for (i = 0; i < num_lines; i++) {
		line = &resolver->lines[i];
		line->id = lines[i];
		line->id_as_offset = by_name ? -1 : parse_uint(lines[i]);
		line->chip_num = -1;
	}

	return resolver;
}

GT_API bool gpiotools_resolve_lines_by_offset(
				struct gpiotools_line_resolver *resolver,
				unsigned int num_lines)
{
	struct gpiotools_resolved_line *line;
	bool used = false;
	int i;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if ((line->id_as_offset != -1) &&
		    (line->id_as_offset < (int)num_lines)) {
			line->chip_num = 0;
			line->offset = line->id_as_offset;
			line->resolved = true;
			used = true;
		}
	}

	return used;
}

GT_API bool gpiotools_resolve_done(struct gpiotools_line_resolver *resolver)
{
	return (!resolver->strict &&
		resolver->num_found >= resolver->num_lines);
}

/*
 * Try to match a single line_info against all requested lines in the resolver.
 * Returns true if the info was claimed by at least one line, false otherwise.
 * On strict-mode uniqueness violation sets errno to EEXIST and returns false.
 */
static bool resolve_line(struct gpiotools_line_resolver *resolver,
			 struct gpiod_line_info *info, int chip_num)
{
	struct gpiotools_resolved_line *line;
	bool resolved = false;
	unsigned int offset;
	const char *name;
	int i;

	offset = gpiod_line_info_get_offset(info);
	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		/* already resolved by offset? */
		if (line->resolved && (line->offset == offset) &&
		    (line->chip_num == chip_num)) {
			line->info = info;
			resolver->num_found++;
			resolved = true;
		}
		if (line->resolved && !resolver->strict)
			continue;

		/* else resolve by name */
		name = gpiod_line_info_get_name(info);
		if (name && (strcmp(line->id, name) == 0)) {
			if (resolver->strict && line->resolved) {
				line->not_unique = true;
				return false;
			}
			line->offset = offset;
			line->info = info;
			line->chip_num = resolver->num_chips;
			line->resolved = true;
			resolver->num_found++;
			resolved = true;
		}
	}

	return resolved;
}

GT_API struct gpiotools_line_resolver *gpiotools_resolve_lines(int num_lines,
							       char **lines,
							       const char *chip_id,
							       bool strict,
							       bool by_name)
{
	struct gpiotools_line_resolver *resolver;
	struct gpiod_chip_info *chip_info;
	struct gpiod_line_info *line_info;
	int num_chips, i, offset;
	struct gpiod_chip *chip;
	bool chip_used;
	char **paths;
	int saved_errno;

	if (chip_id == NULL)
		by_name = true;

	num_chips = gpiotools_chip_paths(chip_id, &paths);
	if (num_chips < 0)
		return NULL;
	if (chip_id && (num_chips == 0)) {
		errno = ENODEV;
		return NULL;
	}

	resolver = gpiotools_resolver_init(num_lines, lines, num_chips,
					   strict, by_name);
	if (resolver == NULL) {
		saved_errno = errno;
		goto err_free_paths;
	}

	for (i = 0; (i < num_chips) && !gpiotools_resolve_done(resolver); i++) {
		chip_used = false;
		chip = gpiod_chip_open(paths[i]);
		if (!chip) {
			if ((errno == EACCES) && (chip_id == NULL)) {
				free(paths[i]);
				paths[i] = NULL;
				continue;
			}

			saved_errno = errno;
			goto err_free_resolver;
		}

		chip_info = gpiod_chip_get_info(chip);
		if (!chip_info) {
			saved_errno = errno;
			gpiod_chip_close(chip);
			goto err_free_resolver;
		}

		num_lines = gpiod_chip_info_get_num_lines(chip_info);

		if (i == 0 && chip_id && !by_name)
			chip_used = gpiotools_resolve_lines_by_offset(resolver,
								      num_lines);

		for (offset = 0;
		     (offset < num_lines) && !gpiotools_resolve_done(resolver);
		     offset++) {
			line_info = gpiod_chip_get_line_info(chip, offset);
			if (!line_info) {
				saved_errno = errno;
				gpiod_chip_info_free(chip_info);
				gpiod_chip_close(chip);
				goto err_free_resolver;
			}

			if (resolve_line(resolver, line_info, i)) {
				chip_used = true;
			} else {
				gpiod_line_info_free(line_info);
			}
		}

		gpiod_chip_close(chip);

		if (chip_used) {
			resolver->chips[resolver->num_chips].info = chip_info;
			resolver->chips[resolver->num_chips].path = paths[i];
			paths[i] = NULL;
			resolver->num_chips++;
		} else {
			gpiod_chip_info_free(chip_info);
			free(paths[i]);
			paths[i] = NULL;
		}
	}

	free(paths);

	return resolver;

err_free_resolver:
	gpiotools_free_line_resolver(resolver);
err_free_paths:
	for (i = 0; i < num_chips; i++)
		free(paths[i]);
	free(paths);
	errno = saved_errno;
	return NULL;
}

GT_API int
gpiotools_validate_resolution(struct gpiotools_line_resolver *resolver)
{
	struct gpiotools_resolved_line *line, *prev;
	bool valid = true;
	int i, j;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (line->resolved) {
			for (j = 0; j < i; j++) {
				prev = &resolver->lines[j];
				if (prev->resolved &&
				    (prev->chip_num == line->chip_num) &&
				    (prev->offset == line->offset)) {
					valid = false;
					break;
				}
			}
			continue;
		}
		valid = false;
	}

	if (!valid) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

GT_API void gpiotools_free_line_resolver(struct gpiotools_line_resolver *resolver)
{
	int i;

	if (!resolver)
		return;

	for (i = 0; i < resolver->num_lines; i++)
		gpiod_line_info_free(resolver->lines[i].info);

	for (i = 0; i < resolver->num_chips; i++) {
		gpiod_chip_info_free(resolver->chips[i].info);
		free(resolver->chips[i].path);
	}

	free(resolver->chips);
	free(resolver);
}

GT_API int gpiotools_get_line_offsets_and_values(
				struct gpiotools_line_resolver *resolver,
				int chip_num, unsigned int *offsets,
				enum gpiod_line_value *values)
{
	struct gpiotools_resolved_line *line;
	int i, num_lines = 0;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (line->chip_num == chip_num) {
			offsets[num_lines] = line->offset;
			if (values)
				values[num_lines] = line->value;

			num_lines++;
		}
	}

	return num_lines;
}

GT_API const char *gpiotools_get_chip_name(
				struct gpiotools_line_resolver *resolver,
				int chip_num)
{
	return gpiod_chip_info_get_name(resolver->chips[chip_num].info);
}

GT_API const char *gpiotools_get_line_name(
				struct gpiotools_line_resolver *resolver,
				int chip_num, unsigned int offset)
{
	struct gpiotools_resolved_line *line;
	int i;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (line->info && (line->offset == offset) &&
		    (line->chip_num == chip_num))
			return gpiod_line_info_get_name(resolver->lines[i].info);
	}

	return NULL;
}

GT_API void gpiotools_set_line_values(struct gpiotools_line_resolver *resolver,
				      int chip_num,
				      enum gpiod_line_value *values)
{
	int i, j;

	for (i = 0, j = 0; i < resolver->num_lines; i++) {
		if (resolver->lines[i].chip_num == chip_num) {
			resolver->lines[i].value = values[j];
			j++;
		}
	}
}
