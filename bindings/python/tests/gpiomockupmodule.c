// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <Python.h>
#include <gpio-mockup.h>

typedef struct {
	PyObject_HEAD
	struct gpio_mockup *mockup;
} gpiomockup_MockupObject;

enum {
	gpiomockup_FLAG_NAMED_LINES = 1,
};

static int gpiomockup_Mockup_init(gpiomockup_MockupObject *self,
				  PyObject *Py_UNUSED(ignored0),
				  PyObject *Py_UNUSED(ignored1))
{
	Py_BEGIN_ALLOW_THREADS;
	self->mockup = gpio_mockup_new();
	Py_END_ALLOW_THREADS;
	if (!self->mockup) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}

	return 0;
}

static void gpiomockup_Mockup_dealloc(gpiomockup_MockupObject *self)
{
	if (self->mockup) {
		Py_BEGIN_ALLOW_THREADS;
		gpio_mockup_unref(self->mockup);
		Py_END_ALLOW_THREADS;
	}

	PyObject_Del(self);
}

static PyObject *gpiomockup_Mockup_probe(gpiomockup_MockupObject *self,
					 PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "chip_sizes",
				  "flags",
				  NULL };

	PyObject *chip_sizes_obj, *iter, *next;
	unsigned int *chip_sizes;
	int ret, flags = 0, i;
	Py_ssize_t num_chips;

	ret = PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist,
					  &chip_sizes_obj, &flags);
	if (!ret)
		return NULL;

	num_chips = PyObject_Size(chip_sizes_obj);
	if (num_chips < 0) {
		return NULL;
	} else if (num_chips == 0) {
		PyErr_SetString(PyExc_TypeError,
				"Number of chips must be greater thatn 0");
		return NULL;
	}

	chip_sizes = PyMem_RawCalloc(num_chips, sizeof(unsigned int));
	if (!chip_sizes)
		return NULL;

	iter = PyObject_GetIter(chip_sizes_obj);
	if (!iter) {
		PyMem_RawFree(chip_sizes);
		return NULL;
	}

	for (i = 0;; i++) {
		next = PyIter_Next(iter);
		if (!next) {
			Py_DECREF(iter);
			break;
		}

		chip_sizes[i] = PyLong_AsUnsignedLong(next);
		Py_DECREF(next);
		if (PyErr_Occurred()) {
			Py_DECREF(iter);
			PyMem_RawFree(chip_sizes);
			return NULL;
		}
	}

	if (flags & gpiomockup_FLAG_NAMED_LINES)
		flags |= GPIO_MOCKUP_FLAG_NAMED_LINES;

	Py_BEGIN_ALLOW_THREADS;
	ret = gpio_mockup_probe(self->mockup, num_chips, chip_sizes, flags);
	Py_END_ALLOW_THREADS;
	PyMem_RawFree(chip_sizes);
	if (ret)
		return NULL;

	Py_RETURN_NONE;
}

static PyObject *gpiomockup_Mockup_remove(gpiomockup_MockupObject *self,
					  PyObject *Py_UNUSED(ignored))
{
	int ret;

	Py_BEGIN_ALLOW_THREADS;
	ret = gpio_mockup_remove(self->mockup);
	Py_END_ALLOW_THREADS;
	if (ret) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	Py_RETURN_NONE;
}

static PyObject *gpiomockup_Mockup_chip_name(gpiomockup_MockupObject *self,
					     PyObject *args)
{
	unsigned int idx;
	const char *name;
	int ret;

	ret = PyArg_ParseTuple(args, "I", &idx);
	if (!ret)
		return NULL;

	name = gpio_mockup_chip_name(self->mockup, idx);
	if (!name) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return PyUnicode_FromString(name);
}

static PyObject *gpiomockup_Mockup_chip_path(gpiomockup_MockupObject *self,
					     PyObject *args)
{
	unsigned int idx;
	const char *path;
	int ret;

	ret = PyArg_ParseTuple(args, "I", &idx);
	if (!ret)
		return NULL;

	path = gpio_mockup_chip_path(self->mockup, idx);
	if (!path) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return PyUnicode_FromString(path);
}

