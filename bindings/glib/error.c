// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <errno.h>
#include <glib.h>
#include <gpiod-glib.h>
#include <stdarg.h>

G_DEFINE_QUARK(g-gpiod-error, gpiodglib_error)

static GpiodglibError error_from_errno(void)
{
	switch (errno) {
	case EPERM:
		return GPIODGLIB_ERR_PERM;
	case ENOENT:
		return GPIODGLIB_ERR_NOENT;
	case EINTR:
		return GPIODGLIB_ERR_INTR;
	case EIO:
		return GPIODGLIB_ERR_IO;
	case ENXIO:
		return GPIODGLIB_ERR_NXIO;
	case E2BIG:
		return GPIODGLIB_ERR_E2BIG;
	case EBADFD:
		return GPIODGLIB_ERR_BADFD;
	case ECHILD:
		return GPIODGLIB_ERR_CHILD;
	case EAGAIN:
		return GPIODGLIB_ERR_AGAIN;
	case ENOMEM:
		/* Special case - as a convention GLib just aborts on ENOMEM. */
		g_error("out of memory");
	case EACCES:
		return GPIODGLIB_ERR_ACCES;
	case EFAULT:
		return GPIODGLIB_ERR_FAULT;
	case EBUSY:
		return GPIODGLIB_ERR_BUSY;
	case EEXIST:
		return GPIODGLIB_ERR_EXIST;
	case ENODEV:
		return GPIODGLIB_ERR_NODEV;
	case EINVAL:
		return GPIODGLIB_ERR_INVAL;
	case ENOTTY:
		return GPIODGLIB_ERR_NOTTY;
	case EPIPE:
		return GPIODGLIB_ERR_PIPE;
	default:
		return GPIODGLIB_ERR_FAILED;
	}
}

void _gpiodglib_set_error_from_errno(GError **err, const gchar *fmt, ...)
{
	g_autofree gchar *msg = NULL;
	va_list va;

	va_start(va, fmt);
	msg = g_strdup_vprintf(fmt, va);
	va_end(va);

	g_set_error(err, GPIODGLIB_ERROR, error_from_errno(),
		    "%s: %s", msg, g_strerror(errno));
}
