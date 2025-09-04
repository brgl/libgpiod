// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <errno.h>
#include <fcntl.h>
#include <libkmod.h>
#include <libmount.h>
#include <linux/version.h>
#include <pthread.h>
#include <search.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "gpiosim.h"

#define GPIOSIM_API		__attribute__((visibility("default")))
#define UNUSED			__attribute__((unused))
#define MIN_KERNEL_VERSION	KERNEL_VERSION(5, 17, 4)

static pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t id_init_once = PTHREAD_ONCE_INIT;
static void *id_root;

struct {
	int lowest;
	bool found;
} id_find_next_ctx;

struct {
	int id;
	int *idp;
} id_del_ctx;

static void id_cleanup(void)
{
	tdestroy(id_root, free);
}

static void id_schedule_cleanup(void)
{
	atexit(id_cleanup);
}

static int id_compare(const void *p1, const void *p2)
{
	int id1 = *(int *)p1;
	int id2 = *(int *)p2;

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return 1;
	return 0;
}

static void id_find_next(const void *node, VISIT which, int depth UNUSED)
{
	int *id = *(int **)node;

	if (id_find_next_ctx.found)
		return;

	switch (which) {
	case postorder:
	case leaf:
		if (*id != id_find_next_ctx.lowest)
			id_find_next_ctx.found = true;
		else
			id_find_next_ctx.lowest++;
		break;
	default:
		break;
	};
}

static void id_del(const void *node, VISIT which, int depth UNUSED)
{
	int *id = *(int **)node;

	if (id_del_ctx.idp)
		return;

	switch (which) {
	case postorder:
	case leaf:
		if (*id == id_del_ctx.id)
			id_del_ctx.idp = id;
		break;
	default:
		break;
	}
}

static int id_alloc(void)
{
	void *ret;
	int *id;

	pthread_once(&id_init_once, id_schedule_cleanup);

	pthread_mutex_lock(&id_lock);

	id_find_next_ctx.lowest = 0;
	id_find_next_ctx.found = false;

	twalk(id_root, id_find_next);

	id = malloc(sizeof(*id));
	if (!id) {
		pthread_mutex_unlock(&id_lock);
		return -1;
	}

	*id = id_find_next_ctx.lowest;

	ret = tsearch(id, &id_root, id_compare);
	if (!ret) {
		pthread_mutex_unlock(&id_lock);
		/* tsearch() doesn't set errno. */
		errno = ENOMEM;
		return -1;
	}

	pthread_mutex_unlock(&id_lock);

	return *id;
}

static void id_free(int id)
{
	pthread_mutex_lock(&id_lock);

	id_del_ctx.id = id;
	id_del_ctx.idp = NULL;

	twalk(id_root, id_del);
	if (id_del_ctx.idp) {
		tdelete(id_del_ctx.idp, &id_root, id_compare);
		free(id_del_ctx.idp);
	}

	pthread_mutex_unlock(&id_lock);
}

struct refcount {
	unsigned int cnt;
	void (*release)(struct refcount *);
};

static void refcount_init(struct refcount *ref,
			  void (*release)(struct refcount *))
{
	ref->cnt = 1;
	ref->release = release;
}

static void refcount_inc(struct refcount *ref)
{
	ref->cnt++;
}

static void refcount_dec(struct refcount *ref)
{
	ref->cnt--;

	if (!ref->cnt)
		ref->release(ref);
}

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

static void list_init(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

static void list_add(struct list_head *new, struct list_head *head)
{
	struct list_head *prev = head->prev;

	head->prev = new;
	new->next = head;
	new->prev = prev;
	prev->next = new;
}

static void list_del(struct list_head *entry)
{
	struct list_head *prev = entry->prev, *next = entry->next;

	prev->next = next;
	next->prev = prev;
}

#define container_of(ptr, type, member) ({ \
	void *__mptr = (void *)(ptr); \
	((type *)(__mptr - offsetof(type, member))); \
})

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_entry_is_head(pos, head, member) (&pos->member == (head))

#define list_for_each_entry(pos, head, member) \
	for (pos = list_first_entry(head, typeof(*pos), member); \
	     !list_entry_is_head(pos, head, member); \
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, next, head, member) \
	for (pos = list_first_entry(head, typeof(*pos), member), \
	     next = list_next_entry(pos, member); \
	     !list_entry_is_head(pos, head, member); \
	     pos = next, next = list_next_entry(next, member))

