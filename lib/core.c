// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Low-level, core library code. */

#include <errno.h>
#include <fcntl.h>
#include <gpiod.h>
#include <limits.h>
#include <linux/gpio.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

enum {
	LINE_FREE = 0,
	LINE_REQUESTED_VALUES,
	LINE_REQUESTED_EVENTS,
};

struct line_fd_handle {
	int fd;
	int refcount;
};

struct gpiod_line {
	unsigned int offset;

	/* The direction of the GPIO line. */
	int direction;

	/* The active-state configuration. */
	int active_state;

	/* The logical value last written to the line. */
	int output_value;

	/* The GPIOLINE_FLAGs returned by GPIO_GET_LINEINFO_IOCTL. */
	__u32 info_flags;

	/* The GPIOD_LINE_REQUEST_FLAGs provided to request the line. */
	__u32 req_flags;

	/*
	 * Indicator of LINE_FREE, LINE_REQUESTED_VALUES or
	 * LINE_REQUESTED_EVENTS.
	 */
	int state;

	struct gpiod_chip *chip;
	struct line_fd_handle *fd_handle;

	char name[32];
	char consumer[32];
};

struct gpiod_chip {
	struct gpiod_line **lines;
	unsigned int num_lines;

	int fd;

	char name[32];
	char label[32];
};

/*
 * The structure is defined in a way that allows internal users to allocate
 * bulk objects that hold a single line on the stack - that way we can reuse
 * a lot of code between functions that take single lines and those that take
 * bulks as arguments while not unnecessarily allocating memory dynamically.
 */
struct gpiod_line_bulk {
	unsigned int num_lines;
	unsigned int max_lines;
	struct gpiod_line *lines[1];
};

#define BULK_SINGLE_LINE_INIT(line) \
		{ 1, 1, { (line) } }

struct gpiod_line_bulk *gpiod_line_bulk_new(unsigned int max_lines)
{
	struct gpiod_line_bulk *bulk;
	size_t size;

	if (max_lines < 1 || max_lines > GPIOD_LINE_BULK_MAX_LINES) {
		errno = EINVAL;
		return NULL;
	}

	size = sizeof(struct gpiod_line_bulk) +
	       (max_lines - 1) * sizeof(struct gpiod_line *);

	bulk = malloc(size);
	if (!bulk)
		return NULL;

	bulk->max_lines = max_lines;
	gpiod_line_bulk_reset(bulk);

	return bulk;
}

void gpiod_line_bulk_reset(struct gpiod_line_bulk *bulk)
{
	bulk->num_lines = 0;
	memset(bulk->lines, 0, bulk->max_lines * sizeof(struct line *));
}

void gpiod_line_bulk_free(struct gpiod_line_bulk *bulk)
{
	free(bulk);
}

int gpiod_line_bulk_add_line(struct gpiod_line_bulk *bulk,
			     struct gpiod_line *line)
{
	if (bulk->num_lines == bulk->max_lines) {
		errno = EINVAL;
		return -1;
	}

	if (bulk->num_lines != 0) {
		if (bulk->lines[0]->chip != gpiod_line_get_chip(line)) {
			errno = EINVAL;
			return -1;
		}
	}

	bulk->lines[bulk->num_lines++] = line;

	return 0;
}

struct gpiod_line *
gpiod_line_bulk_get_line(struct gpiod_line_bulk *bulk, unsigned int index)
{
	if (index >= bulk->num_lines) {
		errno = EINVAL;
		return NULL;
	}

	return bulk->lines[index];
}

unsigned int gpiod_line_bulk_num_lines(struct gpiod_line_bulk *bulk)
{
	return bulk->num_lines;
}

void gpiod_line_bulk_foreach_line(struct gpiod_line_bulk *bulk,
				  gpiod_line_bulk_foreach_cb func, void *data)
{
	unsigned int index;
	int ret;

	for (index = 0; index < bulk->num_lines; index++) {
		ret = func(bulk->lines[index], data);
		if (ret == GPIOD_LINE_BULK_CB_STOP)
			return;
	}
}

#define line_bulk_foreach_line(bulk, line, index)			\
	for ((index) = 0, (line) = (bulk)->lines[0];			\
	     (index) < (bulk)->num_lines;				\
	     (index)++, (line) = (bulk)->lines[(index)])

