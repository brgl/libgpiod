// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>
// SPDX-FileCopyrightText: 2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <Python.h>
#include <linux/version.h>
#include <sys/prctl.h>
#include <sys/utsname.h>

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

static PyObject *
module_check_kernel_version(PyObject *Py_UNUSED(self), PyObject *args)
{
	unsigned int req_maj, req_min, req_rel, curr_maj, curr_min, curr_rel;
	struct utsname un;
	int ret;

	ret = PyArg_ParseTuple(args, "III", &req_maj, &req_min, &req_rel);
	if (!ret)
		return NULL;

	ret = uname(&un);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	ret = sscanf(un.release, "%u.%u.%u", &curr_maj, &curr_min, &curr_rel);
	if (ret != 3) {
		PyErr_SetString(PyExc_RuntimeError,
				"invalid linux version read from the kernel");
		return NULL;
	}

	if (KERNEL_VERSION(curr_maj, curr_min, curr_rel) <
	    KERNEL_VERSION(req_maj, req_min, req_rel))
		Py_RETURN_FALSE;

	Py_RETURN_TRUE;
}

static PyMethodDef module_methods[] = {
	{
		.ml_name = "set_process_name",
		.ml_meth = (PyCFunction)module_set_process_name,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "check_kernel_version",
		.ml_meth = (PyCFunction)module_check_kernel_version,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

static PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	.m_name = "system._ext",
	.m_methods = module_methods,
};

PyMODINIT_FUNC PyInit__ext(void)
{
	return PyModule_Create(&module_def);
}