static int open_write_close(int base_fd, const char *where, const char *what)
{
	ssize_t written, size;
	int fd;

	if (what)
		size = strlen(what) + 1;
	else
		size = 1;

	fd = openat(base_fd, where, O_WRONLY);
	if (fd < 0)
		return -1;

	written = write(fd, what ?: "", size);
	close(fd);
	if (written < 0) {
		return -1;
	} else if (written != size) {
		errno = EIO;
		return -1;
	}

	return 0;
}

static int open_read_close(int base_fd, const char *where,
			   char *buf, size_t bufsize)
{
	ssize_t rd;
	int fd;

	fd = openat(base_fd, where, O_RDONLY);
	if (fd < 0)
		return -1;

	memset(buf, 0, bufsize);
	rd = read(fd, buf, bufsize);
	close(fd);
	if (rd < 0)
		return -1;

	if (buf[rd - 1] == '\n')
		buf[rd - 1] = '\0';

	return 0;
}

static int check_kernel_version(void)
{
	unsigned int major, minor, release;
	struct utsname un;
	int ret;

	ret = uname(&un);
	if (ret)
		return -1;

	ret = sscanf(un.release, "%u.%u.%u", &major, &minor, &release);
	if (ret != 3) {
		errno = EFAULT;
		return -1;
	}

	if (KERNEL_VERSION(major, minor, release) < MIN_KERNEL_VERSION) {
		errno = EOPNOTSUPP;
		return -1;
	}

	return 0;
}

static int check_gpiosim_module(void)
{
	struct kmod_module *module;
	struct kmod_ctx *kmod;
	const char *modpath;
	int ret, initstate;

	kmod = kmod_new(NULL, NULL);
	if (!kmod)
		return -1;

	ret = kmod_module_new_from_name(kmod, "gpio-sim", &module);
	if (ret)
		goto out_unref_kmod;

again:
	/* First check if the module is already loaded or built-in. */
	initstate = kmod_module_get_initstate(module);
	if (initstate < 0) {
		if (errno == ENOENT) {
			/*
			 * It's not loaded, let's see if we can do it manually.
			 * See if we can find the module.
			 */
			modpath = kmod_module_get_path(module);
			if (!modpath) {
				/* libkmod doesn't set errno. */
				errno = ENOENT;
				ret = -1;
				goto out_unref_module;
			}

			ret = kmod_module_probe_insert_module(
				module, KMOD_PROBE_IGNORE_LOADED, NULL, NULL,
				NULL, NULL);
			if (ret)
				goto out_unref_module;

			goto again;
		} else {
			if (errno == 0)
				errno = EOPNOTSUPP;

			goto out_unref_module;
		}
	}

	if (initstate != KMOD_MODULE_BUILTIN &&
	    initstate != KMOD_MODULE_LIVE &&
	    initstate != KMOD_MODULE_COMING) {
		errno = EPERM;
		goto out_unref_module;
	}

	ret = 0;

out_unref_module:
	kmod_module_unref(module);
out_unref_kmod:
	kmod_unref(kmod);
	return ret;
}

static char *configfs_make_item(int at, int id)
{
	char *item_name, prname[17];
	int ret;

	ret = prctl(PR_GET_NAME, prname);
	if (ret)
		return NULL;

	ret = asprintf(&item_name, "%s.%u.%d", prname, getpid(), id);
	if (ret < 0)
		return NULL;

	ret = mkdirat(at, item_name, 0600);
	if (ret) {
		free(item_name);
		return NULL;
	}

	return item_name;
}

struct gpiosim_ctx {
	struct refcount refcnt;
	int cfs_dir_fd;
	char *cfs_mnt_dir;
};

struct gpiosim_dev {
	struct refcount refcnt;
	struct gpiosim_ctx *ctx;
	bool live;
	char *item_name;
	int id;
	char *dev_name;
	int cfs_dir_fd;
	int sysfs_dir_fd;
	struct list_head banks;
	struct list_head deferred_banks;
};

struct gpiosim_bank {
	struct refcount refcnt;
	struct gpiosim_dev *dev;
	struct list_head siblings;
	struct list_head deferred;
	char *item_name;
	int id;
	char *chip_name;
	char *dev_path;
	int cfs_dir_fd;
	int sysfs_dir_fd;
	size_t num_lines;
	struct list_head lines;
};

struct gpiosim_line {
	struct list_head siblings;
	unsigned int offset;
};