bool gpiod_is_gpiochip_device(const char *path)
{
	char *name, *realname, *sysfsp, sysfsdev[16], devstr[16];
	struct stat statbuf;
	bool ret = false;
	int rv, fd;
	ssize_t rd;

	rv = lstat(path, &statbuf);
	if (rv)
		goto out;

	/*
	 * Is it a symbolic link? We have to resolve it before checking
	 * the rest.
	 */
	realname = S_ISLNK(statbuf.st_mode) ? realpath(path, NULL)
					    : strdup(path);
	if (realname == NULL)
		goto out;

	rv = stat(realname, &statbuf);
	if (rv)
		goto out_free_realname;

	/* Is it a character device? */
	if (!S_ISCHR(statbuf.st_mode)) {
		/*
		 * Passing a file descriptor not associated with a character
		 * device to ioctl() makes it set errno to ENOTTY. Let's do
		 * the same in order to stay compatible with the versions of
		 * libgpiod from before the introduction of this routine.
		 */
		errno = ENOTTY;
		goto out_free_realname;
	}

	/* Get the basename. */
	name = basename(realname);

	/* Do we have a corresponding sysfs attribute? */
	rv = asprintf(&sysfsp, "/sys/bus/gpio/devices/%s/dev", name);
	if (rv < 0)
		goto out_free_realname;

	if (access(sysfsp, R_OK) != 0) {
		/*
		 * This is a character device but not the one we're after.
		 * Before the introduction of this function, we'd fail with
		 * ENOTTY on the first GPIO ioctl() call for this file
		 * descriptor. Let's stay compatible here and keep returning
		 * the same error code.
		 */
		errno = ENOTTY;
		goto out_free_sysfsp;
	}

	/*
	 * Make sure the major and minor numbers of the character device
	 * correspond to the ones in the dev attribute in sysfs.
	 */
	snprintf(devstr, sizeof(devstr), "%u:%u",
		 major(statbuf.st_rdev), minor(statbuf.st_rdev));

	fd = open(sysfsp, O_RDONLY);
	if (fd < 0)
		goto out_free_sysfsp;

	memset(sysfsdev, 0, sizeof(sysfsdev));
	rd = read(fd, sysfsdev, sizeof(sysfsdev) - 1);
	close(fd);
	if (rd < 0)
		goto out_free_sysfsp;

	rd--; /* Ignore trailing newline. */
	if ((size_t)rd != strlen(devstr) ||
	    strncmp(sysfsdev, devstr, rd) != 0) {
		errno = ENODEV;
		goto out_free_sysfsp;
	}

	ret = true;

out_free_sysfsp:
	free(sysfsp);
out_free_realname:
	free(realname);
out:
	return ret;
}

struct gpiod_chip *gpiod_chip_open(const char *path)
{
	struct gpiochip_info info;
	struct gpiod_chip *chip;
	int rv, fd;

	fd = open(path, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return NULL;

	/*
	 * We were able to open the file but is it really a gpiochip character
	 * device?
	 */
	if (!gpiod_is_gpiochip_device(path))
		goto err_close_fd;

	chip = malloc(sizeof(*chip));
	if (!chip)
		goto err_close_fd;

	memset(chip, 0, sizeof(*chip));
	memset(&info, 0, sizeof(info));

	rv = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info);
	if (rv < 0)
		goto err_free_chip;

	chip->fd = fd;
	chip->num_lines = info.lines;

	/*
	 * GPIO device must have a name - don't bother checking this field. In
	 * the worst case (would have to be a weird kernel bug) it'll be empty.
	 */
	strncpy(chip->name, info.name, sizeof(chip->name));

	/*
	 * The kernel sets the label of a GPIO device to "unknown" if it
	 * hasn't been defined in DT, board file etc. On the off-chance that
	 * we got an empty string, do the same.
	 */
	if (info.label[0] == '\0')
		strncpy(chip->label, "unknown", sizeof(chip->label));
	else
		strncpy(chip->label, info.label, sizeof(chip->label));

	return chip;

err_free_chip:
	free(chip);
err_close_fd:
	close(fd);

	return NULL;
}

void gpiod_chip_close(struct gpiod_chip *chip)
{
	struct gpiod_line *line;
	unsigned int i;

	if (chip->lines) {
		for (i = 0; i < chip->num_lines; i++) {
			line = chip->lines[i];
			if (line) {
				gpiod_line_release(line);
				free(line);
			}
		}

		free(chip->lines);
	}

	close(chip->fd);
	free(chip);
}

