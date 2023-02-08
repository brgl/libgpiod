// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.h"

/* Generic dealloc callback for all gpiod objects. */
void Py_gpiod_dealloc(PyObject *self)
{
	int ret;

	ret = PyObject_CallFinalizerFromDealloc(self);
	if (ret < 0)
		return;

	PyObject_Del(self);
}

PyObject *Py_gpiod_SetErrFromErrno(void)
{
	PyObject *exc;

	if (errno == ENOMEM)
		return PyErr_NoMemory();

	switch (errno) {
	case EINVAL:
		exc = PyExc_ValueError;
		break;
	case EOPNOTSUPP:
		exc = PyExc_NotImplementedError;
		break;
	case EPIPE:
		exc = PyExc_BrokenPipeError;
		break;
	case ECHILD:
		exc = PyExc_ChildProcessError;
		break;
	case EINTR:
		exc = PyExc_InterruptedError;
		break;
	case EEXIST:
		exc = PyExc_FileExistsError;
		break;
	case ENOENT:
		exc = PyExc_FileNotFoundError;
		break;
	case EISDIR:
		exc = PyExc_IsADirectoryError;
		break;
	case ENOTDIR:
		exc = PyExc_NotADirectoryError;
		break;
	case EPERM:
		exc = PyExc_PermissionError;
		break;
	case ETIMEDOUT:
		exc = PyExc_TimeoutError;
		break;
	default:
		exc = PyExc_OSError;
		break;
	}

	return PyErr_SetFromErrno(exc);
}

PyObject *Py_gpiod_GetGlobalType(const char *type_name)
{
	PyObject *globals;

	globals = PyEval_GetGlobals();
	if (!globals)
		return NULL;

	return PyDict_GetItemString(globals, type_name);
}

unsigned int Py_gpiod_PyLongAsUnsignedInt(PyObject *pylong)
{
	unsigned long tmp;

	tmp = PyLong_AsUnsignedLong(pylong);
	if (PyErr_Occurred())
		return 0;

	if (tmp > UINT_MAX) {
		PyErr_SetString(PyExc_ValueError, "value exceeding UINT_MAX");
		return 0;
	}

	return tmp;
}
