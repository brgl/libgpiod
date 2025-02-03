// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <gpiosim.h>
#include <Python.h>

struct module_const {
	const char *name;
	long val;
};

static const struct module_const module_constants[] = {
	{
		.name = "PULL_DOWN",
		.val = GPIOSIM_PULL_DOWN,
	},
	{
		.name = "PULL_UP",
		.val = GPIOSIM_PULL_UP,
	},
	{
		.name = "VALUE_INACTIVE",
		.val = GPIOSIM_VALUE_INACTIVE,
	},
	{
		.name = "VALUE_ACTIVE",
		.val = GPIOSIM_VALUE_ACTIVE,
	},
	{
		.name = "DIRECTION_INPUT",
		.val = GPIOSIM_DIRECTION_INPUT,
	},
	{
		.name = "DIRECTION_OUTPUT_HIGH",
		.val = GPIOSIM_DIRECTION_OUTPUT_HIGH,
	},
	{
		.name = "DIRECTION_OUTPUT_LOW",
		.val = GPIOSIM_DIRECTION_OUTPUT_LOW,
	},
	{ }
};

struct module_state {
	struct gpiosim_ctx *sim_ctx;
};

static void free_module_state(void *mod)
{
	struct module_state *state = PyModule_GetState((PyObject *)mod);

	if (state->sim_ctx)
		gpiosim_ctx_unref(state->sim_ctx);
}

static PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	.m_name = "gpiosim._ext",
	.m_size = sizeof(struct module_state),
	.m_free = free_module_state,
};

typedef struct {
	PyObject_HEAD
	struct gpiosim_dev *dev;
	struct gpiosim_bank *bank;
} chip_object;

static int chip_init(chip_object *self,
		     PyObject *Py_UNUSED(ignored0),
		     PyObject *Py_UNUSED(ignored1))
{
	struct module_state *state;
	PyObject *mod;

	mod = PyState_FindModule(&module_def);
	if (!mod)
		return -1;

	state = PyModule_GetState(mod);

	self->dev = gpiosim_dev_new(state->sim_ctx);
	if (!self->dev) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}

	self->bank = gpiosim_bank_new(self->dev);
	if (!self->bank) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}

	return 0;
}

static void chip_finalize(chip_object *self)
{
	if (self->dev) {
		if (gpiosim_dev_is_live(self->dev))
			gpiosim_dev_disable(self->dev);
	}

	if (self->bank)
		gpiosim_bank_unref(self->bank);

	if (self->dev)
		gpiosim_dev_unref(self->dev);
}

static void chip_dealloc(PyObject *self)
{
	int ret;

	ret = PyObject_CallFinalizerFromDealloc(self);
	if (ret < 0)
		return;

	PyObject_Del(self);
}

static PyObject *chip_dev_path(chip_object *self, void *Py_UNUSED(ignored))
{
	return PyUnicode_FromString(gpiosim_bank_get_dev_path(self->bank));
}

static PyObject *chip_name(chip_object *self, void *Py_UNUSED(ignored))
{
	return PyUnicode_FromString(gpiosim_bank_get_chip_name(self->bank));
}

static PyGetSetDef chip_getset[] = {
	{
		.name = "dev_path",
		.get = (getter)chip_dev_path,
	},
	{
		.name = "name",
		.get = (getter)chip_name,
	},
	{ }
};

static PyObject *chip_set_label(chip_object *self, PyObject *args)
{
	const char *label;
	int ret;

	ret = PyArg_ParseTuple(args, "s", &label);
	if (!ret)
		return NULL;

	ret = gpiosim_bank_set_label(self->bank, label);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyObject *chip_set_num_lines(chip_object *self, PyObject *args)
{
	unsigned int num_lines;
	int ret;

	ret = PyArg_ParseTuple(args, "I", &num_lines);
	if (!ret)
		return NULL;

	ret = gpiosim_bank_set_num_lines(self->bank, num_lines);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyObject *chip_set_line_name(chip_object *self, PyObject *args)
{
	unsigned int offset;
	const char *name;
	int ret;

	ret = PyArg_ParseTuple(args, "Is", &offset, &name);
	if (!ret)
		return NULL;

	ret = gpiosim_bank_set_line_name(self->bank, offset, name);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyObject *chip_set_hog(chip_object *self, PyObject *args)
{
	unsigned int offset;
	const char *name;
	int ret, dir;

	ret = PyArg_ParseTuple(args, "Isi", &offset, &name, &dir);
	if (!ret)
		return NULL;

	ret = gpiosim_bank_hog_line(self->bank, offset, name, dir);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyObject *chip_enable(chip_object *self, PyObject *Py_UNUSED(args))
{
	int ret;

	ret = gpiosim_dev_enable(self->dev);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyObject *chip_get_value(chip_object *self, PyObject *args)
{
	unsigned int offset;
	int ret, val;

	ret = PyArg_ParseTuple(args, "I", &offset);
	if (!ret)
		return NULL;

	val = gpiosim_bank_get_value(self->bank, offset);
	if (val < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	return PyLong_FromLong(val);
}

static PyObject *chip_set_pull(chip_object *self, PyObject *args)
{
	unsigned int offset;
	int ret, pull;

	ret = PyArg_ParseTuple(args, "II", &offset, &pull);
	if (!ret)
		return NULL;

	ret = gpiosim_bank_set_pull(self->bank, offset, pull);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyMethodDef chip_methods[] = {
	{
		.ml_name = "set_label",
		.ml_meth = (PyCFunction)chip_set_label,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_num_lines",
		.ml_meth = (PyCFunction)chip_set_num_lines,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_line_name",
		.ml_meth = (PyCFunction)chip_set_line_name,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_hog",
		.ml_meth = (PyCFunction)chip_set_hog,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "enable",
		.ml_meth = (PyCFunction)chip_enable,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "get_value",
		.ml_meth = (PyCFunction)chip_get_value,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_pull",
		.ml_meth = (PyCFunction)chip_set_pull,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

static PyTypeObject chip_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiosim.Chip",
	.tp_basicsize = sizeof(chip_object),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)chip_init,
	.tp_finalize = (destructor)chip_finalize,
	.tp_dealloc = (destructor)chip_dealloc,
	.tp_methods = chip_methods,
	.tp_getset = chip_getset,
};

PyMODINIT_FUNC PyInit__ext(void)
{
	const struct module_const *modconst;
	struct module_state *state;
	PyObject *module;
	int ret;

	module = PyModule_Create(&module_def);
	if (!module)
		return NULL;

	ret = PyState_AddModule(module, &module_def);
	if (ret) {
		Py_DECREF(module);
		return NULL;
	}

	state = PyModule_GetState(module);

	state->sim_ctx = gpiosim_ctx_new();
	if (!state->sim_ctx) {
		Py_DECREF(module);
		return PyErr_SetFromErrno(PyExc_OSError);
	}

	ret = PyModule_AddType(module, &chip_type);
	if (ret) {
		Py_DECREF(module);
		return NULL;
	}

	for (modconst = module_constants; modconst->name; modconst++) {
		ret = PyModule_AddIntConstant(module,
					      modconst->name, modconst->val);
		if (ret) {
			Py_DECREF(module);
			return NULL;
		}
	}

	return module;
}