const char *gpiod_chip_name(struct gpiod_chip *chip)
{
	return chip->name;
}

const char *gpiod_chip_label(struct gpiod_chip *chip)
{
	return chip->label;
}

unsigned int gpiod_chip_num_lines(struct gpiod_chip *chip)
{
	return chip->num_lines;
}

struct gpiod_line *
gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
{
	struct gpiod_line *line;
	int rv;

	if (offset >= chip->num_lines) {
		errno = EINVAL;
		return NULL;
	}

	if (!chip->lines) {
		chip->lines = calloc(chip->num_lines,
				     sizeof(struct gpiod_line *));
		if (!chip->lines)
			return NULL;
	}

	if (!chip->lines[offset]) {
		line = malloc(sizeof(*line));
		if (!line)
			return NULL;

		memset(line, 0, sizeof(*line));

		line->offset = offset;
		line->chip = chip;

		chip->lines[offset] = line;
	} else {
		line = chip->lines[offset];
	}

	rv = gpiod_line_update(line);
	if (rv < 0)
		return NULL;

	return line;
}

static struct line_fd_handle *line_make_fd_handle(int fd)
{
	struct line_fd_handle *handle;

	handle = malloc(sizeof(*handle));
	if (!handle)
		return NULL;

	handle->fd = fd;
	handle->refcount = 0;

	return handle;
}

static void line_fd_incref(struct gpiod_line *line)
{
	line->fd_handle->refcount++;
}

static void line_fd_decref(struct gpiod_line *line)
{
	struct line_fd_handle *handle = line->fd_handle;

	handle->refcount--;

	if (handle->refcount == 0) {
		close(handle->fd);
		free(handle);
		line->fd_handle = NULL;
	}
}

static void line_set_fd(struct gpiod_line *line, struct line_fd_handle *handle)
{
	line->fd_handle = handle;
	line_fd_incref(line);
}

static int line_get_fd(struct gpiod_line *line)
{
	return line->fd_handle->fd;
}

struct gpiod_chip *gpiod_line_get_chip(struct gpiod_line *line)
{
	return line->chip;
}

unsigned int gpiod_line_offset(struct gpiod_line *line)
{
	return line->offset;
}

const char *gpiod_line_name(struct gpiod_line *line)
{
	return line->name[0] == '\0' ? NULL : line->name;
}

const char *gpiod_line_consumer(struct gpiod_line *line)
{
	return line->consumer[0] == '\0' ? NULL : line->consumer;
}

int gpiod_line_direction(struct gpiod_line *line)
{
	return line->direction;
}

int gpiod_line_active_state(struct gpiod_line *line)
{
	return line->active_state;
}

int gpiod_line_bias(struct gpiod_line *line)
{
	if (line->info_flags & GPIOLINE_FLAG_BIAS_DISABLE)
		return GPIOD_LINE_BIAS_DISABLE;
	if (line->info_flags & GPIOLINE_FLAG_BIAS_PULL_UP)
		return GPIOD_LINE_BIAS_PULL_UP;
	if (line->info_flags & GPIOLINE_FLAG_BIAS_PULL_DOWN)
		return GPIOD_LINE_BIAS_PULL_DOWN;

	return GPIOD_LINE_BIAS_AS_IS;
}

bool gpiod_line_is_used(struct gpiod_line *line)
{
	return line->info_flags & GPIOLINE_FLAG_KERNEL;
}

bool gpiod_line_is_open_drain(struct gpiod_line *line)
{
	return line->info_flags & GPIOLINE_FLAG_OPEN_DRAIN;
}

bool gpiod_line_is_open_source(struct gpiod_line *line)
{
	return line->info_flags & GPIOLINE_FLAG_OPEN_SOURCE;
}

