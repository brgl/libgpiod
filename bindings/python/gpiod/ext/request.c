// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.h"

typedef struct {
	PyObject_HEAD;
	struct gpiod_line_request *request;
	unsigned int *offsets;
	enum gpiod_line_value *values;
	size_t num_lines;
	struct gpiod_edge_event_buffer *buffer;
} request_object;

static int request_init(PyObject *Py_UNUSED(ignored0),
			PyObject *Py_UNUSED(ignored1),
			PyObject *Py_UNUSED(ignored2))
{
	PyErr_SetString(PyExc_NotImplementedError,
			"_ext.LineRequest cannot be instantiated");

	return -1;
}

static void request_finalize(request_object *self)
{
	if (self->request)
		PyObject_CallMethod((PyObject *)self, "release", "");

	if (self->offsets)
		PyMem_Free(self->offsets);

	if (self->values)
		PyMem_Free(self->values);

	if (self->buffer)
		gpiod_edge_event_buffer_free(self->buffer);
}

static PyObject *
request_chip_name(request_object *self, void *Py_UNUSED(ignored))
{
	return PyUnicode_FromString(
			gpiod_line_request_get_chip_name(self->request));
}

static PyObject *
request_num_lines(request_object *self, void *Py_UNUSED(ignored))
{
	return PyLong_FromUnsignedLong(
			gpiod_line_request_get_num_requested_lines(self->request));
}

static PyObject *request_offsets(request_object *self, void *Py_UNUSED(ignored))
{
	PyObject *lines, *line;
	unsigned int *offsets;
	size_t num_lines, i;
	int ret;

	num_lines = gpiod_line_request_get_num_requested_lines(self->request);

	offsets = PyMem_Calloc(num_lines, sizeof(unsigned int));
	if (!offsets)
		return PyErr_NoMemory();

	gpiod_line_request_get_requested_offsets(self->request, offsets, num_lines);

	lines = PyList_New(num_lines);
	if (!lines) {
		PyMem_Free(offsets);
		return NULL;
	}

	for (i = 0; i < num_lines; i++) {
		line = PyLong_FromUnsignedLong(offsets[i]);
		if (!line) {
			Py_DECREF(lines);
			PyMem_Free(offsets);
			return NULL;
		}

		ret = PyList_SetItem(lines, i, line);
		if (ret) {
			Py_DECREF(line);
			Py_DECREF(lines);
			PyMem_Free(offsets);
			return NULL;
		}
	}

	PyMem_Free(offsets);
	return lines;
}

static PyObject *request_fd(request_object *self, void *Py_UNUSED(ignored))
{
	return PyLong_FromLong(gpiod_line_request_get_fd(self->request));
}

static PyGetSetDef request_getset[] = {
	{
		.name = "chip_name",
		.get = (getter)request_chip_name,
	},
	{
		.name = "num_lines",
		.get = (getter)request_num_lines,
	},
	{
		.name = "offsets",
		.get = (getter)request_offsets,
	},
	{
		.name = "fd",
		.get = (getter)request_fd,
	},
	{ }
};

static PyObject *
request_release(request_object *self, PyObject *Py_UNUSED(ignored))
{
	Py_BEGIN_ALLOW_THREADS;
	gpiod_line_request_release(self->request);
	Py_END_ALLOW_THREADS;
	self->request = NULL;

	Py_RETURN_NONE;
}

static void clear_buffers(request_object *self)
{
	memset(self->offsets, 0, self->num_lines * sizeof(unsigned int));
	memset(self->values, 0, self->num_lines * sizeof(int));
}

static PyObject *request_get_values(request_object *self, PyObject *args)
{
	PyObject *offsets, *values, *val, *type, *iter, *next;
	Py_ssize_t num_offsets, pos;
	int ret;

	ret = PyArg_ParseTuple(args, "OO", &offsets, &values);
	if (!ret)
		return NULL;

	num_offsets = PyObject_Size(offsets);
	if (num_offsets < 0)
		return NULL;

	type = Py_gpiod_GetGlobalType("Value");
	if (!type)
		return NULL;

	iter = PyObject_GetIter(offsets);
	if (!iter)
		return NULL;

	clear_buffers(self);

	for (pos = 0;; pos++) {
		next = PyIter_Next(iter);
		if (!next) {
			Py_DECREF(iter);
			break;
		}

		self->offsets[pos] = Py_gpiod_PyLongAsUnsignedInt(next);
		Py_DECREF(next);
		if (PyErr_Occurred()) {
			Py_DECREF(iter);
			return NULL;
		}
	}

	Py_BEGIN_ALLOW_THREADS;
	ret = gpiod_line_request_get_values_subset(self->request,
						   num_offsets,
						   self->offsets,
						   self->values);
	Py_END_ALLOW_THREADS;
	if (ret)
		return Py_gpiod_SetErrFromErrno();

	for (pos = 0; pos < num_offsets; pos++) {
		val = PyObject_CallFunction(type, "i", self->values[pos]);
		if (!val)
			return NULL;

		ret = PyList_SetItem(values, pos, val);
		if (ret) {
			Py_DECREF(val);
			return NULL;
		}
	}

	Py_RETURN_NONE;
}

