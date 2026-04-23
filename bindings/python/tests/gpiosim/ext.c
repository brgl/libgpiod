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

	state = PyType_GetModuleState(Py_TYPE(self));

	if (!state)
		return -1;

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
	PyTypeObject *tp = Py_TYPE(self);

	ret = PyObject_CallFinalizerFromDealloc(self);
	if (ret < 0)
		return;

	PyObject_Free(self);
	Py_DECREF(tp);
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

static PyType_Slot chip_type_slots[] = {
	{Py_tp_new, PyType_GenericNew},
	{Py_tp_init, (initproc)chip_init},
	{Py_tp_finalize, (destructor)chip_finalize},
	{Py_tp_dealloc, (destructor)chip_dealloc},
	{Py_tp_methods, chip_methods},
	{Py_tp_getset, chip_getset},
	{0, 0}
};

/*
 * See xxlimited.c and _bz2module.c for inspiration.
 *
 * As part of transitioning to multi-phase module initialization, the
 * gpiosim.Chip type needs to become heap allocated so that it can access
 * module state.
 *
 * We disallow subclassing by not specifying Py_TPFLAGS_BASETYPE. This
 * allows the module to use PyType_GetModuleState() since it may otherwise
 * not return the proper module if a subclass is invoking the method.
 *
 * Note:
 * We do not hold PyObject references so no reference cycles should exist. As
 * such, we do not set Py_TPFLAGS_HAVE_GC nor define either tp_traverse or
 * tp_clear.
 *
 * There is still some ongoing debate about this this use case however:
 *   https://github.com/python/cpython/issues/116946
 *
 * Note:
 * If we allow subclassing in the future, reconsider use of PyObject_Free vs
 * using the function defined in the tp_free slot.
 *
 * See also:
 *   https://github.com/python/cpython/pull/138329#issuecomment-3242079564
 *   https://github.com/python/cpython/issues/116946#issuecomment-3242135537
 *   https://github.com/python/cpython/pull/139073
 *   https://github.com/python/cpython/commit/ec689187957cc80af56b9a63251bbc295bafd781
*/
static PyType_Spec chip_type_spec = {
	.name = "gpiosim.Chip",
	.basicsize = sizeof(chip_object),
	.flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE),
	.slots = chip_type_slots,
};

static int module_exec(PyObject *module)
{
	const struct module_const *modconst;
	struct module_state *state;
	PyObject *chip_type_obj;
	int ret;

	state = PyModule_GetState(module);

	state->sim_ctx = gpiosim_ctx_new();
	if (!state->sim_ctx) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}

	chip_type_obj = PyType_FromModuleAndSpec(module, &chip_type_spec, NULL);

	if (!chip_type_obj)
		return -1;

	ret = PyModule_AddType(module, (PyTypeObject*)chip_type_obj);
	Py_DECREF(chip_type_obj);
	if (ret < 0)
		return -1;

	for (modconst = module_constants; modconst->name; modconst++) {
		ret = PyModule_AddIntConstant(module, modconst->name,
					      modconst->val);
		if (ret < 0)
			return -1;
	}

	return 0;
}

static void free_module_state(void *mod)
{
	struct module_state *state = PyModule_GetState((PyObject *)mod);

	if (state->sim_ctx) {
		gpiosim_ctx_unref(state->sim_ctx);
		state->sim_ctx = NULL;
	}
}

static struct PyModuleDef_Slot module_slots[] = {
	{ Py_mod_exec, module_exec },
	{ 0, NULL },
};

static PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	.m_name = "gpiosim._ext",
	.m_size = sizeof(struct module_state),
	.m_free = free_module_state,
	.m_slots = module_slots,
};

PyMODINIT_FUNC PyInit__ext(void)
{
	return PyModuleDef_Init(&module_def);
}