static int line_info_v2_to_info_flags(struct gpio_v2_line_info *info)
{
	int iflags = 0;

	if (info->flags & GPIO_V2_LINE_FLAG_USED)
		iflags |= GPIOLINE_FLAG_KERNEL;

	if (info->flags & GPIO_V2_LINE_FLAG_OPEN_DRAIN)
		iflags |= GPIOLINE_FLAG_OPEN_DRAIN;
	if (info->flags & GPIO_V2_LINE_FLAG_OPEN_SOURCE)
		iflags |= GPIOLINE_FLAG_OPEN_SOURCE;

	if (info->flags & GPIO_V2_LINE_FLAG_BIAS_DISABLED)
		iflags |= GPIOLINE_FLAG_BIAS_DISABLE;
	if (info->flags & GPIO_V2_LINE_FLAG_BIAS_PULL_UP)
		iflags |= GPIOLINE_FLAG_BIAS_PULL_UP;
	if (info->flags & GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN)
		iflags |= GPIOLINE_FLAG_BIAS_PULL_DOWN;

	return iflags;
}

int gpiod_line_update(struct gpiod_line *line)
{
	struct gpio_v2_line_info info;
	int rv;

	memset(&info, 0, sizeof(info));
	info.offset = line->offset;

	rv = ioctl(line->chip->fd, GPIO_V2_GET_LINEINFO_IOCTL, &info);
	if (rv < 0)
		return -1;

	line->direction = info.flags & GPIO_V2_LINE_FLAG_OUTPUT
						? GPIOD_LINE_DIRECTION_OUTPUT
						: GPIOD_LINE_DIRECTION_INPUT;

	line->active_state = info.flags & GPIO_V2_LINE_FLAG_ACTIVE_LOW
						? GPIOD_LINE_ACTIVE_STATE_LOW
						: GPIOD_LINE_ACTIVE_STATE_HIGH;

	line->info_flags = line_info_v2_to_info_flags(&info);

	strncpy(line->name, info.name, sizeof(line->name));
	strncpy(line->consumer, info.consumer, sizeof(line->consumer));

	return 0;
}

static bool line_bulk_all_requested(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line;
	unsigned int idx;

	line_bulk_foreach_line(bulk, line, idx) {
		if (!gpiod_line_is_requested(line)) {
			errno = EPERM;
			return false;
		}
	}

	return true;
}

static bool line_bulk_all_requested_values(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line;
	unsigned int idx;

	line_bulk_foreach_line(bulk, line, idx) {
		if (line->state != LINE_REQUESTED_VALUES) {
			errno = EPERM;
			return false;
		}
	}

	return true;
}

static bool line_bulk_all_free(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line;
	unsigned int idx;

	line_bulk_foreach_line(bulk, line, idx) {
		if (!gpiod_line_is_free(line)) {
			errno = EBUSY;
			return false;
		}
	}

	return true;
}

static bool line_request_direction_is_valid(int direction)
{
	if ((direction == GPIOD_LINE_REQUEST_DIRECTION_AS_IS) ||
	    (direction == GPIOD_LINE_REQUEST_DIRECTION_INPUT) ||
	    (direction == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT))
		return true;

	errno = EINVAL;
	return false;
}

static void line_request_type_to_gpio_v2_line_config(int reqtype,
		struct gpio_v2_line_config *config)
{
	if (reqtype == GPIOD_LINE_REQUEST_DIRECTION_AS_IS)
		return;

	if (reqtype == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT) {
		config->flags |= GPIO_V2_LINE_FLAG_OUTPUT;
		return;
	}
	config->flags |= GPIO_V2_LINE_FLAG_INPUT;

	if (reqtype == GPIOD_LINE_REQUEST_EVENT_RISING_EDGE)
		config->flags |= GPIO_V2_LINE_FLAG_EDGE_RISING;
	else if (reqtype == GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE)
		config->flags |= GPIO_V2_LINE_FLAG_EDGE_FALLING;
	else if (reqtype == GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES)
		config->flags |= (GPIO_V2_LINE_FLAG_EDGE_RISING |
				  GPIO_V2_LINE_FLAG_EDGE_FALLING);
}

static void line_request_flag_to_gpio_v2_line_config(int flags,
		struct gpio_v2_line_config *config)
{
	if (flags & GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW)
		config->flags |= GPIO_V2_LINE_FLAG_ACTIVE_LOW;

	if (flags & GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN)
		config->flags |= GPIO_V2_LINE_FLAG_OPEN_DRAIN;
	else if (flags & GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE)
		config->flags |= GPIO_V2_LINE_FLAG_OPEN_SOURCE;