static PyObject *gpiomockup_Mockup_chip_num(gpiomockup_MockupObject *self,
					     PyObject *args)
{
	unsigned int idx;
	int ret, num;

	ret = PyArg_ParseTuple(args, "I", &idx);
	if (!ret)
		return NULL;

	num = gpio_mockup_chip_num(self->mockup, idx);
	if (num < 0) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return PyLong_FromLong(num);
}

static PyObject *gpiomockup_Mockup_chip_get_value(gpiomockup_MockupObject *self,
						  PyObject *args)
{
	unsigned int chip_idx, line_offset;
	int ret, val;

	ret = PyArg_ParseTuple(args, "II", &chip_idx, &line_offset);
	if (!ret)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	val = gpio_mockup_get_value(self->mockup, chip_idx, line_offset);
	Py_END_ALLOW_THREADS;
	if (val < 0) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return PyLong_FromUnsignedLong(val);
}

static PyObject *gpiomockup_Mockup_chip_set_pull(gpiomockup_MockupObject *self,
						 PyObject *args)
{
	unsigned int chip_idx, line_offset;
	int ret, pull;

	ret = PyArg_ParseTuple(args, "IIi", &chip_idx, &line_offset, &pull);
	if (!ret)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	ret = gpio_mockup_set_pull(self->mockup, chip_idx, line_offset, pull);
	Py_END_ALLOW_THREADS;
	if (ret) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	Py_RETURN_NONE;
}

static PyMethodDef gpiomockup_Mockup_methods[] = {
	{
		.ml_name = "probe",
		.ml_meth = (PyCFunction)(void (*)(void))gpiomockup_Mockup_probe,
		.ml_flags = METH_VARARGS | METH_KEYWORDS,
	},
	{
		.ml_name = "remove",
		.ml_meth = (PyCFunction)gpiomockup_Mockup_remove,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "chip_name",
		.ml_meth = (PyCFunction)gpiomockup_Mockup_chip_name,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "chip_path",
		.ml_meth = (PyCFunction)gpiomockup_Mockup_chip_path,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "chip_num",
		.ml_meth = (PyCFunction)gpiomockup_Mockup_chip_num,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "chip_get_value",
		.ml_meth = (PyCFunction)gpiomockup_Mockup_chip_get_value,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "chip_set_pull",
		.ml_meth = (PyCFunction)gpiomockup_Mockup_chip_set_pull,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

static PyTypeObject gpiomockup_MockupType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiomockup.Mockup",
	.tp_basicsize = sizeof(gpiomockup_MockupObject),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)gpiomockup_Mockup_init,
	.tp_dealloc = (destructor)gpiomockup_Mockup_dealloc,
	.tp_methods = gpiomockup_Mockup_methods,
};

static PyModuleDef gpiomockup_Module = {
	PyModuleDef_HEAD_INIT,
	.m_name = "gpiomockup",
	.m_size = -1,
};

PyMODINIT_FUNC PyInit_gpiomockup(void)
{
	PyObject *module, *val;
	int ret;

	module = PyModule_Create(&gpiomockup_Module);
	if (!module)
		return NULL;

	ret = PyType_Ready(&gpiomockup_MockupType);
	if (ret)
		return NULL;
	Py_INCREF(&gpiomockup_MockupType);

	ret = PyModule_AddObject(module, "Mockup",
				 (PyObject *)&gpiomockup_MockupType);
	if (ret)
		return NULL;

	val = PyLong_FromLong(gpiomockup_FLAG_NAMED_LINES);
	if (!val)
		return NULL;

	ret = PyDict_SetItemString(gpiomockup_MockupType.tp_dict,
				   "FLAG_NAMED_LINES", val);
	if (ret)
		return NULL;

	return module;
}