static inline struct gpiosim_ctx *to_gpiosim_ctx(struct refcount *ref)
{
	return container_of(ref, struct gpiosim_ctx, refcnt);
}

static inline struct gpiosim_dev *to_gpiosim_dev(struct refcount *ref)
{
	return container_of(ref, struct gpiosim_dev, refcnt);
}

static inline struct gpiosim_bank *to_gpiosim_bank(struct refcount *ref)
{
	return container_of(ref, struct gpiosim_bank, refcnt);
}

static int ctx_open_configfs_dir(struct gpiosim_ctx *ctx, const char *cfs_path)
{
	char *path;
	int ret;

	ret = asprintf(&path, "%s/gpio-sim", cfs_path);
	if (ret < 0)
		return -1;

	ctx->cfs_dir_fd = open(path, O_RDONLY);
	free(path);
	if (ctx->cfs_dir_fd < 0)
		return -1;

	return 0;
}

/*
 * We don't need to check the configfs module as loading gpio-sim will pull it
 * in but we need to find out if and where configfs was mounted. If it wasn't
 * then as a last resort we'll try to mount it ourselves.
 */
static int ctx_get_configfs_fd(struct gpiosim_ctx *ctx)
{
	struct libmnt_context *mntctx;
	struct libmnt_iter *iter;
	struct libmnt_table *tb;
	struct libmnt_fs *fs;
	const char *type;
	int ret;

	/* Try to find out if and where configfs is mounted. */
	mntctx = mnt_new_context();
	if (!mntctx)
		return -1;

	ret = mnt_context_get_mtab(mntctx, &tb);
	if (ret)
		goto out_free_ctx;

	iter = mnt_new_iter(MNT_ITER_FORWARD);
	if (!iter)
		goto out_free_ctx;

	while (mnt_table_next_fs(tb, iter, &fs) == 0) {
		type = mnt_fs_get_fstype(fs);

		if (strcmp(type, "configfs") == 0) {
			ret = ctx_open_configfs_dir(ctx, mnt_fs_get_target(fs));
			if (ret)
				goto out_free_iter;

			ret = 0;
			goto out_free_iter;
		}
	}

	/* Didn't find any configfs mounts - let's try to do it ourselves. */
	ctx->cfs_mnt_dir = strdup("/tmp/gpiosim-configfs-XXXXXX");
	if (!ctx->cfs_mnt_dir)
		goto out_free_iter;

	ctx->cfs_mnt_dir = mkdtemp(ctx->cfs_mnt_dir);
	if (!ctx->cfs_mnt_dir)
		goto out_free_tmpdir;

	ret = mount(NULL, ctx->cfs_mnt_dir, "configfs", MS_RELATIME, NULL);
	if (ret)
		goto out_rm_tmpdir;

	ret = ctx_open_configfs_dir(ctx, ctx->cfs_mnt_dir);
	if (ret == 0)
		/* Skip unmounting & deleting the tmp directory on success. */
		goto out_free_iter;

	umount(ctx->cfs_mnt_dir);
out_rm_tmpdir:
	rmdir(ctx->cfs_mnt_dir);
out_free_tmpdir:
	free(ctx->cfs_mnt_dir);
	ctx->cfs_mnt_dir = NULL;
out_free_iter:
	mnt_free_iter(iter);
out_free_ctx:
	mnt_free_context(mntctx);

	return ret;
}

static void ctx_release(struct refcount *ref)
{
	struct gpiosim_ctx *ctx = to_gpiosim_ctx(ref);

	close(ctx->cfs_dir_fd);

	if (ctx->cfs_mnt_dir) {
		umount(ctx->cfs_mnt_dir);
		rmdir(ctx->cfs_mnt_dir);
		free(ctx->cfs_mnt_dir);
	}

	free(ctx);
}

GPIOSIM_API struct gpiosim_ctx *gpiosim_ctx_new(void)
{
	struct gpiosim_ctx *ctx;
	int ret;

	ret = check_kernel_version();
	if (ret)
		return NULL;

	ret = check_gpiosim_module();
	if (ret)
		return NULL;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;

	memset(ctx, 0, sizeof(*ctx));
	refcount_init(&ctx->refcnt, ctx_release);

	ret = ctx_get_configfs_fd(ctx);
	if (ret) {
		free(ctx);
		return NULL;
	}

	return ctx;
}