	if (flags & GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE)
		config->flags |= GPIO_V2_LINE_FLAG_BIAS_DISABLED;
	else if (flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN)
		config->flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN;
	else if (flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP)
		config->flags |= GPIO_V2_LINE_FLAG_BIAS_PULL_UP;
}

static void line_request_config_to_gpio_v2_line_config(
		const struct gpiod_line_request_config *reqcfg,
		struct gpio_v2_line_config *lc)
{
	line_request_type_to_gpio_v2_line_config(reqcfg->request_type, lc);
	line_request_flag_to_gpio_v2_line_config(reqcfg->flags, lc);
}

static bool line_request_config_validate(
		const struct gpiod_line_request_config *config)
{
	int bias_flags = 0;

	if ((config->request_type != GPIOD_LINE_REQUEST_DIRECTION_OUTPUT) &&
	    (config->flags & (GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN |
			      GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE)))
		return false;


	if ((config->flags & GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN) &&
	    (config->flags & GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE)) {
		return false;
	}

	if (config->flags & GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE)
		bias_flags++;
	if (config->flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP)
		bias_flags++;
	if (config->flags & GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN)
		bias_flags++;
	if (bias_flags > 1)
		return false;

	return true;
}

static void lines_bitmap_set_bit(__u64 *bits, int nr)
{
	*bits |= _BITULL(nr);
}

static void lines_bitmap_clear_bit(__u64 *bits, int nr)
{
	*bits &= ~_BITULL(nr);
}

static int lines_bitmap_test_bit(__u64 bits, int nr)
{
	return !!(bits & _BITULL(nr));
}

static void lines_bitmap_assign_bit(__u64 *bits, int nr, bool value)
{
	if (value)
		lines_bitmap_set_bit(bits, nr);
	else
		lines_bitmap_clear_bit(bits, nr);
}

static int line_request_values(struct gpiod_line_bulk *bulk,
			       const struct gpiod_line_request_config *config,
			       const int *vals)
{
	struct gpiod_line *line;
	struct line_fd_handle *line_fd;
	struct gpio_v2_line_request req;
	unsigned int i;
	int rv, fd;

	if (!line_request_config_validate(config)) {
		errno = EINVAL;
		return -1;
	}

	memset(&req, 0, sizeof(req));

	req.num_lines = gpiod_line_bulk_num_lines(bulk);
	line_request_config_to_gpio_v2_line_config(config, &req.config);

	line_bulk_foreach_line(bulk, line, i)
		req.offsets[i] = gpiod_line_offset(line);

	if (config->request_type == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT &&
	    vals) {
		req.config.num_attrs = 1;
		req.config.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
		line_bulk_foreach_line(bulk, line, i) {
			lines_bitmap_assign_bit(
				&req.config.attrs[0].mask, i, 1);
			lines_bitmap_assign_bit(
				&req.config.attrs[0].attr.values,
				i, vals[i]);
		}
	}

	if (config->consumer)
		strncpy(req.consumer, config->consumer,
			sizeof(req.consumer) - 1);

	line = gpiod_line_bulk_get_line(bulk, 0);
	fd = line->chip->fd;

	rv = ioctl(fd, GPIO_V2_GET_LINE_IOCTL, &req);
	if (rv < 0)
		return -1;

	line_fd = line_make_fd_handle(req.fd);
	if (!line_fd)
		return -1;

	line_bulk_foreach_line(bulk, line, i) {
		line->state = LINE_REQUESTED_VALUES;
		line->req_flags = config->flags;
		if (config->request_type == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT)
			line->output_value = lines_bitmap_test_bit(
				req.config.attrs[0].attr.values, i);
		line_set_fd(line, line_fd);

		rv = gpiod_line_update(line);
		if (rv) {
			gpiod_line_release_bulk(bulk);
			return rv;
		}
	}

	return 0;
}

static int line_request_event_single(struct gpiod_line *line,
			const struct gpiod_line_request_config *config)
{
	struct line_fd_handle *line_fd;
	struct gpio_v2_line_request req;
	int rv;

	memset(&req, 0, sizeof(req));

	if (config->consumer)
		strncpy(req.consumer, config->consumer,
			sizeof(req.consumer) - 1);

	req.offsets[0] = gpiod_line_offset(line);
	req.num_lines = 1;
	line_request_config_to_gpio_v2_line_config(config, &req.config);

	rv = ioctl(line->chip->fd, GPIO_V2_GET_LINE_IOCTL, &req);
	if (rv < 0)
		return -1;

	line_fd = line_make_fd_handle(req.fd);
	if (!line_fd)
		return -1;

	line->state = LINE_REQUESTED_EVENTS;
	line->req_flags = config->flags;
	line_set_fd(line, line_fd);

	rv = gpiod_line_update(line);
	if (rv) {
		gpiod_line_release(line);
		return rv;
	}

	return 0;
}

