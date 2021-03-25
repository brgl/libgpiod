// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.h"

typedef struct {
	PyObject_HEAD;
	struct gpiod_line_settings *settings;
} line_settings_object;

static int set_int_prop(struct gpiod_line_settings *settings, int val,
			int (*func)(struct gpiod_line_settings *, int))
{
	int ret;

	ret = func(settings, val);
	if (ret) {
		Py_gpiod_SetErrFromErrno();
		return -1;
	}

	return 0;
}

static int
line_settings_init(line_settings_object *self, PyObject *args, PyObject *kwargs)
{
	static char *kwlist[] = {
		"direction",
		"edge_detection",
		"bias",
		"drive",
		"active_low",
		"debounce_period",
		"event_clock",
		"output_value",
		NULL
	};

	int direction, edge, bias, drive, active_low, event_clock, output_value,
	    ret;
	unsigned long debounce_period;

	ret = PyArg_ParseTupleAndKeywords(args, kwargs, "IIIIpkII", kwlist,
			&direction, &edge, &bias, &drive, &active_low,
			&debounce_period, &event_clock, &output_value);
	if (!ret)
		return -1;

	self->settings = gpiod_line_settings_new();
	if (!self->settings) {
		Py_gpiod_SetErrFromErrno();
		return -1;
	}

	ret = set_int_prop(self->settings, direction,
			   gpiod_line_settings_set_direction);
	if (ret)
		return -1;

	ret = set_int_prop(self->settings, edge,
			   gpiod_line_settings_set_edge_detection);
	if (ret)
		return -1;

	ret = set_int_prop(self->settings, bias,
			   gpiod_line_settings_set_bias);
	if (ret)
		return -1;

	ret = set_int_prop(self->settings, drive,
			   gpiod_line_settings_set_drive);
	if (ret)
		return -1;

	gpiod_line_settings_set_active_low(self->settings, active_low);
	gpiod_line_settings_set_debounce_period_us(self->settings,
						   debounce_period);

	ret = set_int_prop(self->settings, edge,
			   gpiod_line_settings_set_edge_detection);
	if (ret)
		return -1;

	ret = set_int_prop(self->settings, output_value,
			   gpiod_line_settings_set_output_value);
	if (ret)
		return -1;

	return 0;
}

static void line_settings_finalize(line_settings_object *self)
{
	if (self->settings)
		gpiod_line_settings_free(self->settings);
}

PyTypeObject line_settings_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod._ext.LineSettings",
	.tp_basicsize = sizeof(line_settings_object),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)line_settings_init,
	.tp_finalize = (destructor)line_settings_finalize,
	.tp_dealloc = (destructor)Py_gpiod_dealloc,
};

struct gpiod_line_settings *Py_gpiod_LineSettingsGetData(PyObject *obj)
{
	line_settings_object *settings;
	PyObject *type;

	type = PyObject_Type(obj);
	if (!type)
		return NULL;

	if ((PyTypeObject *)type != &line_settings_type) {
		PyErr_SetString(PyExc_TypeError,
				"not a gpiod._ext.LineSettings object");
		Py_DECREF(type);
		return NULL;
	}
	Py_DECREF(type);

	settings = (line_settings_object *)obj;

	return settings->settings;
}
