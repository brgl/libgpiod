/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl> */

#ifndef __LIBGPIOD_PYTHON_MODULE_H__
#define __LIBGPIOD_PYTHON_MODULE_H__

#include <gpiod.h>
#include <Python.h>

PyObject *Py_gpiod_SetErrFromErrno(void);
PyObject *Py_gpiod_GetGlobalType(const char *type_name);
unsigned int Py_gpiod_PyLongAsUnsignedInt(PyObject *pylong);
void Py_gpiod_dealloc(PyObject *self);
PyObject *Py_gpiod_MakeRequestObject(struct gpiod_line_request *request,
				     size_t event_buffer_size);
struct gpiod_line_config *Py_gpiod_LineConfigGetData(PyObject *obj);
struct gpiod_line_settings *Py_gpiod_LineSettingsGetData(PyObject *obj);

#endif /* __LIBGPIOD_PYTHON_MODULE_H__ */
