// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.h"

typedef struct {
	PyObject_HEAD;
	struct gpiod_line_config *cfg;
} line_config_object;

static int line_config_init(line_config_object *self,
		       PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(ignored))
{
	self->cfg = gpiod_line_config_new();
	if (!self->cfg) {
		Py_gpiod_SetErrFromErrno();
		return -1;
	}

	return 0;
}

static void line_config_finalize(line_config_object *self)
{
	if (self->cfg)
		gpiod_line_config_free(self->cfg);
}

static unsigned int *make_offsets(PyObject *obj, Py_ssize_t len)
{
	unsigned int *offsets;
	PyObject *offset;
	Py_ssize_t i;

	offsets = PyMem_Calloc(len, sizeof(unsigned int));
	if (!offsets)
		return (unsigned int *)PyErr_NoMemory();

	for (i = 0; i < len; i++) {
		offset = PyList_GetItem(obj, i);
		if (!offset) {
			PyMem_Free(offsets);
			return NULL;
		}

		offsets[i] = Py_gpiod_PyLongAsUnsignedInt(offset);
		if (PyErr_Occurred()) {
			PyMem_Free(offsets);
			return NULL;
		}
	}

	return offsets;
}

static PyObject *
line_config_add_line_settings(line_config_object *self, PyObject *args)
{
	PyObject *offsets_obj, *settings_obj;
	struct gpiod_line_settings *settings;
	unsigned int *offsets;
	Py_ssize_t num_offsets;
	int ret;

	ret = PyArg_ParseTuple(args, "OO", &offsets_obj, &settings_obj);
	if (!ret)
		return NULL;

	num_offsets = PyObject_Size(offsets_obj);
	if (num_offsets < 0)
		return NULL;

	offsets = make_offsets(offsets_obj, num_offsets);
	if (!offsets)
		return NULL;

	settings = Py_gpiod_LineSettingsGetData(settings_obj);
	if (!settings) {
		PyMem_Free(offsets);
		return NULL;
	}

	ret = gpiod_line_config_add_line_settings(self->cfg, offsets,
						  num_offsets, settings);
	PyMem_Free(offsets);
	if (ret)
		return Py_gpiod_SetErrFromErrno();

	Py_RETURN_NONE;
}

static PyMethodDef line_config_methods[] = {
	{
		.ml_name = "add_line_settings",
		.ml_meth = (PyCFunction)line_config_add_line_settings,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

PyTypeObject line_config_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod._ext.LineConfig",
	.tp_basicsize = sizeof(line_config_object),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)line_config_init,
	.tp_finalize = (destructor)line_config_finalize,
	.tp_dealloc = (destructor)Py_gpiod_dealloc,
	.tp_methods = line_config_methods,
};

struct gpiod_line_config *Py_gpiod_LineConfigGetData(PyObject *obj)
{
	line_config_object *line_cfg;
	PyObject *type;

	type = PyObject_Type(obj);
	if (!type)
		return NULL;

	if ((PyTypeObject *)type != &line_config_type) {
		PyErr_SetString(PyExc_TypeError,
				"not a gpiod._ext.LineConfig object");
		Py_DECREF(type);
		return NULL;
	}
	Py_DECREF(type);

	line_cfg = (line_config_object *)obj;

	return line_cfg->cfg;
}