static int line_request_events(struct gpiod_line_bulk *bulk,
			       const struct gpiod_line_request_config *config)
{
	struct gpiod_line *line;
	unsigned int off;
	int rv, rev;

	line_bulk_foreach_line(bulk, line, off) {
		rv = line_request_event_single(line, config);
		if (rv) {
			for (rev = off - 1; rev >= 0; rev--) {
				line = gpiod_line_bulk_get_line(bulk, rev);
				gpiod_line_release(line);
			}

			return -1;
		}
	}

	return 0;
}

int gpiod_line_request(struct gpiod_line *line,
		       const struct gpiod_line_request_config *config,
		       int default_val)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);

	return gpiod_line_request_bulk(&bulk, config, &default_val);
}

static bool line_request_is_direction(int request)
{
	return request == GPIOD_LINE_REQUEST_DIRECTION_AS_IS ||
	       request == GPIOD_LINE_REQUEST_DIRECTION_INPUT ||
	       request == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
}

static bool line_request_is_events(int request)
{
	return request == GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE ||
	       request == GPIOD_LINE_REQUEST_EVENT_RISING_EDGE ||
	       request == GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
}

int gpiod_line_request_bulk(struct gpiod_line_bulk *bulk,
			    const struct gpiod_line_request_config *config,
			    const int *vals)
{
	if (!line_bulk_all_free(bulk))
		return -1;

	if (line_request_is_direction(config->request_type))
		return line_request_values(bulk, config, vals);
	else if (line_request_is_events(config->request_type))
		return line_request_events(bulk, config);

	errno = EINVAL;
	return -1;
}

void gpiod_line_release(struct gpiod_line *line)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);

	gpiod_line_release_bulk(&bulk);
}

void gpiod_line_release_bulk(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line;
	unsigned int idx;

	line_bulk_foreach_line(bulk, line, idx) {
		if (line->state != LINE_FREE) {
			line_fd_decref(line);
			line->state = LINE_FREE;
		}
	}
}

bool gpiod_line_is_requested(struct gpiod_line *line)
{
	return (line->state == LINE_REQUESTED_VALUES ||
		line->state == LINE_REQUESTED_EVENTS);
}

bool gpiod_line_is_free(struct gpiod_line *line)
{
	return line->state == LINE_FREE;
}

int gpiod_line_get_value(struct gpiod_line *line)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);
	int rv, value;

	rv = gpiod_line_get_value_bulk(&bulk, &value);
	if (rv < 0)
		return -1;

	return value;
}

int gpiod_line_get_value_bulk(struct gpiod_line_bulk *bulk, int *values)
{
	struct gpio_v2_line_values lv;
	struct gpiod_line *line;
	unsigned int i;
	int rv, fd;

	if (!line_bulk_all_requested(bulk))
		return -1;

	line = gpiod_line_bulk_get_line(bulk, 0);

	memset(&lv, 0, sizeof(lv));

	if (line->state == LINE_REQUESTED_VALUES) {
		for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++)
			lines_bitmap_set_bit(&lv.mask, i);

		fd = line_get_fd(line);

		rv = ioctl(fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &lv);
		if (rv < 0)
			return -1;

		for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++)
			values[i] = lines_bitmap_test_bit(lv.bits, i);

	} else if (line->state == LINE_REQUESTED_EVENTS) {
		lines_bitmap_set_bit(&lv.mask, 0);
		for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++) {
			line = gpiod_line_bulk_get_line(bulk, i);

			fd = line_get_fd(line);
			rv = ioctl(fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &lv);
			if (rv < 0)
				return -1;
			values[i] = lines_bitmap_test_bit(lv.bits, 0);
		}
	} else {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

int gpiod_line_set_value(struct gpiod_line *line, int value)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);

	return gpiod_line_set_value_bulk(&bulk, &value);
}