GPIOSIM_API struct gpiosim_ctx *gpiosim_ctx_ref(struct gpiosim_ctx *ctx)
{
	refcount_inc(&ctx->refcnt);

	return ctx;
}

GPIOSIM_API void gpiosim_ctx_unref(struct gpiosim_ctx *ctx)
{
	refcount_dec(&ctx->refcnt);
}

static void dev_release(struct refcount *ref)
{
	struct gpiosim_dev *dev = to_gpiosim_dev(ref);
	struct gpiosim_ctx *ctx = dev->ctx;

	if (dev->live)
		gpiosim_dev_disable(dev);

	unlinkat(ctx->cfs_dir_fd, dev->item_name, AT_REMOVEDIR);
	close(dev->cfs_dir_fd);
	free(dev->dev_name);
	free(dev->item_name);
	id_free(dev->id);
	gpiosim_ctx_unref(ctx);
	free(dev);
}

GPIOSIM_API struct gpiosim_dev *gpiosim_dev_new(struct gpiosim_ctx *ctx)
{
	int configfs_fd, ret, id;
	struct gpiosim_dev *dev;
	char devname[128];
	char *item_name;

	id = id_alloc();
	if (id < 0)
		return NULL;

	item_name = configfs_make_item(ctx->cfs_dir_fd, id);
	if (!item_name)
		goto err_free_id;

	configfs_fd = openat(ctx->cfs_dir_fd, item_name, O_RDONLY);
	if (configfs_fd < 0)
		goto err_unlink;

	dev = malloc(sizeof(*dev));
	if (!dev)
		goto err_close_fd;

	ret = open_read_close(configfs_fd, "dev_name",
			      devname, sizeof(devname));
	if (ret)
		goto err_free_dev;

	memset(dev, 0, sizeof(*dev));
	refcount_init(&dev->refcnt, dev_release);
	list_init(&dev->banks);
	list_init(&dev->deferred_banks);
	dev->cfs_dir_fd = configfs_fd;
	dev->sysfs_dir_fd = -1;
	dev->item_name = item_name;
	dev->id = id;

	dev->dev_name = strdup(devname);
	if (!dev->dev_name)
		goto err_free_dev;

	dev->ctx = gpiosim_ctx_ref(ctx);

	return dev;

err_free_dev:
	free(dev);
err_close_fd:
	close(configfs_fd);
err_unlink:
	unlinkat(ctx->cfs_dir_fd, item_name, AT_REMOVEDIR);
	free(item_name);
err_free_id:
	id_free(id);

	return NULL;
}

GPIOSIM_API struct gpiosim_dev *gpiosim_dev_ref(struct gpiosim_dev *dev)
{
	refcount_inc(&dev->refcnt);

	return dev;
}

GPIOSIM_API void gpiosim_dev_unref(struct gpiosim_dev *dev)
{
	refcount_dec(&dev->refcnt);
}

GPIOSIM_API struct gpiosim_ctx *gpiosim_dev_get_ctx(struct gpiosim_dev *dev)
{
	return gpiosim_ctx_ref(dev->ctx);
}

GPIOSIM_API const char *gpiosim_dev_get_name(struct gpiosim_dev *dev)
{
	return dev->dev_name;
}

static bool dev_check_pending(struct gpiosim_dev *dev)
{
	if (dev->live)
		errno = EBUSY;

	return !dev->live;
}

static bool dev_check_live(struct gpiosim_dev *dev)
{
	if (!dev->live)
		errno = ENODEV;

	return dev->live;
}

static int bank_set_chip_name(struct gpiosim_bank *bank)
{
	char chip_name[32];
	int ret;

	ret = open_read_close(bank->cfs_dir_fd, "chip_name",
			      chip_name, sizeof(chip_name));
	if (ret)
		return -1;

	bank->chip_name = strdup(chip_name);
	if (!bank->chip_name)
		return -1;

	return 0;
}

static int bank_set_dev_path(struct gpiosim_bank *bank)
{
	char dev_path[64];

	snprintf(dev_path, sizeof(dev_path), "/dev/%s", bank->chip_name);

	bank->dev_path = strdup(dev_path);
	if (!bank->dev_path)
		return -1;

	return 0;
}

static int bank_open_sysfs_dir(struct gpiosim_bank *bank)
{
	struct gpiosim_dev *dev = bank->dev;
	int fd;

	fd = openat(dev->sysfs_dir_fd, bank->chip_name, O_RDONLY);
	if (fd < 0)
		return -1;

	bank->sysfs_dir_fd = fd;

	return 0;
}

