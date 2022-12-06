// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <Python.h>
#include <sys/prctl.h>

static PyObject *
module_set_process_name(PyObject *Py_UNUSED(self), PyObject *args)
{
	const char *name;
	int ret;

	ret = PyArg_ParseTuple(args, "s", &name);
	if (!ret)
		return NULL;

	ret = prctl(PR_SET_NAME, name);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

static PyMethodDef module_methods[] = {
	{
		.ml_name = "set_process_name",
		.ml_meth = (PyCFunction)module_set_process_name,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

static PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	.m_name = "procname._ext",
	.m_methods = module_methods,
};

PyMODINIT_FUNC PyInit__ext(void)
{
	return PyModule_Create(&module_def);
}
