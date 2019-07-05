/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <libkmod.h>
#include <libudev.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "gpio-mockup.h"

#define EXPORT			__attribute__((visibility("default")))

static const unsigned int min_kern_major = 5;
static const unsigned int min_kern_minor = 1;
static const unsigned int min_kern_release = 0;

struct gpio_mockup_chip {
	char *name;
	char *path;
	unsigned int num;
};

struct gpio_mockup {
	struct gpio_mockup_chip **chips;
	unsigned int num_chips;
	struct kmod_ctx *kmod;
	struct kmod_module *module;
	int refcount;
};

static void free_chip(struct gpio_mockup_chip *chip)
{
	free(chip->name);
	free(chip->path);
	free(chip);
}

static bool check_kernel_version(void)
{
	unsigned int major, minor, release;
	struct utsname un;
	int rv;

	rv = uname(&un);
	if (rv)
		return false;

	rv = sscanf(un.release, "%u.%u.%u", &major, &minor, &release);
	if (rv != 3) {
		errno = EFAULT;
		return false;
	}

	if (major < min_kern_major) {
		goto bad_version;
	} else if (major > min_kern_major) {
		goto good_version;
	} else {
		if (minor < min_kern_minor) {
			goto bad_version;
		} else if (minor > min_kern_minor) {
			goto good_version;
		} else {
			if (release < min_kern_release)
				goto bad_version;
			else
				goto good_version;
		}
	}

good_version:
	return true;

bad_version:
	errno = EOPNOTSUPP;
	return false;
}

EXPORT struct gpio_mockup *gpio_mockup_new(void)
{
	struct gpio_mockup *ctx;
	const char *modpath;
	int rv;

	if (!check_kernel_version())
		goto err_out;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		goto err_out;

	memset(ctx, 0, sizeof(*ctx));
	ctx->refcount = 1;

	ctx->kmod = kmod_new(NULL, NULL);
	if (!ctx->kmod)
		goto err_free_kmod;

	rv = kmod_module_new_from_name(ctx->kmod, "gpio-mockup", &ctx->module);
	if (rv)
		goto err_unref_module;

	/* First see if we can find the module. */
	modpath = kmod_module_get_path(ctx->module);
	if (!modpath) {
		errno = ENOENT;
		goto err_unref_module;
	}

	/*
	 * Then see if we can freely load and unload it. If it's already
	 * loaded - no problem, we'll remove it next anyway.
	 */
	rv = kmod_module_probe_insert_module(ctx->module, 0,
					     "gpio_mockup_ranges=-1,4",
					     NULL, NULL, NULL);
	if (rv)
		goto err_unref_module;

	/* We need to chech that the gpio-mockup debugfs directory exists. */
	rv = access("/sys/kernel/debug/gpio-mockup", R_OK);
	if (rv)
		goto err_unref_module;

	rv = kmod_module_remove_module(ctx->module, 0);
	if (rv)
		goto err_unref_module;

	return ctx;

err_unref_module:
	kmod_unref(ctx->kmod);
err_free_kmod:
	free(ctx);
err_out:
	return NULL;
}

EXPORT void gpio_mockup_ref(struct gpio_mockup *ctx)
{
	ctx->refcount++;
}

EXPORT void gpio_mockup_unref(struct gpio_mockup *ctx)
{
	ctx->refcount--;

	if (ctx->refcount == 0) {
		if (ctx->chips)
			gpio_mockup_remove(ctx);

		kmod_module_unref(ctx->module);
		kmod_unref(ctx->kmod);
		free(ctx);
	}
}

static char *make_module_param_string(unsigned int num_chips,
				      unsigned int *num_lines, int flags)
{
	char *params, *new;
	unsigned int i;
	int rv;

	params = strdup("gpio_mockup_ranges=");
	if (!params)
		return NULL;

	for (i = 0; i < num_chips; i++) {
		rv = asprintf(&new, "%s-1,%u,", params, num_lines[i]);
		free(params);
		if (rv < 0)
			return NULL;

		params = new;
	}
	params[strlen(params) - 1] = '\0'; /* Remove the last comma. */

	if (flags & GPIO_MOCKUP_FLAG_NAMED_LINES) {
		rv = asprintf(&new, "%s gpio_mockup_named_lines", params);
		free(params);
		if (rv < 0)
			return NULL;

		params = new;
	}

	return params;
}

