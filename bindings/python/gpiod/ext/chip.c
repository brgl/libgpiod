// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include "internal.h"

typedef struct {
	PyObject_HEAD;
	struct gpiod_chip *chip;
} chip_object;

static int
chip_init(chip_object *self, PyObject *args, PyObject *Py_UNUSED(ignored))
{
	struct gpiod_chip *chip;
	char *path;
	int ret;

	ret = PyArg_ParseTuple(args, "s", &path);
	if (!ret)
		return -1;

	Py_BEGIN_ALLOW_THREADS;
	chip = gpiod_chip_open(path);
	Py_END_ALLOW_THREADS;
	if (!chip) {
		Py_gpiod_SetErrFromErrno();
		return -1;
	}

	self->chip = chip;

	return 0;
}

static void chip_finalize(chip_object *self)
{
	if (self->chip)
		PyObject_CallMethod((PyObject *)self, "close", "");
}

static PyObject *chip_path(chip_object *self, void *Py_UNUSED(ignored))
{
	return PyUnicode_FromString(gpiod_chip_get_path(self->chip));
}

static PyObject *chip_fd(chip_object *self, void *Py_UNUSED(ignored))
{
	return PyLong_FromLong(gpiod_chip_get_fd(self->chip));
}

static PyGetSetDef chip_getset[] = {
	{
		.name = "path",
		.get = (getter)chip_path,
	},
	{
		.name = "fd",
		.get = (getter)chip_fd,
	},
	{ }
};

static PyObject *chip_close(chip_object *self, PyObject *Py_UNUSED(ignored))
{
	Py_BEGIN_ALLOW_THREADS;
	gpiod_chip_close(self->chip);
	Py_END_ALLOW_THREADS;
	self->chip = NULL;

	Py_RETURN_NONE;
}

static PyObject *chip_get_info(chip_object *self, PyObject *Py_UNUSED(ignored))
{
	struct gpiod_chip_info *info;
	PyObject *type, *ret;

	type = Py_gpiod_GetGlobalType("ChipInfo");
	if (!type)
		return NULL;

	info = gpiod_chip_get_info(self->chip);
	if (!info)
		return PyErr_SetFromErrno(PyExc_OSError);

	 ret = PyObject_CallFunction(type, "ssI",
				     gpiod_chip_info_get_name(info),
				     gpiod_chip_info_get_label(info),
				     gpiod_chip_info_get_num_lines(info));
	 gpiod_chip_info_free(info);
	 return ret;
}

static PyObject *make_line_info(struct gpiod_line_info *info)
{
	PyObject *type;

	type = Py_gpiod_GetGlobalType("LineInfo");
	if (!type)
		return NULL;

	return PyObject_CallFunction(type, "IsOsiOiiiiOk",
				gpiod_line_info_get_offset(info),
				gpiod_line_info_get_name(info),
				gpiod_line_info_is_used(info) ?
							Py_True : Py_False,
				gpiod_line_info_get_consumer(info),
				gpiod_line_info_get_direction(info),
				gpiod_line_info_is_active_low(info) ?
							Py_True : Py_False,
				gpiod_line_info_get_bias(info),
				gpiod_line_info_get_drive(info),
				gpiod_line_info_get_edge_detection(info),
				gpiod_line_info_get_event_clock(info),
				gpiod_line_info_is_debounced(info) ?
							Py_True : Py_False,
				gpiod_line_info_get_debounce_period_us(info));
}

static PyObject *chip_get_line_info(chip_object *self, PyObject *args)
{
	struct gpiod_line_info *info;
	unsigned int offset;
	PyObject *info_obj;
	int ret, watch;

	ret = PyArg_ParseTuple(args, "Ip", &offset, &watch);
	if (!ret)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	if (watch)
		info = gpiod_chip_watch_line_info(self->chip, offset);
	else
		info = gpiod_chip_get_line_info(self->chip, offset);
	Py_END_ALLOW_THREADS;
	if (!info)
		return Py_gpiod_SetErrFromErrno();

	info_obj = make_line_info(info);
	gpiod_line_info_free(info);
	return info_obj;
}

static PyObject *
chip_unwatch_line_info(chip_object *self, PyObject *args)
{
	unsigned int offset;
	int ret;

	ret = PyArg_ParseTuple(args, "I", &offset);
	if (!ret)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	ret = gpiod_chip_unwatch_line_info(self->chip, offset);
	Py_END_ALLOW_THREADS;
	if (ret)
		return Py_gpiod_SetErrFromErrno();

	Py_RETURN_NONE;
}