static int bank_enable(struct gpiosim_bank *bank)
{
	int ret;

	ret = bank_set_chip_name(bank);
	if (ret)
		return -1;

	ret = bank_set_dev_path(bank);
	if (ret)
		return -1;

	return bank_open_sysfs_dir(bank);
}

static int dev_open_sysfs_dir(struct gpiosim_dev *dev)
{
	int ret, fd;
	char *sysp;

	ret = asprintf(&sysp, "/sys/devices/platform/%s", dev->dev_name);
	if (ret < 0)
		return -1;

	fd = open(sysp, O_RDONLY);
	free(sysp);
	if (fd < 0)
		return -1;

	dev->sysfs_dir_fd = fd;

	return 0;
}

/* Closes the sysfs dir for this device and all its child banks. */
static void dev_close_sysfs_dirs(struct gpiosim_dev *dev)
{
	struct gpiosim_bank *bank;

	list_for_each_entry(bank, &dev->banks, siblings) {
		free(bank->chip_name);
		free(bank->dev_path);
		bank->chip_name = bank->dev_path = NULL;
		close(bank->sysfs_dir_fd);
		bank->sysfs_dir_fd = -1;
	}

	close(dev->sysfs_dir_fd);
	dev->sysfs_dir_fd = -1;
}

GPIOSIM_API int gpiosim_dev_enable(struct gpiosim_dev *dev)
{
	struct gpiosim_bank *bank;
	int ret;

	if (!dev_check_pending(dev))
		return -1;

	ret = open_write_close(dev->cfs_dir_fd, "live", "1");
	if (ret)
		return -1;

	ret = dev_open_sysfs_dir(dev);
	if (ret) {
		open_write_close(dev->cfs_dir_fd, "live", "0");
		return -1;
	}

	bank = container_of(&dev->banks, struct gpiosim_bank, siblings);

	list_for_each_entry(bank, &dev->banks, siblings) {
		ret = bank_enable(bank);
		if (ret) {
			dev_close_sysfs_dirs(dev);
			open_write_close(dev->cfs_dir_fd, "live", "0");
			return -1;
		}
	}

	dev->live = true;

	return 0;
}

static void bank_release_finish(struct gpiosim_bank *bank)
{
	struct gpiosim_dev *dev = bank->dev;
	struct gpiosim_line *line, *tmp;
	char buf[64];

	list_for_each_entry_safe(line, tmp, &bank->lines, siblings) {
		snprintf(buf, sizeof(buf), "line%u/hog", line->offset);
		unlinkat(bank->cfs_dir_fd, buf, AT_REMOVEDIR);

		snprintf(buf, sizeof(buf), "line%u", line->offset);
		unlinkat(bank->cfs_dir_fd, buf, AT_REMOVEDIR);

		list_del(&line->siblings);
		free(line);
	}

	close(bank->cfs_dir_fd);
	unlinkat(dev->cfs_dir_fd, bank->item_name, AT_REMOVEDIR);
	free(bank->item_name);
	id_free(bank->id);
	free(bank);
}

GPIOSIM_API int gpiosim_dev_disable(struct gpiosim_dev *dev)
{
	struct gpiosim_bank *bank, *tmp;
	int ret;

	if (!dev_check_live(dev))
		return -1;

	ret = open_write_close(dev->cfs_dir_fd, "live", "0");
	if (ret)
		return ret;

	list_for_each_entry_safe(bank, tmp, &dev->deferred_banks, deferred) {
		list_del(&bank->deferred);
		bank_release_finish(bank);
	}

	dev_close_sysfs_dirs(dev);

	dev->live = false;

	return 0;
}

GPIOSIM_API bool gpiosim_dev_is_live(struct gpiosim_dev *dev)
{
	return dev->live;
}

static void bank_release(struct refcount *ref)
{
	struct gpiosim_bank *bank = to_gpiosim_bank(ref);
	struct gpiosim_dev *dev = bank->dev;

	list_del(&bank->siblings);

	/*
	 * If the device is still active, defer removing the bank directories
	 * until it's disabled. Otherwise, do it now.
	 */
	if (dev->live)
		list_add(&bank->deferred, &dev->deferred_banks);
	else
		bank_release_finish(bank);

	gpiosim_dev_unref(dev);
}