int gpiod_line_set_value_bulk(struct gpiod_line_bulk *bulk, const int *values)
{
	struct gpio_v2_line_values lv;
	struct gpiod_line *line;
	unsigned int i;
	int rv, fd;

	if (!line_bulk_all_requested(bulk))
		return -1;

	memset(&lv, 0, sizeof(lv));

	for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++) {
		lines_bitmap_set_bit(&lv.mask, i);
		lines_bitmap_assign_bit(&lv.bits, i, values && values[i]);
	}

	line = gpiod_line_bulk_get_line(bulk, 0);
	fd = line_get_fd(line);

	rv = ioctl(fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &lv);
	if (rv < 0)
		return -1;

	line_bulk_foreach_line(bulk, line, i)
		line->output_value = lines_bitmap_test_bit(lv.bits, i);

	return 0;
}

int gpiod_line_set_config(struct gpiod_line *line, int direction,
			  int flags, int value)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);

	return gpiod_line_set_config_bulk(&bulk, direction, flags, &value);
}

int gpiod_line_set_config_bulk(struct gpiod_line_bulk *bulk,
			       int direction, int flags,
			       const int *values)
{
	struct gpio_v2_line_config hcfg;
	struct gpiod_line *line;
	unsigned int i;
	int rv, fd;

	if (!line_bulk_all_requested_values(bulk))
		return -1;

	if (!line_request_direction_is_valid(direction))
		return -1;

	memset(&hcfg, 0, sizeof(hcfg));

	line_request_flag_to_gpio_v2_line_config(flags, &hcfg);
	line_request_type_to_gpio_v2_line_config(direction, &hcfg);
	if (direction == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT && values) {
		hcfg.num_attrs = 1;
		hcfg.attrs[0].attr.id = GPIO_V2_LINE_ATTR_ID_OUTPUT_VALUES;
		for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++) {
			lines_bitmap_assign_bit(&hcfg.attrs[0].mask, i, 1);
			lines_bitmap_assign_bit(
				&hcfg.attrs[0].attr.values, i, values[i]);
		}
	}

	line = gpiod_line_bulk_get_line(bulk, 0);
	fd = line_get_fd(line);

	rv = ioctl(fd, GPIO_V2_LINE_SET_CONFIG_IOCTL, &hcfg);
	if (rv < 0)
		return -1;

	line_bulk_foreach_line(bulk, line, i) {
		line->req_flags = flags;
		if (direction == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT)
			line->output_value = lines_bitmap_test_bit(
				hcfg.attrs[0].attr.values, i);

		rv = gpiod_line_update(line);
		if (rv < 0)
			return rv;
	}
	return 0;
}

int gpiod_line_set_flags(struct gpiod_line *line, int flags)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);

	return gpiod_line_set_flags_bulk(&bulk, flags);
}

int gpiod_line_set_flags_bulk(struct gpiod_line_bulk *bulk, int flags)
{
	struct gpiod_line *line;
	int values[GPIOD_LINE_BULK_MAX_LINES];
	unsigned int i;
	int direction;

	line = gpiod_line_bulk_get_line(bulk, 0);
	if (line->direction == GPIOD_LINE_DIRECTION_OUTPUT) {
		line_bulk_foreach_line(bulk, line, i)
			values[i] = line->output_value;

		direction = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
	} else {
		direction = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
	}

	return gpiod_line_set_config_bulk(bulk, direction,
					  flags, values);
}

int gpiod_line_set_direction_input(struct gpiod_line *line)
{
	return gpiod_line_set_config(line, GPIOD_LINE_REQUEST_DIRECTION_INPUT,
				     line->req_flags, 0);
}

int gpiod_line_set_direction_input_bulk(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line;

	line = gpiod_line_bulk_get_line(bulk, 0);
	return gpiod_line_set_config_bulk(bulk,
					  GPIOD_LINE_REQUEST_DIRECTION_INPUT,
					  line->req_flags, NULL);
}

int gpiod_line_set_direction_output(struct gpiod_line *line, int value)
{
	return gpiod_line_set_config(line, GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
				     line->req_flags, value);
}

int gpiod_line_set_direction_output_bulk(struct gpiod_line_bulk *bulk,
					 const int *values)
{
	struct gpiod_line *line;

	line = gpiod_line_bulk_get_line(bulk, 0);
	return gpiod_line_set_config_bulk(bulk,
					  GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
					  line->req_flags, values);
}