static bool devpath_is_mockup(const char *devpath)
{
	static const char mockup_devpath[] = "/devices/platform/gpio-mockup";

	return !strncmp(devpath, mockup_devpath, sizeof(mockup_devpath) - 1);
}

static int chipcmp(const void *c1, const void *c2)
{
	const struct gpio_mockup_chip *chip1, *chip2;

	chip1 = *(const struct gpio_mockup_chip **)c1;
	chip2 = *(const struct gpio_mockup_chip **)c2;

	return chip1->num > chip2->num;
}

static struct gpio_mockup_chip *make_chip(const char *sysname,
					  const char *devnode)
{
	struct gpio_mockup_chip *chip;
	int rv;

	chip = malloc(sizeof(*chip));
	if (!chip)
		return NULL;

	chip->name = strdup(sysname);
	if (!chip->name) {
		free(chip);
		return NULL;
	}

	chip->path = strdup(devnode);
	if (!chip->path) {
		free(chip->name);
		free(chip);
		return NULL;
	}

	rv = sscanf(sysname, "gpiochip%u", &chip->num);
	if (rv != 1) {
		errno = EINVAL;
		free(chip->path);
		free(chip->name);
		free(chip);
		return NULL;
	}

	return chip;
}

EXPORT int gpio_mockup_probe(struct gpio_mockup *ctx, unsigned int num_chips,
			     unsigned int *chip_sizes, int flags)
{
	const char *devpath, *devnode, *sysname, *action;
	struct gpio_mockup_chip *chip;
	struct udev_monitor *monitor;
	unsigned int i, detected = 0;
	struct udev_device *dev;
	struct udev *udev_ctx;
	struct pollfd pfd;
	char *params;
	int rv;

	if (ctx->chips) {
		errno = EBUSY;
		goto err_out;
	}

	if (num_chips < 1) {
		errno = EINVAL;
		goto err_out;
	}

	udev_ctx = udev_new();
	if (!udev_ctx)
		goto err_out;

	monitor = udev_monitor_new_from_netlink(udev_ctx, "udev");
	if (!monitor)
		goto err_unref_udev;

	rv = udev_monitor_filter_add_match_subsystem_devtype(monitor,
							     "gpio", NULL);
	if (rv < 0)
		goto err_unref_monitor;

	rv = udev_monitor_enable_receiving(monitor);
	if (rv < 0)
		goto err_unref_monitor;

	params = make_module_param_string(num_chips, chip_sizes, flags);
	if (!params)
		goto err_unref_monitor;

	rv = kmod_module_probe_insert_module(ctx->module,
					     KMOD_PROBE_FAIL_ON_LOADED,
					     params, NULL, NULL, NULL);
	free(params);
	if (rv)
		goto err_unref_monitor;

	ctx->chips = calloc(num_chips, sizeof(struct gpio_mockup_chip *));
	if (!ctx->chips)
		goto err_remove_module;

	pfd.fd = udev_monitor_get_fd(monitor);
	pfd.events = POLLIN | POLLPRI;

	while (num_chips > detected) {
		rv = poll(&pfd, 1, 5000);
		if (rv < 0) {
			goto err_free_chips;
		} if (rv == 0) {
			errno = EAGAIN;
			goto err_free_chips;
		}

		dev = udev_monitor_receive_device(monitor);
		if (!dev)
			goto err_free_chips;

		devpath = udev_device_get_devpath(dev);
		devnode = udev_device_get_devnode(dev);
		sysname = udev_device_get_sysname(dev);
		action = udev_device_get_action(dev);

		if (!devpath || !devnode || !sysname ||
		    !devpath_is_mockup(devpath) || strcmp(action, "add") != 0) {
			udev_device_unref(dev);
			continue;
		}

		chip = make_chip(sysname, devnode);
		if (!chip)
			goto err_free_chips;

		ctx->chips[detected++] = chip;
		udev_device_unref(dev);
	}

	udev_monitor_unref(monitor);
	udev_unref(udev_ctx);

	/*
	 * We can't assume that the order in which the mockup gpiochip
	 * devices are created will be deterministic, yet we want the
	 * index passed to the test_chip_*() functions to correspond with the
	 * order in which the chips were defined in the TEST_DEFINE()
	 * macro.
	 *
	 * Once all gpiochips are there, sort them by chip number.
	 */
	qsort(ctx->chips, ctx->num_chips, sizeof(*ctx->chips), chipcmp);

	return 0;

err_free_chips:
	for (i = 0; i < detected; i++)
		free_chip(ctx->chips[i]);
	free(ctx->chips);
err_remove_module:
	kmod_module_remove_module(ctx->module, 0);
err_unref_monitor:
	udev_monitor_unref(monitor);
err_unref_udev:
	udev_unref(udev_ctx);
err_out:
	return -1;
}