GPIOSIM_API struct gpiosim_bank *gpiosim_bank_new(struct gpiosim_dev *dev)
{
	struct gpiosim_bank *bank;
	int configfs_fd, id;
	char *item_name;

	if (!dev_check_pending(dev))
		return NULL;

	id = id_alloc();
	if (id < 0)
		return NULL;

	item_name = configfs_make_item(dev->cfs_dir_fd, id);
	if (!item_name)
		goto err_free_id;

	configfs_fd = openat(dev->cfs_dir_fd, item_name, O_RDONLY);
	if (configfs_fd < 0)
		goto err_unlink;

	bank = malloc(sizeof(*bank));
	if (!bank)
		goto err_close_cfs;

	memset(bank, 0, sizeof(*bank));

	refcount_init(&bank->refcnt, bank_release);
	list_add(&bank->siblings, &dev->banks);
	bank->cfs_dir_fd = configfs_fd;
	bank->dev = gpiosim_dev_ref(dev);
	bank->item_name = item_name;
	bank->num_lines = 1;
	bank->id = id;
	list_init(&bank->lines);

	return bank;

err_close_cfs:
	close(configfs_fd);
err_unlink:
	unlinkat(dev->cfs_dir_fd, item_name, AT_REMOVEDIR);
err_free_id:
	id_free(id);

	return NULL;
}

GPIOSIM_API struct gpiosim_bank *gpiosim_bank_ref(struct gpiosim_bank *bank)
{
	refcount_inc(&bank->refcnt);

	return bank;
}

GPIOSIM_API void gpiosim_bank_unref(struct gpiosim_bank *bank)
{
	refcount_dec(&bank->refcnt);
}

GPIOSIM_API struct gpiosim_dev *gpiosim_bank_get_dev(struct gpiosim_bank *bank)
{
	return gpiosim_dev_ref(bank->dev);
}

GPIOSIM_API const char *gpiosim_bank_get_chip_name(struct gpiosim_bank *bank)
{
	return bank->chip_name;
}

GPIOSIM_API const char *gpiosim_bank_get_dev_path(struct gpiosim_bank *bank)
{
	return bank->dev_path;
}

GPIOSIM_API int gpiosim_bank_set_label(struct gpiosim_bank *bank,
				       const char *label)
{
	if (!dev_check_pending(bank->dev))
		return -1;

	return open_write_close(bank->cfs_dir_fd, "label", label);
}

GPIOSIM_API int gpiosim_bank_set_num_lines(struct gpiosim_bank *bank,
					   size_t num_lines)
{
	char buf[32];
	int ret;

	if (!dev_check_pending(bank->dev))
		return -1;

	snprintf(buf, sizeof(buf), "%zu", num_lines);

	ret = open_write_close(bank->cfs_dir_fd, "num_lines", buf);
	if (ret)
		return -1;

	bank->num_lines = num_lines;

	return 0;
}

static int bank_make_line_dir(struct gpiosim_bank *bank, unsigned int offset)
{
	struct gpiosim_line *line;
	char buf[32];
	int ret;

	snprintf(buf, sizeof(buf), "line%u", offset);

	ret = faccessat(bank->cfs_dir_fd, buf, W_OK, 0);
	if (!ret)
		return 0;
	if (ret && errno != ENOENT)
		return -1;

	line = malloc(sizeof(*line));
	if (!line)
		return -1;

	ret = mkdirat(bank->cfs_dir_fd, buf, O_RDONLY);
	if (ret) {
		free(line);
		return -1;
	}

	memset(line, 0, sizeof(*line));
	line->offset = offset;
	list_add(&line->siblings, &bank->lines);

	return 0;
}

GPIOSIM_API int gpiosim_bank_set_line_name(struct gpiosim_bank *bank,
					   unsigned int offset,
					   const char *name)
{
	char buf[32];
	int ret;

	if (!dev_check_pending(bank->dev))
		return -1;

	ret = bank_make_line_dir(bank, offset);
	if (ret)
		return -1;

	snprintf(buf, sizeof(buf), "line%u/name", offset);

	return open_write_close(bank->cfs_dir_fd, buf, name ?: "");
}

GPIOSIM_API int gpiosim_bank_hog_line(struct gpiosim_bank *bank,
				      unsigned int offset, const char *name,
				      enum gpiosim_direction direction)
{
	char buf[64], *dir;
	int ret, fd;