int gpiod_line_event_wait(struct gpiod_line *line,
			  const struct timespec *timeout)
{
	struct gpiod_line_bulk bulk = BULK_SINGLE_LINE_INIT(line);

	return gpiod_line_event_wait_bulk(&bulk, timeout, NULL);
}

int gpiod_line_event_wait_bulk(struct gpiod_line_bulk *bulk,
			       const struct timespec *timeout,
			       struct gpiod_line_bulk *event_bulk)
{
	struct pollfd fds[GPIOD_LINE_BULK_MAX_LINES];
	unsigned int off, num_lines;
	struct gpiod_line *line;
	int rv;

	if (!line_bulk_all_requested(bulk))
		return -1;

	memset(fds, 0, sizeof(fds));
	num_lines = gpiod_line_bulk_num_lines(bulk);

	line_bulk_foreach_line(bulk, line, off) {
		fds[off].fd = line_get_fd(line);
		fds[off].events = POLLIN | POLLPRI;
	}

	rv = ppoll(fds, num_lines, timeout, NULL);
	if (rv < 0)
		return -1;
	else if (rv == 0)
		return 0;

	for (off = 0; off < num_lines; off++) {
		if (fds[off].revents) {
			if (fds[off].revents & POLLNVAL) {
				errno = EINVAL;
				return -1;
			}

			if (event_bulk) {
				line = gpiod_line_bulk_get_line(bulk, off);
				rv = gpiod_line_bulk_add_line(event_bulk, line);
				if (rv)
					return -1;
			}

			if (!--rv)
				break;
		}
	}

	return 1;
}

int gpiod_line_event_read(struct gpiod_line *line,
			  struct gpiod_line_event *event)
{
	int ret;

	ret = gpiod_line_event_read_multiple(line, event, 1);
	if (ret < 0)
		return -1;

	return 0;
}

int gpiod_line_event_read_multiple(struct gpiod_line *line,
				   struct gpiod_line_event *events,
				   unsigned int num_events)
{
	int fd;

	fd = gpiod_line_event_get_fd(line);
	if (fd < 0)
		return -1;

	return gpiod_line_event_read_fd_multiple(fd, events, num_events);
}

int gpiod_line_event_get_fd(struct gpiod_line *line)
{
	if (line->state != LINE_REQUESTED_EVENTS) {
		errno = EPERM;
		return -1;
	}

	return line_get_fd(line);
}

int gpiod_line_event_read_fd(int fd, struct gpiod_line_event *event)
{
	int ret;

	ret = gpiod_line_event_read_fd_multiple(fd, event, 1);
	if (ret < 0)
		return -1;

	return 0;
}

int gpiod_line_event_read_fd_multiple(int fd, struct gpiod_line_event *events,
				      unsigned int num_events)
{
	/*
	 * 16 is the maximum number of events the kernel can store in the FIFO
	 * so we can allocate the buffer on the stack.
	 *
	 * NOTE: This is no longer strictly true for uAPI v2.  While 16 is
	 * the default for single line, a request with multiple lines will
	 * have a larger buffer.  So need to rethink the allocation here,
	 * or at least the comment above...
	 */
	struct gpio_v2_line_event evdata[16], *curr;
	struct gpiod_line_event *event;
	unsigned int events_read, i;
	ssize_t rd;

	memset(evdata, 0, sizeof(evdata));

	if (num_events > 16)
		num_events = 16;

	rd = read(fd, evdata, num_events * sizeof(*evdata));
	if (rd < 0) {
		return -1;
	} else if ((unsigned int)rd < sizeof(*evdata)) {
		errno = EIO;
		return -1;
	}

	events_read = rd / sizeof(*evdata);
	if (events_read < num_events)
		num_events = events_read;

	for (i = 0; i < num_events; i++) {
		curr = &evdata[i];
		event = &events[i];

		event->offset = curr->offset;
		event->event_type = curr->id == GPIO_V2_LINE_EVENT_RISING_EDGE
					? GPIOD_LINE_EVENT_RISING_EDGE
					: GPIOD_LINE_EVENT_FALLING_EDGE;
		event->ts.tv_sec = curr->timestamp_ns / 1000000000ULL;
		event->ts.tv_nsec = curr->timestamp_ns % 1000000000ULL;
	}

	return i;
}