static PyObject *
chip_read_info_event(chip_object *self, PyObject *Py_UNUSED(ignored))
{
	PyObject *type, *info_obj, *event_obj;
	struct gpiod_info_event *event;
	struct gpiod_line_info *info;

	type = Py_gpiod_GetGlobalType("InfoEvent");
	if (!type)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	event = gpiod_chip_read_info_event(self->chip);
	Py_END_ALLOW_THREADS;
	if (!event)
		return Py_gpiod_SetErrFromErrno();

	info = gpiod_info_event_get_line_info(event);

	info_obj = make_line_info(info);
	if (!info_obj) {
		gpiod_info_event_free(event);
		return NULL;
	}

	event_obj = PyObject_CallFunction(type, "iKO",
				gpiod_info_event_get_event_type(event),
				gpiod_info_event_get_timestamp_ns(event),
				info_obj);
	Py_DECREF(info_obj);
	gpiod_info_event_free(event);
	return event_obj;
}

static PyObject *chip_line_offset_from_id(chip_object *self, PyObject *args)
{
	int ret, offset;
	char *name;

	ret = PyArg_ParseTuple(args, "s", &name);
	if (!ret)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	offset = gpiod_chip_get_line_offset_from_name(self->chip, name);
	Py_END_ALLOW_THREADS;
	if (offset < 0)
		return Py_gpiod_SetErrFromErrno();

	return PyLong_FromLong(offset);
}

static struct gpiod_request_config *
make_request_config(PyObject *consumer_obj, PyObject *event_buffer_size_obj)
{
	struct gpiod_request_config *req_cfg;
	size_t event_buffer_size;
	const char *consumer;

	req_cfg = gpiod_request_config_new();
	if (!req_cfg) {
		Py_gpiod_SetErrFromErrno();
		return NULL;
	}

	if (consumer_obj != Py_None) {
		consumer = PyUnicode_AsUTF8(consumer_obj);
		if (!consumer) {
			gpiod_request_config_free(req_cfg);
			return NULL;
		}

		gpiod_request_config_set_consumer(req_cfg, consumer);
	}

	if (event_buffer_size_obj != Py_None) {
		event_buffer_size = PyLong_AsSize_t(event_buffer_size_obj);
		if (PyErr_Occurred()) {
			gpiod_request_config_free(req_cfg);
			return NULL;
		}

		gpiod_request_config_set_event_buffer_size(req_cfg,
							   event_buffer_size);
	}

	return req_cfg;
}

static PyObject *chip_request_lines(chip_object *self, PyObject *args)
{
	PyObject *line_config, *consumer, *event_buffer_size, *req_obj;
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_config *line_cfg;
	struct gpiod_line_request *request;
	int ret;

	ret = PyArg_ParseTuple(args, "OOO",
			       &line_config, &consumer, &event_buffer_size);
	if (!ret)
		return NULL;

	line_cfg = Py_gpiod_LineConfigGetData(line_config);
	if (!line_cfg)
		return NULL;

	req_cfg = make_request_config(consumer, event_buffer_size);
	if (!req_cfg)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	request = gpiod_chip_request_lines(self->chip, req_cfg, line_cfg);
	Py_END_ALLOW_THREADS;
	if (!request) {
		gpiod_request_config_free(req_cfg);
		return Py_gpiod_SetErrFromErrno();
	}

	req_obj = Py_gpiod_MakeRequestObject(request,
			gpiod_request_config_get_event_buffer_size(req_cfg));
	if (!req_obj)
		gpiod_line_request_release(request);
	gpiod_request_config_free(req_cfg);

	return req_obj;
}

static PyMethodDef chip_methods[] = {
	{
		.ml_name = "close",
		.ml_meth = (PyCFunction)chip_close,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "get_info",
		.ml_meth = (PyCFunction)chip_get_info,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "get_line_info",
		.ml_meth = (PyCFunction)chip_get_line_info,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "unwatch_line_info",
		.ml_meth = (PyCFunction)chip_unwatch_line_info,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "read_info_event",
		.ml_meth = (PyCFunction)chip_read_info_event,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "line_offset_from_id",
		.ml_meth = (PyCFunction)chip_line_offset_from_id,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "request_lines",
		.ml_meth = (PyCFunction)chip_request_lines,
		.ml_flags = METH_VARARGS,
	},
	{ }
};

PyTypeObject chip_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod._ext.Chip",
	.tp_basicsize = sizeof(chip_object),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)chip_init,
	.tp_finalize = (destructor)chip_finalize,
	.tp_dealloc = (destructor)Py_gpiod_dealloc,
	.tp_getset = chip_getset,
	.tp_methods = chip_methods,
};