	switch (direction) {
	case GPIOSIM_DIRECTION_INPUT:
		dir = "input";
		break;
	case GPIOSIM_DIRECTION_OUTPUT_HIGH:
		dir = "output-high";
		break;
	case GPIOSIM_DIRECTION_OUTPUT_LOW:
		dir = "output-low";
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	if (!dev_check_pending(bank->dev))
		return -1;

	ret = bank_make_line_dir(bank, offset);
	if (ret)
		return -1;

	snprintf(buf, sizeof(buf), "line%u/hog", offset);

	ret = faccessat(bank->cfs_dir_fd, buf, W_OK, 0);
	if (ret) {
		if (errno == ENOENT) {
			ret = mkdirat(bank->cfs_dir_fd, buf, O_RDONLY);
			if (ret)
				return -1;
		} else {
			return -1;
		}
	}

	fd = openat(bank->cfs_dir_fd, buf, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = open_write_close(fd, "name", name ?: "");
	if (ret) {
		close(fd);
		return -1;
	}

	ret = open_write_close(fd, "direction", dir);
	close(fd);
	return ret;
}

GPIOSIM_API int gpiosim_bank_clear_hog(struct gpiosim_bank *bank,
				       unsigned int offset)
{
	char buf[64];
	
	if (!dev_check_pending(bank->dev))
		return -1;

	snprintf(buf, sizeof(buf), "line%u/hog", offset);

	return unlinkat(bank->cfs_dir_fd, buf, AT_REMOVEDIR);
}

GPIOSIM_API int gpiosim_bank_set_line_valid(struct gpiosim_bank *bank,
					    unsigned int offset, bool valid)
{
	char buf[32];
	int ret;

	if (!dev_check_pending(bank->dev))
		return -1;

	ret = bank_make_line_dir(bank, offset);
	if (ret)
		return -1;

	snprintf(buf, sizeof(buf), "line%u/valid", offset);

	return open_write_close(bank->cfs_dir_fd, buf, valid ? "1" : "0");
}

static int sysfs_read_bank_attr(struct gpiosim_bank *bank, unsigned int offset,
				const char *attr, char *buf, size_t bufsize)
{
	struct gpiosim_dev *dev = bank->dev;
	char where[32];

	if (!dev_check_live(dev))
		return -1;

	snprintf(where, sizeof(where), "sim_gpio%u/%s", offset, attr);

	return open_read_close(bank->sysfs_dir_fd, where, buf, bufsize);
}

GPIOSIM_API enum
gpiosim_value gpiosim_bank_get_value(struct gpiosim_bank *bank,
				     unsigned int offset)
{
	char what[3];
	int ret;

	ret = sysfs_read_bank_attr(bank, offset, "value", what, sizeof(what));
	if (ret)
		return GPIOSIM_VALUE_ERROR;

	if (what[0] == '0')
		return GPIOSIM_VALUE_INACTIVE;
	if (what[0] == '1')
		return GPIOSIM_VALUE_ACTIVE;

	errno = EIO;
	return GPIOSIM_VALUE_ERROR;
}

GPIOSIM_API enum gpiosim_pull
gpiosim_bank_get_pull(struct gpiosim_bank *bank, unsigned int offset)
{
	char what[16];
	int ret;

	ret = sysfs_read_bank_attr(bank, offset, "pull", what, sizeof(what));
	if (ret)
		return GPIOSIM_PULL_ERROR;

	if (strcmp(what, "pull-down") == 0)
		return GPIOSIM_PULL_DOWN;
	if (strcmp(what, "pull-up") == 0)
		return GPIOSIM_PULL_UP;

	errno = EIO;
	return GPIOSIM_PULL_ERROR;
}

GPIOSIM_API int
gpiosim_bank_set_pull(struct gpiosim_bank *bank,
		      unsigned int offset, enum gpiosim_pull pull)
{
	struct gpiosim_dev *dev = bank->dev;
	char where[32], what[16];

	if (!dev_check_live(dev))
		return -1;

	if (pull != GPIOSIM_PULL_DOWN && pull != GPIOSIM_PULL_UP) {
		errno = EINVAL;
		return -1;
	}

	snprintf(where, sizeof(where), "sim_gpio%u/pull", offset);
	snprintf(what, sizeof(what),
		 pull == GPIOSIM_PULL_DOWN ? "pull-down" : "pull-up");

	return open_write_close(bank->sysfs_dir_fd, where, what);
}
