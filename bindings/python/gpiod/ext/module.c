// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <gpiod.h>
#include <Python.h>

struct module_const {
	const char *name;
	long val;
};

static const struct module_const module_constants[] = {
	{
		.name = "VALUE_INACTIVE",
		.val = GPIOD_LINE_VALUE_INACTIVE,
	},
	{
		.name = "VALUE_ACTIVE",
		.val = GPIOD_LINE_VALUE_ACTIVE,
	},
	{
		.name = "DIRECTION_AS_IS",
		.val = GPIOD_LINE_DIRECTION_AS_IS,
	},
	{
		.name = "DIRECTION_INPUT",
		.val = GPIOD_LINE_DIRECTION_INPUT,
	},
	{
		.name = "DIRECTION_OUTPUT",
		.val = GPIOD_LINE_DIRECTION_OUTPUT,
	},
	{
		.name = "BIAS_AS_IS",
		.val = GPIOD_LINE_BIAS_AS_IS,
	},
	{
		.name = "BIAS_UNKNOWN",
		.val = GPIOD_LINE_BIAS_UNKNOWN,
	},
	{
		.name = "BIAS_DISABLED",
		.val = GPIOD_LINE_BIAS_DISABLED,
	},
	{
		.name = "BIAS_PULL_UP",
		.val = GPIOD_LINE_BIAS_PULL_UP,
	},
	{
		.name = "BIAS_PULL_DOWN",
		.val = GPIOD_LINE_BIAS_PULL_DOWN,
	},
	{
		.name = "DRIVE_PUSH_PULL",
		.val = GPIOD_LINE_DRIVE_PUSH_PULL,
	},
	{
		.name = "DRIVE_OPEN_DRAIN",
		.val = GPIOD_LINE_DRIVE_OPEN_DRAIN,
	},
	{
		.name = "DRIVE_OPEN_SOURCE",
		.val = GPIOD_LINE_DRIVE_OPEN_SOURCE,
	},
	{
		.name = "EDGE_NONE",
		.val = GPIOD_LINE_EDGE_NONE,
	},
	{
		.name = "EDGE_FALLING",
		.val = GPIOD_LINE_EDGE_FALLING,
	},
	{
		.name = "EDGE_RISING",
		.val = GPIOD_LINE_EDGE_RISING,
	},
	{
		.name = "EDGE_BOTH",
		.val = GPIOD_LINE_EDGE_BOTH,
	},
	{
		.name = "CLOCK_MONOTONIC",
		.val = GPIOD_LINE_CLOCK_MONOTONIC,
	},
	{
		.name = "CLOCK_REALTIME",
		.val = GPIOD_LINE_CLOCK_REALTIME,
	},
	{
		.name = "CLOCK_HTE",
		.val = GPIOD_LINE_CLOCK_HTE,
	},
	{
		.name = "EDGE_EVENT_TYPE_RISING",
		.val = GPIOD_EDGE_EVENT_RISING_EDGE,
	},
	{
		.name = "EDGE_EVENT_TYPE_FALLING",
		.val = GPIOD_EDGE_EVENT_FALLING_EDGE,
	},
	{
		.name = "INFO_EVENT_TYPE_LINE_REQUESTED",
		.val = GPIOD_INFO_EVENT_LINE_REQUESTED,
	},
	{
		.name = "INFO_EVENT_TYPE_LINE_RELEASED",
		.val = GPIOD_INFO_EVENT_LINE_RELEASED,
	},
	{
		.name = "INFO_EVENT_TYPE_LINE_CONFIG_CHANGED",
		.val = GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED,
	},
	{ }
};

static PyObject *
module_is_gpiochip_device(PyObject *Py_UNUSED(self), PyObject *args)
{
	const char *path;
	int ret;

	ret =  PyArg_ParseTuple(args, "s", &path);
	if (!ret)
		return NULL;

	return PyBool_FromLong(gpiod_is_gpiochip_device(path));
}

static PyMethodDef module_methods[] = {
	{
		.ml_name = "is_gpiochip_device",
		.ml_meth = (PyCFunction)module_is_gpiochip_device,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

static PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT,
	.m_name = "gpiod._ext",
	.m_methods = module_methods,
};

extern PyTypeObject chip_type;
extern PyTypeObject line_config_type;
extern PyTypeObject line_settings_type;
extern PyTypeObject request_type;

static PyTypeObject *types[] = {
	&chip_type,
	&line_config_type,
	&line_settings_type,
	&request_type,
	NULL,
};

PyMODINIT_FUNC PyInit__ext(void)
{
	const struct module_const *modconst;
	PyTypeObject **type;
	PyObject *module;
	int ret;

	module = PyModule_Create(&module_def);
	if (!module)
		return NULL;

	ret = PyModule_AddStringConstant(module, "api_version",
					 gpiod_version_string());
	if (ret) {
		Py_DECREF(module);
		return NULL;
	}

	for (type = types; *type; type++) {
		ret = PyModule_AddType(module, *type);
		if (ret) {
			Py_DECREF(module);
			return NULL;
		}
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