EXPORT int gpio_mockup_remove(struct gpio_mockup *ctx)
{
	unsigned int i;
	int rv;

	if (!ctx->chips) {
		errno = ENODEV;
		return -1;
	}

	rv = kmod_module_remove_module(ctx->module, 0);
	if (rv)
		return -1;

	for (i = 0; i < ctx->num_chips; i++)
		free_chip(ctx->chips[i]);
	free(ctx->chips);
	ctx->chips = NULL;
	ctx->num_chips = 0;

	return 0;
}

EXPORT const char *
gpio_mockup_chip_name(struct gpio_mockup *ctx, unsigned int idx)
{
	if (!ctx->chips) {
		errno = ENODEV;
		return NULL;
	}

	return ctx->chips[idx]->name;
}

EXPORT const char *
gpio_mockup_chip_path(struct gpio_mockup *ctx, unsigned int idx)
{
	if (!ctx->chips) {
		errno = ENODEV;
		return NULL;
	}

	return ctx->chips[idx]->path;
}

EXPORT int gpio_mockup_chip_num(struct gpio_mockup *ctx, unsigned int idx)
{
	if (!ctx->chips) {
		errno = ENODEV;
		return -1;
	}

	return ctx->chips[idx]->num;
}

static int debugfs_open(unsigned int chip_num,
			unsigned int line_offset, int flags)
{
	char *path;
	int fd, rv;

	rv = asprintf(&path, "/sys/kernel/debug/gpio-mockup/gpiochip%u/%u",
		      chip_num, line_offset);
	if (rv < 0)
		return -1;

	fd = open(path, flags);
	free(path);

	return fd;
}

EXPORT int gpio_mockup_get_value(struct gpio_mockup *ctx,
				 unsigned int chip_idx,
				 unsigned int line_offset)
{
	ssize_t rd;
	char buf;
	int fd;

	if (!ctx->chips) {
		errno = ENODEV;
		return -1;
	}

	fd = debugfs_open(ctx->chips[chip_idx]->num, line_offset, O_RDONLY);
	if (fd < 0)
		return fd;

	rd = read(fd, &buf, 1);
	close(fd);
	if (rd < 0)
		return rd;
	if (rd != 1) {
		errno = ENOTTY;
		return -1;
	}
	if (buf != '0' && buf != '1') {
		errno = EIO;
		return -1;
	}

	return buf == '0' ? 0 : 1;
}

EXPORT int gpio_mockup_set_pull(struct gpio_mockup *ctx,
				unsigned int chip_idx,
				unsigned int line_offset, int pull)
{
	char buf[2];
	ssize_t wr;
	int fd;

	if (!ctx->chips) {
		errno = ENODEV;
		return -1;
	}

	fd = debugfs_open(ctx->chips[chip_idx]->num, line_offset, O_WRONLY);
	if (fd < 0)
		return fd;

	buf[0] = pull ? '1' : '0';
	buf[1] = '\n';

	wr = write(fd, &buf, sizeof(buf));
	close(fd);
	if (wr < 0)
		return wr;
	if (wr != sizeof(buf)) {
		errno = EAGAIN;
		return -1;
	}

	return 0;
}