static PyObject *request_set_values(request_object *self, PyObject *args)
{
	PyObject *values, *key, *val, *val_stripped;
	Py_ssize_t pos = 0;
	int ret;

	ret = PyArg_ParseTuple(args, "O", &values);
	if (!ret)
		return NULL;

	clear_buffers(self);

	while (PyDict_Next(values, &pos, &key, &val)) {
		self->offsets[pos - 1] = Py_gpiod_PyLongAsUnsignedInt(key);
		if (PyErr_Occurred())
			return NULL;

		val_stripped = PyObject_GetAttrString(val, "value");
		if (!val_stripped)
			return NULL;

		self->values[pos - 1] = PyLong_AsLong(val_stripped);
		Py_DECREF(val_stripped);
		if (PyErr_Occurred())
			return NULL;
	}

	Py_BEGIN_ALLOW_THREADS;
	ret = gpiod_line_request_set_values_subset(self->request,
						   pos,
						   self->offsets,
						   self->values);
	Py_END_ALLOW_THREADS;
	if (ret)
		return Py_gpiod_SetErrFromErrno();

	Py_RETURN_NONE;
}

static PyObject *request_reconfigure_lines(request_object *self, PyObject *args)
{
	struct gpiod_line_config *line_cfg;
	PyObject *line_cfg_obj;
	int ret;

	ret = PyArg_ParseTuple(args, "O", &line_cfg_obj);
	if (!ret)
		return NULL;

	line_cfg = Py_gpiod_LineConfigGetData(line_cfg_obj);
	if (!line_cfg)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	ret = gpiod_line_request_reconfigure_lines(self->request, line_cfg);
	Py_END_ALLOW_THREADS;
	if (ret)
		return Py_gpiod_SetErrFromErrno();

	Py_RETURN_NONE;
}

static PyObject *request_read_edge_events(request_object *self, PyObject *args)
{
	PyObject *max_events_obj, *event_obj, *events, *type;
	size_t max_events, num_events, i;
	struct gpiod_edge_event *event;
	int ret;

	ret = PyArg_ParseTuple(args, "O", &max_events_obj);
	if (!ret)
		return NULL;

	if (max_events_obj != Py_None) {
		max_events = PyLong_AsSize_t(max_events_obj);
		if (PyErr_Occurred())
			return NULL;
	} else {
		max_events = 64;
	}

	type = Py_gpiod_GetGlobalType("EdgeEvent");
	if (!type)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	ret = gpiod_line_request_read_edge_events(self->request,
						 self->buffer, max_events);
	Py_END_ALLOW_THREADS;
	if (ret < 0)
		return Py_gpiod_SetErrFromErrno();

	num_events = ret;

	events = PyList_New(num_events);
	if (!events)
		return NULL;

	for (i = 0; i < num_events; i++) {
		event = gpiod_edge_event_buffer_get_event(self->buffer, i);
		if (!event) {
			Py_DECREF(events);
			return NULL;
		}

		event_obj = PyObject_CallFunction(type, "iKiii",
				gpiod_edge_event_get_event_type(event),
				gpiod_edge_event_get_timestamp_ns(event),
				gpiod_edge_event_get_line_offset(event),
				gpiod_edge_event_get_global_seqno(event),
				gpiod_edge_event_get_line_seqno(event));
		if (!event_obj) {
			Py_DECREF(events);
			return NULL;
		}

		ret = PyList_SetItem(events, i, event_obj);
		if (ret) {
			Py_DECREF(event_obj);
			Py_DECREF(events);
			return NULL;
		}
	}

	return events;
}

static PyMethodDef request_methods[] = {
	{
		.ml_name = "release",
		.ml_meth = (PyCFunction)request_release,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "get_values",
		.ml_meth = (PyCFunction)request_get_values,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_values",
		.ml_meth = (PyCFunction)request_set_values,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "reconfigure_lines",
		.ml_meth = (PyCFunction)request_reconfigure_lines,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "read_edge_events",
		.ml_meth = (PyCFunction)request_read_edge_events,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

PyTypeObject request_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod._ext.Request",
	.tp_basicsize = sizeof(request_object),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)request_init,
	.tp_finalize = (destructor)request_finalize,
	.tp_dealloc = (destructor)Py_gpiod_dealloc,
	.tp_getset = request_getset,
	.tp_methods = request_methods,
};

PyObject *Py_gpiod_MakeRequestObject(struct gpiod_line_request *request,
				     size_t event_buffer_size)
{
	struct gpiod_edge_event_buffer *buffer;
	enum gpiod_line_value *values;
	request_object *req_obj;
	unsigned int *offsets;
	size_t num_lines;

	num_lines = gpiod_line_request_get_num_requested_lines(request);

	req_obj = PyObject_New(request_object, &request_type);
	if (!req_obj)
		return NULL;

	offsets = PyMem_Calloc(num_lines, sizeof(unsigned int));
	if (!offsets) {
		Py_DECREF(req_obj);
		return NULL;
	}

	values = PyMem_Calloc(num_lines, sizeof(int));
	if (!values) {
		PyMem_Free(offsets);
		Py_DECREF(req_obj);
		return NULL;
	}

	buffer = gpiod_edge_event_buffer_new(event_buffer_size);
	if (!buffer) {
		PyMem_Free(values);
		PyMem_Free(offsets);
		Py_DECREF(req_obj);
		return Py_gpiod_SetErrFromErrno();
	}

	req_obj->request = request;
	req_obj->offsets = offsets;
	req_obj->values = values;
	req_obj->num_lines = num_lines;
	req_obj->buffer = buffer;

	return (PyObject *)req_obj;
}
