// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <Python.h>
#include <gpiod.h>

#define LINE_REQUEST_MAX_LINES 64

typedef struct {
	PyObject_HEAD
	struct gpiod_chip *chip;
} gpiod_ChipObject;

typedef struct {
	PyObject_HEAD
	struct gpiod_line *line;
	gpiod_ChipObject *owner;
} gpiod_LineObject;

typedef struct {
	PyObject_HEAD
	struct gpiod_line_event event;
	gpiod_LineObject *source;
} gpiod_LineEventObject;

typedef struct {
	PyObject_HEAD
	PyObject **lines;
	Py_ssize_t num_lines;
	Py_ssize_t iter_idx;
} gpiod_LineBulkObject;

typedef struct {
	PyObject_HEAD
	unsigned int offset;
	gpiod_ChipObject *owner;
} gpiod_LineIterObject;

static gpiod_LineBulkObject *gpiod_LineToLineBulk(gpiod_LineObject *line);
static gpiod_LineObject *gpiod_MakeLineObject(gpiod_ChipObject *owner,
					      struct gpiod_line *line);

enum {
	gpiod_LINE_REQ_DIR_AS_IS = 1,
	gpiod_LINE_REQ_DIR_IN,
	gpiod_LINE_REQ_DIR_OUT,
	gpiod_LINE_REQ_EV_FALLING_EDGE,
	gpiod_LINE_REQ_EV_RISING_EDGE,
	gpiod_LINE_REQ_EV_BOTH_EDGES,
};

enum {
	gpiod_LINE_REQ_FLAG_OPEN_DRAIN		= GPIOD_BIT(0),
	gpiod_LINE_REQ_FLAG_OPEN_SOURCE		= GPIOD_BIT(1),
	gpiod_LINE_REQ_FLAG_ACTIVE_LOW		= GPIOD_BIT(2),
	gpiod_LINE_REQ_FLAG_BIAS_DISABLE	= GPIOD_BIT(3),
	gpiod_LINE_REQ_FLAG_BIAS_PULL_DOWN	= GPIOD_BIT(4),
	gpiod_LINE_REQ_FLAG_BIAS_PULL_UP	= GPIOD_BIT(5),
};

enum {
	gpiod_DIRECTION_INPUT = 1,
	gpiod_DIRECTION_OUTPUT,
};

enum {
	gpiod_BIAS_UNKNOWN = 1,
	gpiod_BIAS_DISABLE,
	gpiod_BIAS_PULL_UP,
	gpiod_BIAS_PULL_DOWN,
};

enum {
	gpiod_RISING_EDGE = 1,
	gpiod_FALLING_EDGE,
};

static bool gpiod_ChipIsClosed(gpiod_ChipObject *chip)
{
	if (!chip->chip) {
		PyErr_SetString(PyExc_ValueError,
				"I/O operation on closed file");
		return true;
	}

	return false;
}

static PyObject *gpiod_CallMethodPyArgs(PyObject *obj, const char *method,
					PyObject *args, PyObject *kwds)
{
	PyObject *callable, *ret;

	callable = PyObject_GetAttrString((PyObject *)obj, method);
	if (!callable)
		return NULL;

	ret = PyObject_Call(callable, args, kwds);
	Py_DECREF(callable);

	return ret;
}

static int gpiod_LineEvent_init(PyObject *Py_UNUSED(ignored0),
				PyObject *Py_UNUSED(ignored1),
				PyObject *Py_UNUSED(ignored2))
{
	PyErr_SetString(PyExc_NotImplementedError,
			"Only gpiod.Line can create new LineEvent objects.");
	return -1;
}

static void gpiod_LineEvent_dealloc(gpiod_LineEventObject *self)
{
	if (self->source)
		Py_DECREF(self->source);

	PyObject_Del(self);
}

PyDoc_STRVAR(gpiod_LineEvent_get_type_doc,
"Event type of this line event (integer).");

PyObject *gpiod_LineEvent_get_type(gpiod_LineEventObject *self,
				   PyObject *Py_UNUSED(ignored))
{
	int rv;

	if (self->event.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
		rv = gpiod_RISING_EDGE;
	else
		rv = gpiod_FALLING_EDGE;

	return Py_BuildValue("I", rv);
}

PyDoc_STRVAR(gpiod_LineEvent_get_sec_doc,
"Seconds value of the line event timestamp (integer).");

PyObject *gpiod_LineEvent_get_sec(gpiod_LineEventObject *self,
				  PyObject *Py_UNUSED(ignored))
{
	return Py_BuildValue("I", self->event.ts.tv_sec);
}

PyDoc_STRVAR(gpiod_LineEvent_get_nsec_doc,
"Nanoseconds value of the line event timestamp (integer).");

PyObject *gpiod_LineEvent_get_nsec(gpiod_LineEventObject *self,
				   PyObject *Py_UNUSED(ignored))
{
	return Py_BuildValue("I", self->event.ts.tv_nsec);
}

PyDoc_STRVAR(gpiod_LineEvent_get_source_doc,
"Line object representing the GPIO line on which this event\n"
"occurred (gpiod.Line object).");

gpiod_LineObject *gpiod_LineEvent_get_source(gpiod_LineEventObject *self,
					     PyObject *Py_UNUSED(ignored))
{
	Py_INCREF(self->source);
	return self->source;
}

static PyGetSetDef gpiod_LineEvent_getset[] = {
	{
		.name = "type",
		.get = (getter)gpiod_LineEvent_get_type,
		.doc = gpiod_LineEvent_get_type_doc,
	},
	{
		.name = "sec",
		.get = (getter)gpiod_LineEvent_get_sec,
		.doc = gpiod_LineEvent_get_sec_doc,
	},
	{
		.name = "nsec",
		.get = (getter)gpiod_LineEvent_get_nsec,
		.doc = gpiod_LineEvent_get_nsec_doc,
	},
	{
		.name = "source",
		.get = (getter)gpiod_LineEvent_get_source,
		.doc = gpiod_LineEvent_get_source_doc,
	},
	{ }
};

static PyObject *gpiod_LineEvent_repr(gpiod_LineEventObject *self)
{
	PyObject *line_repr, *ret;
	const char *edge;

	if (self->event.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
		edge = "RISING EDGE";
	else
		edge = "FALLING EDGE";

	line_repr = PyObject_CallMethod((PyObject *)self->source,
					"__repr__", "");

	ret = PyUnicode_FromFormat("'%s (%ld.%ld) source(%S)'",
				   edge, self->event.ts.tv_sec,
				   self->event.ts.tv_nsec, line_repr);
	Py_DECREF(line_repr);

	return ret;
}

PyDoc_STRVAR(gpiod_LineEventType_doc,
"Represents a single GPIO line event. This object is immutable and can only\n"
"be created by an instance of gpiod.Line.");

static PyTypeObject gpiod_LineEventType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod.LineEvent",
	.tp_basicsize = sizeof(gpiod_LineEventObject),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = gpiod_LineEventType_doc,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)gpiod_LineEvent_init,
	.tp_dealloc = (destructor)gpiod_LineEvent_dealloc,
	.tp_getset = gpiod_LineEvent_getset,
	.tp_repr = (reprfunc)gpiod_LineEvent_repr,
};

static int gpiod_Line_init(PyObject *Py_UNUSED(ignored0),
			   PyObject *Py_UNUSED(ignored1),
			   PyObject *Py_UNUSED(ignored2))
{
	PyErr_SetString(PyExc_NotImplementedError,
			"Only gpiod.Chip can create new Line objects.");
	return -1;
}

static void gpiod_Line_dealloc(gpiod_LineObject *self)
{
	if (self->owner)
		Py_DECREF(self->owner);

	PyObject_Del(self);
}

PyDoc_STRVAR(gpiod_Line_owner_doc,
"owner() -> Chip object owning the line\n"
"\n"
"Get the GPIO chip owning this line.");

static PyObject *gpiod_Line_owner(gpiod_LineObject *self,
				  PyObject *Py_UNUSED(ignored))
{
	Py_INCREF(self->owner);
	return (PyObject *)self->owner;
}

PyDoc_STRVAR(gpiod_Line_offset_doc,
"offset() -> integer\n"
"\n"
"Get the offset of the GPIO line.");

static PyObject *gpiod_Line_offset(gpiod_LineObject *self,
				   PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	return Py_BuildValue("I", gpiod_line_offset(self->line));
}

PyDoc_STRVAR(gpiod_Line_name_doc,
"name() -> string\n"
"\n"
"Get the name of the GPIO line.");

static PyObject *gpiod_Line_name(gpiod_LineObject *self,
				 PyObject *Py_UNUSED(ignored))
{
	const char *name;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	name = gpiod_line_name(self->line);
	if (name)
		return PyUnicode_FromFormat("%s", name);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_Line_consumer_doc,
"consumer() -> string\n"
"\n"
"Get the consumer string of the GPIO line.");

static PyObject *gpiod_Line_consumer(gpiod_LineObject *self,
				     PyObject *Py_UNUSED(ignored))
{
	const char *consumer;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	consumer = gpiod_line_consumer(self->line);
	if (consumer)
		return PyUnicode_FromFormat("%s", consumer);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_Line_direction_doc,
"direction() -> integer\n"
"\n"
"Get the direction setting of this GPIO line.");

static PyObject *gpiod_Line_direction(gpiod_LineObject *self,
				      PyObject *Py_UNUSED(ignored))
{
	PyObject *ret;
	int dir;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	dir = gpiod_line_direction(self->line);

	if (dir == GPIOD_LINE_DIRECTION_INPUT)
		ret = Py_BuildValue("I", gpiod_DIRECTION_INPUT);
	else
		ret = Py_BuildValue("I", gpiod_DIRECTION_OUTPUT);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_is_active_low_doc,
"is_active_low() -> boolean\n"
"\n"
"Check if this line's signal is inverted");

static PyObject *gpiod_Line_is_active_low(gpiod_LineObject *self,
					  PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	if (gpiod_line_is_active_low(self->line))
		Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

PyDoc_STRVAR(gpiod_Line_bias_doc,
"bias() -> integer\n"
"\n"
"Get the bias setting of this GPIO line.");

static PyObject *gpiod_Line_bias(gpiod_LineObject *self,
				 PyObject *Py_UNUSED(ignored))
{
	int bias;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	bias = gpiod_line_bias(self->line);

	switch (bias) {
	case GPIOD_LINE_BIAS_PULL_UP:
		return Py_BuildValue("I", gpiod_BIAS_PULL_UP);
	case GPIOD_LINE_BIAS_PULL_DOWN:
		return Py_BuildValue("I", gpiod_BIAS_PULL_DOWN);
	case GPIOD_LINE_BIAS_DISABLE:
		return Py_BuildValue("I", gpiod_BIAS_DISABLE);
	case GPIOD_LINE_BIAS_UNKNOWN:
	default:
		return Py_BuildValue("I", gpiod_BIAS_UNKNOWN);
	}
}

PyDoc_STRVAR(gpiod_Line_is_used_doc,
"is_used() -> boolean\n"
"\n"
"Check if this line is used by the kernel or other user space process.");

static PyObject *gpiod_Line_is_used(gpiod_LineObject *self,
				    PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	if (gpiod_line_is_used(self->line))
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

PyDoc_STRVAR(gpiod_Line_is_open_drain_doc,
"is_open_drain() -> boolean\n"
"\n"
"Check if this line represents an open-drain GPIO.");

static PyObject *gpiod_Line_is_open_drain(gpiod_LineObject *self,
					  PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	if (gpiod_line_is_open_drain(self->line))
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

PyDoc_STRVAR(gpiod_Line_is_open_source_doc,
"is_open_source() -> boolean\n"
"\n"
"Check if this line represents an open-source GPIO.");

static PyObject *gpiod_Line_is_open_source(gpiod_LineObject *self,
					   PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	if (gpiod_line_is_open_source(self->line))
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

PyDoc_STRVAR(gpiod_Line_request_doc,
"request(consumer[, type[, flags[, default_val]]]) -> None\n"
"\n"
"Request this GPIO line.\n"
"\n"
"  consumer\n"
"    Name of the consumer.\n"
"  type\n"
"    Type of the request.\n"
"  flags\n"
"    Other configuration flags.\n"
"  default_val\n"
"    Default value of this line."
"\n"
"Note: default_vals argument (sequence of default values passed down to\n"
"LineBulk.request()) is still supported for backward compatibility but is\n"
"now deprecated when requesting single lines.");

static PyObject *gpiod_Line_request(gpiod_LineObject *self,
				    PyObject *args, PyObject *kwds)
{
	PyObject *ret, *def_val, *def_vals;
	gpiod_LineBulkObject *bulk_obj;
	int rv;

	if (kwds && PyDict_Size(kwds) > 0) {
		def_val = PyDict_GetItemString(kwds, "default_val");
		def_vals = PyDict_GetItemString(kwds, "default_vals");
	} else {
		def_val = def_vals = NULL;
	}

	if (def_val && def_vals) {
		PyErr_SetString(PyExc_TypeError,
				"Cannot pass both default_val and default_vals arguments at the same time");
		return NULL;
	}

	if (def_val) {
		/*
		 * If default_val was passed as a single value, we wrap it
		 * in a tuple and add it to the kwds dictionary to be passed
		 * down to LineBulk.request(). We also remove the 'default_val'
		 * entry from kwds.
		 *
		 * I'm not sure if it's allowed to modify the kwds dictionary
		 * but it doesn't seem to cause any problems. If it does then
		 * we can simply copy the dictionary before calling
		 * LineBulk.request().
		 */
		rv = PyDict_DelItemString(kwds, "default_val");
		if (rv)
			return NULL;

		def_vals = Py_BuildValue("(O)", def_val);
		if (!def_vals)
			return NULL;

		rv = PyDict_SetItemString(kwds, "default_vals", def_vals);
		if (rv) {
			Py_DECREF(def_vals);
			return NULL;
		}
	}

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	ret = gpiod_CallMethodPyArgs((PyObject *)bulk_obj,
				     "request", args, kwds);
	Py_DECREF(bulk_obj);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_is_requested_doc,
"is_requested() -> boolean\n"
"\n"
"Check if this user has ownership of this line.");

static PyObject *gpiod_Line_is_requested(gpiod_LineObject *self,
					 PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	if (gpiod_line_is_requested(self->line))
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

PyDoc_STRVAR(gpiod_Line_get_value_doc,
"get_value() -> integer\n"
"\n"
"Read the current value of this GPIO line.");

static PyObject *gpiod_Line_get_value(gpiod_LineObject *self,
				      PyObject *Py_UNUSED(ignored))
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *vals, *ret;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	vals = PyObject_CallMethod((PyObject *)bulk_obj, "get_values", "");
	Py_DECREF(bulk_obj);
	if (!vals)
		return NULL;

	ret = PyList_GetItem(vals, 0);
	Py_INCREF(ret);
	Py_DECREF(vals);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_set_value_doc,
"set_value(value) -> None\n"
"\n"
"Set the value of this GPIO line.\n"
"\n"
"  value\n"
"    New value (integer)");

static PyObject *gpiod_Line_set_value(gpiod_LineObject *self, PyObject *args)
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *val, *vals, *ret;
	int rv;

	rv = PyArg_ParseTuple(args, "O", &val);
	if (!rv)
		return NULL;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	vals = Py_BuildValue("(O)", val);
	if (!vals) {
		Py_DECREF(bulk_obj);
		return NULL;
	}

	ret = PyObject_CallMethod((PyObject *)bulk_obj,
				  "set_values", "(O)", vals);
	Py_DECREF(bulk_obj);
	Py_DECREF(vals);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_set_config_doc,
"set_config(direction,flags,value) -> None\n"
"\n"
"Set the configuration of this GPIO line.\n"
"\n"
"  direction\n"
"    New direction (integer)\n"
"  flags\n"
"    New flags (integer)\n"
"  value\n"
"    New value (integer)");

static PyObject *gpiod_Line_set_config(gpiod_LineObject *self, PyObject *args)
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *dirn, *flags, *val, *vals, *ret;
	int rv;

	val = NULL;
	rv = PyArg_ParseTuple(args, "OO|O", &dirn, &flags, &val);
	if (!rv)
		return NULL;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	if (val) {
		vals = Py_BuildValue("(O)", val);
		if (!vals) {
			Py_DECREF(bulk_obj);
			return NULL;
		}
		ret = PyObject_CallMethod((PyObject *)bulk_obj,
				"set_config", "OO(O)", dirn, flags, vals);
		Py_DECREF(vals);
	} else {
		ret = PyObject_CallMethod((PyObject *)bulk_obj,
				"set_config", "OO", dirn, flags);
	}

	Py_DECREF(bulk_obj);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_set_flags_doc,
"set_flags(flags) -> None\n"
"\n"
"Set the flags of this GPIO line.\n"
"\n"
"  flags\n"
"    New flags (integer)");

static PyObject *gpiod_Line_set_flags(gpiod_LineObject *self, PyObject *args)
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *ret;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	ret = PyObject_CallMethod((PyObject *)bulk_obj,
				  "set_flags", "O", args);
	Py_DECREF(bulk_obj);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_set_direction_input_doc,
"set_direction_input() -> None\n"
"\n"
"Set the direction of this GPIO line to input.\n");

static PyObject *gpiod_Line_set_direction_input(gpiod_LineObject *self,
						PyObject *Py_UNUSED(ignored))
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *ret;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	ret = PyObject_CallMethod((PyObject *)bulk_obj,
				  "set_direction_input", "");
	Py_DECREF(bulk_obj);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_set_direction_output_doc,
"set_direction_output(value) -> None\n"
"\n"
"Set the direction of this GPIO line to output.\n"
"\n"
"  value\n"
"    New value (integer)");

static PyObject *gpiod_Line_set_direction_output(gpiod_LineObject *self,
						 PyObject *args)
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *val, *vals, *ret;
	int rv;
	const char *fmt;

	val = NULL;
	rv = PyArg_ParseTuple(args, "|O", &val);
	if (!rv)
		return NULL;

	if (val) {
		fmt = "(O)";
		vals = Py_BuildValue(fmt, val);
	} else {
		vals = Py_BuildValue("()");
		fmt = "O"; /* pass empty args to bulk */
	}
	if (!vals)
		return NULL;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	ret = PyObject_CallMethod((PyObject *)bulk_obj,
				  "set_direction_output", fmt, vals);

	Py_DECREF(bulk_obj);
	Py_DECREF(vals);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_release_doc,
"release() -> None\n"
"\n"
"Release this GPIO line.");

static PyObject *gpiod_Line_release(gpiod_LineObject *self,
				    PyObject *Py_UNUSED(ignored))
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *ret;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	ret = PyObject_CallMethod((PyObject *)bulk_obj, "release", "");
	Py_DECREF(bulk_obj);

	return ret;
}

PyDoc_STRVAR(gpiod_Line_update_doc,
"update() -> None\n"
"\n"
"Re-read the line information from the kernel.");

static PyObject *gpiod_Line_update(gpiod_LineObject *self,
				   PyObject *Py_UNUSED(ignored))
{
	int ret;

	ret = gpiod_line_update(self->line);
	if (ret)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_Line_event_wait_doc,
"event_wait([sec[ ,nsec]]) -> boolean\n"
"\n"
"Wait for a line event to occur on this GPIO line.\n"
"\n"
"  sec\n"
"    Number of seconds to wait before timeout.\n"
"  nsec\n"
"    Number of nanoseconds to wait before timeout.\n"
"\n"
"Returns True if an event occurred on this line before timeout. False\n"
"otherwise.");

static PyObject *gpiod_Line_event_wait(gpiod_LineObject *self,
				       PyObject *args, PyObject *kwds)
{
	gpiod_LineBulkObject *bulk_obj;
	PyObject *events;

	bulk_obj = gpiod_LineToLineBulk(self);
	if (!bulk_obj)
		return NULL;

	events = gpiod_CallMethodPyArgs((PyObject *)bulk_obj,
					"event_wait", args, kwds);
	Py_DECREF(bulk_obj);
	if (!events)
		return NULL;

	if (events == Py_None) {
		Py_DECREF(Py_None);
		Py_RETURN_FALSE;
	}

	Py_DECREF(events);
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(gpiod_Line_event_read_doc,
"event_read() -> gpiod.LineEvent object\n"
"\n"
"Read a single line event from this GPIO line object.");

static gpiod_LineEventObject *gpiod_Line_event_read(gpiod_LineObject *self,
						    PyObject *Py_UNUSED(ignored))
{
	gpiod_LineEventObject *ret;
	int rv;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	ret = PyObject_New(gpiod_LineEventObject, &gpiod_LineEventType);
	if (!ret)
		return NULL;

	ret->source = NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_event_read(self->line, &ret->event);
	Py_END_ALLOW_THREADS;
	if (rv) {
		Py_DECREF(ret);
		return (gpiod_LineEventObject *)PyErr_SetFromErrno(
							PyExc_OSError);
	}

	Py_INCREF(self);
	ret->source = self;

	return ret;
}

PyDoc_STRVAR(gpiod_Line_event_read_multiple_doc,
"event_read_multiple() -> list of gpiod.LineEvent object\n"
"\n"
"Read multiple line events from this GPIO line object.");

static PyObject *gpiod_Line_event_read_multiple(gpiod_LineObject *self,
						PyObject *Py_UNUSED(ignored))
{
	struct gpiod_line_event evbuf[16];
	gpiod_LineEventObject *event;
	int rv, num_events, i;
	PyObject *events;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	memset(evbuf, 0, sizeof(evbuf));
	Py_BEGIN_ALLOW_THREADS;
	num_events = gpiod_line_event_read_multiple(self->line, evbuf,
					sizeof(evbuf) / sizeof(*evbuf));
	Py_END_ALLOW_THREADS;
	if (num_events < 0)
		return PyErr_SetFromErrno(PyExc_OSError);

	events = PyList_New(num_events);
	if (!events)
		return NULL;

	for (i = 0; i < num_events; i++) {
		event = PyObject_New(gpiod_LineEventObject,
				     &gpiod_LineEventType);
		if (!event) {
			Py_DECREF(events);
			return NULL;
		}

		memcpy(&event->event, &evbuf[i], sizeof(event->event));
		Py_INCREF(self);
		event->source = self;

		rv = PyList_SetItem(events, i, (PyObject *)event);
		if (rv < 0) {
			Py_DECREF(events);
			Py_DECREF(event);
			return NULL;
		}
	}

	return events;
}

PyDoc_STRVAR(gpiod_Line_event_get_fd_doc,
"event_get_fd() -> integer\n"
"\n"
"Get the event file descriptor number associated with this line.");

static PyObject *gpiod_Line_event_get_fd(gpiod_LineObject *self,
					 PyObject *Py_UNUSED(ignored))
{
	int fd;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	fd = gpiod_line_event_get_fd(self->line);
	if (fd < 0) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	return PyLong_FromLong(fd);
}

static PyObject *gpiod_Line_repr(gpiod_LineObject *self)
{
	PyObject *chip_name, *ret;
	const char *line_name;

	if (gpiod_ChipIsClosed(self->owner))
		return NULL;

	chip_name = PyObject_CallMethod((PyObject *)self->owner, "name", "");
	if (!chip_name)
		return NULL;

	line_name = gpiod_line_name(self->line);

	ret = PyUnicode_FromFormat("'%S:%u /%s/'", chip_name,
				   gpiod_line_offset(self->line),
				   line_name ?: "unnamed");
	Py_DECREF(chip_name);
	return ret;
}

static PyMethodDef gpiod_Line_methods[] = {
	{
		.ml_name = "owner",
		.ml_meth = (PyCFunction)gpiod_Line_owner,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_owner_doc,
	},
	{
		.ml_name = "offset",
		.ml_meth = (PyCFunction)gpiod_Line_offset,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_offset_doc,
	},
	{
		.ml_name = "name",
		.ml_meth = (PyCFunction)gpiod_Line_name,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_name_doc,
	},
	{
		.ml_name = "consumer",
		.ml_meth = (PyCFunction)gpiod_Line_consumer,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_consumer_doc,
	},
	{
		.ml_name = "direction",
		.ml_meth = (PyCFunction)gpiod_Line_direction,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_direction_doc,
	},
	{
		.ml_name = "is_active_low",
		.ml_meth = (PyCFunction)gpiod_Line_is_active_low,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_is_active_low_doc,
	},
	{
		.ml_name = "bias",
		.ml_meth = (PyCFunction)gpiod_Line_bias,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_bias_doc,
	},
	{
		.ml_name = "is_used",
		.ml_meth = (PyCFunction)gpiod_Line_is_used,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_is_used_doc,
	},
	{
		.ml_name = "is_open_drain",
		.ml_meth = (PyCFunction)gpiod_Line_is_open_drain,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_is_open_drain_doc,
	},
	{
		.ml_name = "is_open_source",
		.ml_meth = (PyCFunction)gpiod_Line_is_open_source,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_is_open_source_doc,
	},
	{
		.ml_name = "request",
		.ml_meth = (PyCFunction)(void (*)(void))gpiod_Line_request,
		.ml_flags = METH_VARARGS | METH_KEYWORDS,
		.ml_doc = gpiod_Line_request_doc,
	},
	{
		.ml_name = "is_requested",
		.ml_meth = (PyCFunction)gpiod_Line_is_requested,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_is_requested_doc,
	},
	{
		.ml_name = "get_value",
		.ml_meth = (PyCFunction)gpiod_Line_get_value,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_get_value_doc,
	},
	{
		.ml_name = "set_value",
		.ml_meth = (PyCFunction)gpiod_Line_set_value,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Line_set_value_doc,
	},
	{
		.ml_name = "set_config",
		.ml_meth = (PyCFunction)gpiod_Line_set_config,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Line_set_config_doc,
	},
	{
		.ml_name = "set_flags",
		.ml_meth = (PyCFunction)gpiod_Line_set_flags,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Line_set_flags_doc,
	},
	{
		.ml_name = "set_direction_input",
		.ml_meth = (PyCFunction)gpiod_Line_set_direction_input,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_set_direction_input_doc,
	},
	{
		.ml_name = "set_direction_output",
		.ml_meth = (PyCFunction)gpiod_Line_set_direction_output,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Line_set_direction_output_doc,
	},
	{
		.ml_name = "release",
		.ml_meth = (PyCFunction)gpiod_Line_release,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_release_doc,
	},
	{
		.ml_name = "update",
		.ml_meth = (PyCFunction)gpiod_Line_update,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_update_doc,
	},
	{
		.ml_name = "event_wait",
		.ml_meth = (PyCFunction)(void (*)(void))gpiod_Line_event_wait,
		.ml_flags = METH_VARARGS | METH_KEYWORDS,
		.ml_doc = gpiod_Line_event_wait_doc,
	},
	{
		.ml_name = "event_read",
		.ml_meth = (PyCFunction)gpiod_Line_event_read,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_event_read_doc,
	},
	{
		.ml_name = "event_read_multiple",
		.ml_meth = (PyCFunction)gpiod_Line_event_read_multiple,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_event_read_multiple_doc,
	},
	{
		.ml_name = "event_get_fd",
		.ml_meth = (PyCFunction)gpiod_Line_event_get_fd,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Line_event_get_fd_doc,
	},
	{ }
};

PyDoc_STRVAR(gpiod_LineType_doc,
"Represents a GPIO line.\n"
"\n"
"The lifetime of this object is managed by the chip that owns it. Once\n"
"the corresponding gpiod.Chip is closed, a gpiod.Line object must not be\n"
"used.\n"
"\n"
"Line objects can only be created by the owning chip.");

static PyTypeObject gpiod_LineType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod.Line",
	.tp_basicsize = sizeof(gpiod_LineObject),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = gpiod_LineType_doc,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)gpiod_Line_init,
	.tp_dealloc = (destructor)gpiod_Line_dealloc,
	.tp_repr = (reprfunc)gpiod_Line_repr,
	.tp_methods = gpiod_Line_methods,
};

static bool gpiod_LineBulkOwnerIsClosed(gpiod_LineBulkObject *self)
{
	gpiod_LineObject *line = (gpiod_LineObject *)self->lines[0];

	return gpiod_ChipIsClosed(line->owner);
}

static int gpiod_LineBulk_init(gpiod_LineBulkObject *self,
			       PyObject *args, PyObject *Py_UNUSED(ignored))
{
	PyObject *lines, *iter, *next;
	Py_ssize_t i;
	int rv;

	rv = PyArg_ParseTuple(args, "O", &lines);
	if (!rv)
		return -1;

	self->num_lines = PyObject_Size(lines);
	if (self->num_lines < 1) {
		PyErr_SetString(PyExc_TypeError,
				"Argument must be a non-empty sequence");
		return -1;
	}
	if (self->num_lines > LINE_REQUEST_MAX_LINES) {
		PyErr_SetString(PyExc_TypeError,
				"Too many objects in the sequence");
		return -1;
	}

	self->lines = PyMem_Calloc(self->num_lines, sizeof(PyObject *));
	if (!self->lines) {
		PyErr_SetString(PyExc_MemoryError, "Out of memory");
		return -1;
	}

	iter = PyObject_GetIter(lines);
	if (!iter) {
		PyMem_Free(self->lines);
		return -1;
	}

	for (i = 0;;) {
		next = PyIter_Next(iter);
		if (!next) {
			Py_DECREF(iter);
			break;
		}

		if (next->ob_type != &gpiod_LineType) {
			PyErr_SetString(PyExc_TypeError,
					"Argument must be a sequence of GPIO lines");
			Py_DECREF(next);
			Py_DECREF(iter);
			goto errout;
		}

		self->lines[i++] = next;
	}

	self->iter_idx = -1;

	return 0;

errout:

	if (i > 0) {
		for (--i; i >= 0; i--)
			Py_DECREF(self->lines[i]);
	}
	PyMem_Free(self->lines);
	self->lines = NULL;

	return -1;
}

static void gpiod_LineBulk_dealloc(gpiod_LineBulkObject *self)
{
	Py_ssize_t i;

	if (!self->lines)
		return;

	for (i = 0; i < self->num_lines; i++)
		Py_DECREF(self->lines[i]);

	PyMem_Free(self->lines);
	PyObject_Del(self);
}

static PyObject *gpiod_LineBulk_iternext(gpiod_LineBulkObject *self)
{
	if (self->iter_idx < 0) {
		self->iter_idx = 0; /* First element */
	} else if (self->iter_idx >= self->num_lines) {
		self->iter_idx = -1;
		return NULL; /* Last element */
	}

	Py_INCREF(self->lines[self->iter_idx]);
	return self->lines[self->iter_idx++];
}

PyDoc_STRVAR(gpiod_LineBulk_to_list_doc,
"to_list() -> list of gpiod.Line objects\n"
"\n"
"Convert this LineBulk to a list");

static PyObject *gpiod_LineBulk_to_list(gpiod_LineBulkObject *self,
					PyObject *Py_UNUSED(ignored))
{
	PyObject *list;
	Py_ssize_t i;
	int rv;

	list = PyList_New(self->num_lines);
	if (!list)
		return NULL;

	for (i = 0; i < self->num_lines; i++) {
		Py_INCREF(self->lines[i]);
		rv = PyList_SetItem(list, i, self->lines[i]);
		if (rv < 0) {
			Py_DECREF(list);
			return NULL;
		}
	}

	return list;
}

static struct gpiod_line_bulk *
gpiod_LineBulkObjToCLineBulk(gpiod_LineBulkObject *bulk_obj)
{
	struct gpiod_line_bulk *bulk;
	gpiod_LineObject *line_obj;
	Py_ssize_t i;

	bulk = gpiod_line_bulk_new(bulk_obj->num_lines);
	if (!bulk) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}

	for (i = 0; i < bulk_obj->num_lines; i++) {
		line_obj = (gpiod_LineObject *)bulk_obj->lines[i];
		gpiod_line_bulk_add_line(bulk, line_obj->line);
	}

	return bulk;
}

static void gpiod_MakeRequestConfig(struct gpiod_line_request_config *conf,
				    const char *consumer,
				    int request_type, int flags)
{
	memset(conf, 0, sizeof(*conf));

	conf->consumer = consumer;

	switch (request_type) {
	case gpiod_LINE_REQ_DIR_IN:
		conf->request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
		break;
	case gpiod_LINE_REQ_DIR_OUT:
		conf->request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
		break;
	case gpiod_LINE_REQ_EV_FALLING_EDGE:
		conf->request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
		break;
	case gpiod_LINE_REQ_EV_RISING_EDGE:
		conf->request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
		break;
	case gpiod_LINE_REQ_EV_BOTH_EDGES:
		conf->request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
		break;
	case gpiod_LINE_REQ_DIR_AS_IS:
	default:
		conf->request_type = GPIOD_LINE_REQUEST_DIRECTION_AS_IS;
		break;
	}

	if (flags & gpiod_LINE_REQ_FLAG_OPEN_DRAIN)
		conf->flags |= GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;
	if (flags & gpiod_LINE_REQ_FLAG_OPEN_SOURCE)
		conf->flags |= GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;
	if (flags & gpiod_LINE_REQ_FLAG_ACTIVE_LOW)
		conf->flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
	if (flags & gpiod_LINE_REQ_FLAG_BIAS_DISABLE)
		conf->flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
	if (flags & gpiod_LINE_REQ_FLAG_BIAS_PULL_DOWN)
		conf->flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
	if (flags & gpiod_LINE_REQ_FLAG_BIAS_PULL_UP)
		conf->flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
}

PyDoc_STRVAR(gpiod_LineBulk_request_doc,
"request(consumer[, type[, flags[, default_vals]]]) -> None\n"
"\n"
"Request all lines held by this LineBulk object.\n"
"\n"
"  consumer\n"
"    Name of the consumer.\n"
"  type\n"
"    Type of the request.\n"
"  flags\n"
"    Other configuration flags.\n"
"  default_vals\n"
"    List of default values.\n");

static PyObject *gpiod_LineBulk_request(gpiod_LineBulkObject *self,
					PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "consumer",
				  "type",
				  "flags",
				  "default_vals",
				  NULL };

	int rv, type = gpiod_LINE_REQ_DIR_AS_IS, flags = 0,
	    default_vals[LINE_REQUEST_MAX_LINES], val;
	PyObject *def_vals_obj = NULL, *iter, *next;
	struct gpiod_line_request_config conf;
	struct gpiod_line_bulk *bulk;
	Py_ssize_t num_def_vals;
	char *consumer = NULL;
	Py_ssize_t i;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	rv = PyArg_ParseTupleAndKeywords(args, kwds, "s|iiO", kwlist,
					 &consumer, &type,
					 &flags, &def_vals_obj);
	if (!rv)
		return NULL;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	gpiod_MakeRequestConfig(&conf, consumer, type, flags);

	if (def_vals_obj) {
		memset(default_vals, 0, sizeof(default_vals));

		num_def_vals = PyObject_Size(def_vals_obj);
		if (num_def_vals != self->num_lines) {
			PyErr_SetString(PyExc_TypeError,
					"Number of default values is not the same as the number of lines");
			return NULL;
		}

		iter = PyObject_GetIter(def_vals_obj);
		if (!iter)
			return NULL;

		for (i = 0;; i++) {
			next = PyIter_Next(iter);
			if (!next) {
				Py_DECREF(iter);
				break;
			}

			val = PyLong_AsUnsignedLong(next);
			Py_DECREF(next);
			if (PyErr_Occurred()) {
				Py_DECREF(iter);
				return NULL;
			}

			default_vals[i] = !!val;
		}
	}

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_request_bulk(bulk, &conf, default_vals);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_get_values_doc,
"get_values() -> list of integers\n"
"\n"
"Read the values of all the lines held by this LineBulk object. The index\n"
"of each value in the returned list corresponds to the index of the line\n"
"in this gpiod.LineBulk object.");

static PyObject *gpiod_LineBulk_get_values(gpiod_LineBulkObject *self,
					   PyObject *Py_UNUSED(ignored))
{
	int rv, vals[LINE_REQUEST_MAX_LINES];
	struct gpiod_line_bulk *bulk;
	PyObject *val_list, *val;
	Py_ssize_t i;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	memset(vals, 0, sizeof(vals));
	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_get_value_bulk(bulk, vals);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	val_list = PyList_New(self->num_lines);
	if (!val_list)
		return NULL;

	for (i = 0; i < self->num_lines; i++) {
		val = Py_BuildValue("i", vals[i]);
		if (!val) {
			Py_DECREF(val_list);
			return NULL;
		}

		rv = PyList_SetItem(val_list, i, val);
		if (rv < 0) {
			Py_DECREF(val);
			Py_DECREF(val_list);
			return NULL;
		}
	}

	return val_list;
}

static int gpiod_TupleToIntArray(PyObject *src, int *dst, Py_ssize_t nv)
{
	Py_ssize_t num_vals, i;
	PyObject *iter, *next;
	int val;

	num_vals = PyObject_Size(src);
	if (num_vals != nv) {
		PyErr_SetString(PyExc_TypeError,
				"Number of values must correspond to the number of lines");
		return -1;
	}

	iter = PyObject_GetIter(src);
	if (!iter)
		return -1;

	for (i = 0;; i++) {
		next = PyIter_Next(iter);
		if (!next) {
			Py_DECREF(iter);
			break;
		}

		val = PyLong_AsLong(next);
		Py_DECREF(next);
		if (PyErr_Occurred()) {
			Py_DECREF(iter);
			return -1;
		}
		dst[i] = (int)val;
	}

	return 0;
}

PyDoc_STRVAR(gpiod_LineBulk_set_values_doc,
"set_values(values) -> None\n"
"\n"
"Set the values of all the lines held by this LineBulk object.\n"
"\n"
"  values\n"
"    List of values (integers) to set.\n"
"\n"
"The number of values in the list passed as argument must be the same as\n"
"the number of lines held by this gpiod.LineBulk object. The index of each\n"
"value corresponds to the index of each line in the object.\n");

static PyObject *gpiod_LineBulk_set_values(gpiod_LineBulkObject *self,
					   PyObject *args)
{
	int rv, vals[LINE_REQUEST_MAX_LINES];
	struct gpiod_line_bulk *bulk;
	PyObject *val_list;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	memset(vals, 0, sizeof(vals));

	rv = PyArg_ParseTuple(args, "O", &val_list);
	if (!rv)
		return NULL;

	rv = gpiod_TupleToIntArray(val_list, vals, self->num_lines);
	if (rv)
		return NULL;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_set_value_bulk(bulk, vals);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_set_config_doc,
"set_config(direction,flags,values) -> None\n"
"\n"
"Set the configuration of all the lines held by this LineBulk object.\n"
"\n"
"  direction\n"
"    New direction (integer)\n"
"  flags\n"
"    New flags (integer)\n"
"  values\n"
"    List of values (integers) to set when direction is output.\n"
"\n"
"The number of values in the list passed as argument must be the same as\n"
"the number of lines held by this gpiod.LineBulk object. The index of each\n"
"value corresponds to the index of each line in the object.\n");

static PyObject *gpiod_LineBulk_set_config(gpiod_LineBulkObject *self,
					   PyObject *args)
{
	int rv, vals[LINE_REQUEST_MAX_LINES];
	struct gpiod_line_bulk *bulk;
	PyObject *val_list;
	const int *valp;
	int dirn, flags;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	val_list = NULL;
	rv = PyArg_ParseTuple(args, "ii|(O)", &dirn, &flags, &val_list);
	if (!rv)
		return NULL;

	if (val_list == NULL) {
		valp = NULL;
	} else {
		memset(vals, 0, sizeof(vals));
		rv = gpiod_TupleToIntArray(val_list, vals, self->num_lines);
		if (rv)
			return NULL;
		valp = vals;
	}

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_set_config_bulk(bulk, dirn, flags, valp);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_set_flags_doc,
"set_flags(flags) -> None\n"
"\n"
"Set the flags of all the lines held by this LineBulk object.\n"
"\n"
"  flags\n"
"    New flags (integer)");

static PyObject *gpiod_LineBulk_set_flags(gpiod_LineBulkObject *self,
					  PyObject *args)
{
	struct gpiod_line_bulk *bulk;
	int rv, flags;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	rv = PyArg_ParseTuple(args, "i", &flags);
	if (!rv)
		return NULL;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_set_flags_bulk(bulk, flags);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_set_direction_input_doc,
"set_direction_input() -> None\n"
"\n"
"Set the direction of all the lines held by this LineBulk object to input.\n");

static PyObject *gpiod_LineBulk_set_direction_input(gpiod_LineBulkObject *self,
						PyObject *Py_UNUSED(ignored))
{
	struct gpiod_line_bulk *bulk;
	int rv;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_set_direction_input_bulk(bulk);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_set_direction_output_doc,
"set_direction_output(value) -> None\n"
"\n"
"Set the direction of all the lines held by this LineBulk object to output.\n"
"\n"
"  values\n"
"    List of values (integers) to set when direction is output.\n"
"\n"
"The number of values in the list passed as argument must be the same as\n"
"the number of lines held by this gpiod.LineBulk object. The index of each\n"
"value corresponds to the index of each line in the object.\n");

static PyObject *gpiod_LineBulk_set_direction_output(
				gpiod_LineBulkObject *self,
				PyObject *args)
{
	int rv, vals[LINE_REQUEST_MAX_LINES];
	struct gpiod_line_bulk *bulk;
	PyObject *val_list;
	const int *valp;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	val_list = NULL;
	rv = PyArg_ParseTuple(args, "|O", &val_list);
	if (!rv)
		return NULL;

	if (val_list == NULL)
		valp = NULL;
	else {
		memset(vals, 0, sizeof(vals));
		rv = gpiod_TupleToIntArray(val_list, vals, self->num_lines);
		if (rv)
			return NULL;
		valp = vals;
	}

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_set_direction_output_bulk(bulk, valp);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv)
		return PyErr_SetFromErrno(PyExc_OSError);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_release_doc,
"release() -> None\n"
"\n"
"Release all lines held by this LineBulk object.");

static PyObject *gpiod_LineBulk_release(gpiod_LineBulkObject *self,
					PyObject *Py_UNUSED(ignored))
{
	struct gpiod_line_bulk *bulk;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	gpiod_line_release_bulk(bulk);
	gpiod_line_bulk_free(bulk);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_LineBulk_event_wait_doc,
"event_wait([sec[ ,nsec]]) -> gpiod.LineBulk object or None\n"
"\n"
"Poll the lines held by this LineBulk Object for line events.\n"
"\n"
"  sec\n"
"    Number of seconds to wait before timeout.\n"
"  nsec\n"
"    Number of nanoseconds to wait before timeout.\n"
"\n"
"Returns a gpiod.LineBulk object containing references to lines on which\n"
"events occurred or None if we reached the timeout without any event\n"
"occurring.");

static PyObject *gpiod_LineBulk_event_wait(gpiod_LineBulkObject *self,
					   PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "sec", "nsec", NULL };

	struct gpiod_line_bulk *bulk, *ev_bulk;
	gpiod_LineObject *line_obj;
	gpiod_ChipObject *owner;
	long sec = 0, nsec = 0;
	struct timespec ts;
	PyObject *ret;
	unsigned int idx;
	int rv;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	rv = PyArg_ParseTupleAndKeywords(args, kwds,
					 "|ll", kwlist, &sec, &nsec);
	if (!rv)
		return NULL;

	ts.tv_sec = sec;
	ts.tv_nsec = nsec;

	bulk = gpiod_LineBulkObjToCLineBulk(self);
	if (!bulk)
		return NULL;

	ev_bulk = gpiod_line_bulk_new(self->num_lines);
	if (!ev_bulk) {
		gpiod_line_bulk_free(bulk);
		return NULL;
	}

	Py_BEGIN_ALLOW_THREADS;
	rv = gpiod_line_event_wait_bulk(bulk, &ts, ev_bulk);
	gpiod_line_bulk_free(bulk);
	Py_END_ALLOW_THREADS;
	if (rv < 0) {
		gpiod_line_bulk_free(ev_bulk);
		return PyErr_SetFromErrno(PyExc_OSError);
	} else if (rv == 0) {
		gpiod_line_bulk_free(ev_bulk);
		Py_RETURN_NONE;
	}

	ret = PyList_New(gpiod_line_bulk_num_lines(ev_bulk));
	if (!ret) {
		gpiod_line_bulk_free(ev_bulk);
		return NULL;
	}

	owner = ((gpiod_LineObject *)(self->lines[0]))->owner;

	for (idx = 0; idx < gpiod_line_bulk_num_lines(ev_bulk); idx++) {
		line_obj = gpiod_MakeLineObject(owner, gpiod_line_bulk_get_line(ev_bulk, idx));
		if (!line_obj) {
			gpiod_line_bulk_free(ev_bulk);
			Py_DECREF(ret);
			return NULL;
		}

		rv = PyList_SetItem(ret, idx, (PyObject *)line_obj);
		if (rv < 0) {
			gpiod_line_bulk_free(ev_bulk);
			Py_DECREF(ret);
			return NULL;
		}
	}

	gpiod_line_bulk_free(ev_bulk);

	return ret;
}

static PyObject *gpiod_LineBulk_repr(gpiod_LineBulkObject *self)
{
	PyObject *list, *list_repr, *chip_name, *ret;
	gpiod_LineObject *line;

	if (gpiod_LineBulkOwnerIsClosed(self))
		return NULL;

	list = gpiod_LineBulk_to_list(self, NULL);
	if (!list)
		return NULL;

	list_repr = PyObject_Repr(list);
	Py_DECREF(list);
	if (!list_repr)
		return NULL;

	line = (gpiod_LineObject *)self->lines[0];
	chip_name = PyObject_CallMethod((PyObject *)line->owner, "name", "");
	if (!chip_name) {
		Py_DECREF(list_repr);
		return NULL;
	}

	ret = PyUnicode_FromFormat("%U%U", chip_name, list_repr);
	Py_DECREF(chip_name);
	Py_DECREF(list_repr);
	return ret;
}

static PyMethodDef gpiod_LineBulk_methods[] = {
	{
		.ml_name = "to_list",
		.ml_meth = (PyCFunction)gpiod_LineBulk_to_list,
		.ml_doc = gpiod_LineBulk_to_list_doc,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "request",
		.ml_meth = (PyCFunction)(void (*)(void))gpiod_LineBulk_request,
		.ml_doc = gpiod_LineBulk_request_doc,
		.ml_flags = METH_VARARGS | METH_KEYWORDS,
	},
	{
		.ml_name = "get_values",
		.ml_meth = (PyCFunction)gpiod_LineBulk_get_values,
		.ml_doc = gpiod_LineBulk_get_values_doc,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "set_values",
		.ml_meth = (PyCFunction)gpiod_LineBulk_set_values,
		.ml_doc = gpiod_LineBulk_set_values_doc,
		.ml_flags = METH_VARARGS,
	},
	{
		.ml_name = "set_config",
		.ml_meth = (PyCFunction)gpiod_LineBulk_set_config,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_LineBulk_set_config_doc,
	},
	{
		.ml_name = "set_flags",
		.ml_meth = (PyCFunction)gpiod_LineBulk_set_flags,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_LineBulk_set_flags_doc,
	},
	{
		.ml_name = "set_direction_input",
		.ml_meth = (PyCFunction)gpiod_LineBulk_set_direction_input,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_LineBulk_set_direction_input_doc,
	},
	{
		.ml_name = "set_direction_output",
		.ml_meth = (PyCFunction)gpiod_LineBulk_set_direction_output,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_LineBulk_set_direction_output_doc,
	},
	{
		.ml_name = "release",
		.ml_meth = (PyCFunction)gpiod_LineBulk_release,
		.ml_doc = gpiod_LineBulk_release_doc,
		.ml_flags = METH_NOARGS,
	},
	{
		.ml_name = "event_wait",
		.ml_meth = (PyCFunction)(void (*)(void))gpiod_LineBulk_event_wait,
		.ml_doc = gpiod_LineBulk_event_wait_doc,
		.ml_flags = METH_VARARGS | METH_KEYWORDS,
	},
	{ }
};

PyDoc_STRVAR(gpiod_LineBulkType_doc,
"Represents a set of GPIO lines.\n"
"\n"
"Objects of this type are immutable. The constructor takes as argument\n"
"a sequence of gpiod.Line objects. It doesn't accept objects of any other\n"
"type.");

static PyTypeObject gpiod_LineBulkType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod.LineBulk",
	.tp_basicsize = sizeof(gpiod_LineBulkObject),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = gpiod_LineBulkType_doc,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)gpiod_LineBulk_init,
	.tp_dealloc = (destructor)gpiod_LineBulk_dealloc,
	.tp_iter = PyObject_SelfIter,
	.tp_iternext = (iternextfunc)gpiod_LineBulk_iternext,
	.tp_repr = (reprfunc)gpiod_LineBulk_repr,
	.tp_methods = gpiod_LineBulk_methods,
};

static gpiod_LineBulkObject *gpiod_LineToLineBulk(gpiod_LineObject *line)
{
	gpiod_LineBulkObject *ret;
	PyObject *args;

	args = Py_BuildValue("((O))", line);
	if (!args)
		return NULL;

	ret = (gpiod_LineBulkObject *)PyObject_CallObject(
					(PyObject *)&gpiod_LineBulkType,
					args);
	Py_DECREF(args);

	return ret;
}

static int gpiod_Chip_init(gpiod_ChipObject *self,
			   PyObject *args, PyObject *Py_UNUSED(ignored))
{
	char *path;
	int rv;

	rv = PyArg_ParseTuple(args, "s", &path);
	if (!rv)
		return -1;

	Py_BEGIN_ALLOW_THREADS;
	self->chip = gpiod_chip_open(path);
	Py_END_ALLOW_THREADS;
	if (!self->chip) {
		PyErr_SetFromErrno(PyExc_OSError);
		return -1;
	}

	return 0;
}

static void gpiod_Chip_dealloc(gpiod_ChipObject *self)
{
	if (self->chip)
		gpiod_chip_close(self->chip);

	PyObject_Del(self);
}

static PyObject *gpiod_Chip_repr(gpiod_ChipObject *self)
{
	if (gpiod_ChipIsClosed(self))
		return NULL;

	return PyUnicode_FromFormat("'%s /%s/ %u lines'",
				    gpiod_chip_name(self->chip),
				    gpiod_chip_label(self->chip),
				    gpiod_chip_num_lines(self->chip));
}

PyDoc_STRVAR(gpiod_Chip_close_doc,
"close() -> None\n"
"\n"
"Close the associated gpiochip descriptor. The chip object must no longer\n"
"be used after this method is called.\n");

static PyObject *gpiod_Chip_close(gpiod_ChipObject *self,
				  PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self))
		return NULL;

	gpiod_chip_close(self->chip);
	self->chip = NULL;

	Py_RETURN_NONE;
}

PyDoc_STRVAR(gpiod_Chip_enter_doc,
"Controlled execution enter callback.");

static PyObject *gpiod_Chip_enter(gpiod_ChipObject *chip,
				  PyObject *Py_UNUSED(ignored))
{
	Py_INCREF(chip);
	return (PyObject *)chip;
}

PyDoc_STRVAR(gpiod_Chip_exit_doc,
"Controlled execution exit callback.");

static PyObject *gpiod_Chip_exit(gpiod_ChipObject *chip,
				 PyObject *Py_UNUSED(ignored))
{
	return PyObject_CallMethod((PyObject *)chip, "close", "");
}

PyDoc_STRVAR(gpiod_Chip_name_doc,
"name() -> string\n"
"\n"
"Get the name of the GPIO chip");

static PyObject *gpiod_Chip_name(gpiod_ChipObject *self,
				 PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self))
		return NULL;

	return PyUnicode_FromFormat("%s", gpiod_chip_name(self->chip));
}

PyDoc_STRVAR(gpiod_Chip_label_doc,
"label() -> string\n"
"\n"
"Get the label of the GPIO chip");

static PyObject *gpiod_Chip_label(gpiod_ChipObject *self,
				  PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self))
		return NULL;

	return PyUnicode_FromFormat("%s", gpiod_chip_label(self->chip));
}

PyDoc_STRVAR(gpiod_Chip_num_lines_doc,
"num_lines() -> integer\n"
"\n"
"Get the number of lines exposed by this GPIO chip.");

static PyObject *gpiod_Chip_num_lines(gpiod_ChipObject *self,
				      PyObject *Py_UNUSED(ignored))
{
	if (gpiod_ChipIsClosed(self))
		return NULL;

	return Py_BuildValue("I", gpiod_chip_num_lines(self->chip));
}

static gpiod_LineObject *
gpiod_MakeLineObject(gpiod_ChipObject *owner, struct gpiod_line *line)
{
	gpiod_LineObject *obj;

	obj = PyObject_New(gpiod_LineObject, &gpiod_LineType);
	if (!obj)
		return NULL;

	obj->line = line;
	Py_INCREF(owner);
	obj->owner = owner;

	return obj;
}

PyDoc_STRVAR(gpiod_Chip_get_line_doc,
"get_line(offset) -> gpiod.Line object\n"
"\n"
"Get the GPIO line at given offset.\n"
"\n"
"  offset\n"
"    Line offset (integer)");

static gpiod_LineObject *
gpiod_Chip_get_line(gpiod_ChipObject *self, PyObject *args)
{
	struct gpiod_line *line;
	unsigned int offset;
	int rv;

	if (gpiod_ChipIsClosed(self))
		return NULL;

	rv = PyArg_ParseTuple(args, "I", &offset);
	if (!rv)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	line = gpiod_chip_get_line(self->chip, offset);
	Py_END_ALLOW_THREADS;
	if (!line)
		return (gpiod_LineObject *)PyErr_SetFromErrno(PyExc_OSError);

	return gpiod_MakeLineObject(self, line);
}

static gpiod_LineBulkObject *gpiod_ListToLineBulk(PyObject *lines)
{
	gpiod_LineBulkObject *bulk;
	PyObject *arg;

	arg = PyTuple_Pack(1, lines);
	if (!arg)
		return NULL;

	bulk = (gpiod_LineBulkObject *)PyObject_CallObject(
					(PyObject *)&gpiod_LineBulkType,
					arg);
	Py_DECREF(arg);

	return bulk;
}

static gpiod_LineBulkObject *
gpiod_LineBulkObjectFromBulk(gpiod_ChipObject *chip, struct gpiod_line_bulk *bulk)
{
	gpiod_LineBulkObject *bulk_obj;
	gpiod_LineObject *line_obj;
	struct gpiod_line *line;
	unsigned int idx;
	PyObject *list;
	int rv;

	list = PyList_New(gpiod_line_bulk_num_lines(bulk));
	if (!list)
		return NULL;

	for (idx = 0; idx < gpiod_line_bulk_num_lines(bulk); idx++) {
		line = gpiod_line_bulk_get_line(bulk, idx);
		line_obj = gpiod_MakeLineObject(chip, line);
		if (!line_obj) {
			Py_DECREF(list);
			return NULL;
		}

		rv = PyList_SetItem(list, idx, (PyObject *)line_obj);
		if (rv < 0) {
			Py_DECREF(line_obj);
			Py_DECREF(list);
			return NULL;
		}
	}

	bulk_obj = gpiod_ListToLineBulk(list);
	Py_DECREF(list);
	if (!bulk_obj)
		return NULL;

	return bulk_obj;
}

PyDoc_STRVAR(gpiod_Chip_find_line_doc,
"find_line(name) -> gpiod.LineBulk object or None\n"
"\n"
"Find all GPIO lines by name among lines exposed by this GPIO chip..\n"
"\n"
"  name\n"
"    Line name (string)\n"
"  unique\n"
"    Indicates whether an exception should be raised if more than one lines\n"
"    matches the name\n"
"\n"
"Returns a gpiod.LineBulk object containing all matching lines or None if\n"
"line with given name is not associated with this chip.");

static gpiod_LineBulkObject *
gpiod_Chip_find_line(gpiod_ChipObject *self, PyObject *args, PyObject *kwds)
{
	static char *kwlist[] = { "", "unique", NULL };

	gpiod_LineBulkObject *bulk_obj;
	struct gpiod_line_bulk *bulk;
	int rv, unique = 0;
	const char *name;

	if (gpiod_ChipIsClosed(self))
		return NULL;

	rv = PyArg_ParseTupleAndKeywords(args, kwds, "s|p",
					 kwlist, &name, &unique);
	if (!rv)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	bulk = gpiod_chip_find_line(self->chip, name);
	Py_END_ALLOW_THREADS;
	if (!bulk) {
		if (errno == ENOENT) {
			Py_INCREF(Py_None);
			return (gpiod_LineBulkObject *)Py_None;
		}

		return (gpiod_LineBulkObject *)PyErr_SetFromErrno(
							PyExc_OSError);
	}

	if (unique && gpiod_line_bulk_num_lines(bulk) > 1) {
		gpiod_line_bulk_free(bulk);
		PyErr_SetString(PyExc_RuntimeError, "line not unique");
		return NULL;
	}

	bulk_obj = gpiod_LineBulkObjectFromBulk(self, bulk);
	gpiod_line_bulk_free(bulk);
	return bulk_obj;
}

PyDoc_STRVAR(gpiod_Chip_get_lines_doc,
"get_lines(offsets) -> gpiod.LineBulk object\n"
"\n"
"Get a set of GPIO lines by their offsets.\n"
"\n"
"  offsets\n"
"    List of lines offsets.");

static gpiod_LineBulkObject *
gpiod_Chip_get_lines(gpiod_ChipObject *self, PyObject *args)
{
	PyObject *offsets, *iter, *next, *lines, *arg;
	gpiod_LineBulkObject *bulk;
	Py_ssize_t num_offsets, i;
	gpiod_LineObject *line;
	int rv;

	rv = PyArg_ParseTuple(args, "O", &offsets);
	if (!rv)
		return NULL;

	num_offsets = PyObject_Size(offsets);
	if (num_offsets < 1) {
		PyErr_SetString(PyExc_TypeError,
				"Argument must be a non-empty sequence of offsets");
		return NULL;
	}

	lines = PyList_New(num_offsets);
	if (!lines)
		return NULL;

	iter = PyObject_GetIter(offsets);
	if (!iter) {
		Py_DECREF(lines);
		return NULL;
	}

	for (i = 0;;) {
		next = PyIter_Next(iter);
		if (!next) {
			Py_DECREF(iter);
			break;
		}

		arg = PyTuple_Pack(1, next);
		Py_DECREF(next);
		if (!arg) {
			Py_DECREF(iter);
			Py_DECREF(lines);
			return NULL;
		}

		line = gpiod_Chip_get_line(self, arg);
		Py_DECREF(arg);
		if (!line) {
			Py_DECREF(iter);
			Py_DECREF(lines);
			return NULL;
		}

		rv = PyList_SetItem(lines, i++, (PyObject *)line);
		if (rv < 0) {
			Py_DECREF(line);
			Py_DECREF(iter);
			Py_DECREF(lines);
			return NULL;
		}
	}

	bulk = gpiod_ListToLineBulk(lines);
	Py_DECREF(lines);
	if (!bulk)
		return NULL;

	return bulk;
}

PyDoc_STRVAR(gpiod_Chip_get_all_lines_doc,
"get_all_lines() -> gpiod.LineBulk object\n"
"\n"
"Get all lines exposed by this Chip.");

static gpiod_LineBulkObject *
gpiod_Chip_get_all_lines(gpiod_ChipObject *self, PyObject *Py_UNUSED(ignored))
{
	gpiod_LineBulkObject *bulk_obj;
	struct gpiod_line_bulk *bulk;

	if (gpiod_ChipIsClosed(self))
		return NULL;

	bulk = gpiod_chip_get_all_lines(self->chip);
	if (!bulk)
		return (gpiod_LineBulkObject *)PyErr_SetFromErrno(
							PyExc_OSError);

	bulk_obj = gpiod_LineBulkObjectFromBulk(self, bulk);
	gpiod_line_bulk_free(bulk);
	return bulk_obj;
}

static PyMethodDef gpiod_Chip_methods[] = {
	{
		.ml_name = "close",
		.ml_meth = (PyCFunction)gpiod_Chip_close,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Chip_close_doc,
	},
	{
		.ml_name = "__enter__",
		.ml_meth = (PyCFunction)gpiod_Chip_enter,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Chip_enter_doc,
	},
	{
		.ml_name = "__exit__",
		.ml_meth = (PyCFunction)gpiod_Chip_exit,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Chip_exit_doc,
	},
	{
		.ml_name = "name",
		.ml_meth = (PyCFunction)gpiod_Chip_name,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Chip_name_doc,
	},
	{
		.ml_name = "label",
		.ml_meth = (PyCFunction)gpiod_Chip_label,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Chip_label_doc,
	},
	{
		.ml_name = "num_lines",
		.ml_meth = (PyCFunction)gpiod_Chip_num_lines,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Chip_num_lines_doc,
	},
	{
		.ml_name = "get_line",
		.ml_meth = (PyCFunction)gpiod_Chip_get_line,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Chip_get_line_doc,
	},
	{
		.ml_name = "find_line",
		.ml_meth = (PyCFunction)(void (*)(void))gpiod_Chip_find_line,
		.ml_flags = METH_VARARGS | METH_KEYWORDS,
		.ml_doc = gpiod_Chip_find_line_doc,
	},
	{
		.ml_name = "get_lines",
		.ml_meth = (PyCFunction)gpiod_Chip_get_lines,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Chip_get_lines_doc,
	},
	{
		.ml_name = "get_all_lines",
		.ml_meth = (PyCFunction)gpiod_Chip_get_all_lines,
		.ml_flags = METH_NOARGS,
		.ml_doc = gpiod_Chip_get_all_lines_doc,
	},
	{ }
};

PyDoc_STRVAR(gpiod_ChipType_doc,
"Represents a GPIO chip.\n"
"\n"
"Chip object manages all resources associated with the GPIO chip\n"
"it represents.\n"
"\n"
"The gpiochip device file is opened during the object's construction.\n"
"The Chip object's constructor takes a description string as argument the\n"
"meaning of which depends on the second, optional parameter which defines\n"
"the way the description string should be interpreted. The available\n"
"options are: OPEN_BY_NAME, OPEN_BY_NUMBER, OPEN_BY_PATH and OPEN_LOOKUP.\n"
"The last option means that libgpiod should open the chip based on the best\n"
"guess what the path is. This is also the default if the second argument is\n"
"missing.\n"
"\n"
"Callers must close the chip by calling the close() method when it's no\n"
"longer used.\n"
"\n"
"Example:\n"
"\n"
"    chip = gpiod.Chip('gpiochip0', gpiod.Chip.OPEN_BY_NAME)\n"
"    do_something(chip)\n"
"    chip.close()\n"
"\n"
"The gpiod.Chip class also supports controlled execution ('with' statement).\n"
"\n"
"Example:\n"
"\n"
"    with gpiod.Chip('0', gpiod.Chip.OPEN_BY_NUMBER) as chip:\n"
"        do_something(chip)");

static PyTypeObject gpiod_ChipType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod.Chip",
	.tp_basicsize = sizeof(gpiod_ChipObject),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = gpiod_ChipType_doc,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)gpiod_Chip_init,
	.tp_dealloc = (destructor)gpiod_Chip_dealloc,
	.tp_repr = (reprfunc)gpiod_Chip_repr,
	.tp_methods = gpiod_Chip_methods,
};

static int gpiod_LineIter_init(gpiod_LineIterObject *self,
			       PyObject *args, PyObject *Py_UNUSED(ignored))
{
	gpiod_ChipObject *chip_obj;
	int rv;

	rv = PyArg_ParseTuple(args, "O!", &gpiod_ChipType,
			      (PyObject *)&chip_obj);
	if (!rv)
		return -1;

	if (gpiod_ChipIsClosed(chip_obj))
		return -1;

	self->offset = 0;
	self->owner = chip_obj;
	Py_INCREF(chip_obj);

	return 0;
}

static void gpiod_LineIter_dealloc(gpiod_LineIterObject *self)
{
	PyObject_Del(self);
}

static gpiod_LineObject *gpiod_LineIter_next(gpiod_LineIterObject *self)
{
	struct gpiod_line *line;

	if (self->offset == gpiod_chip_num_lines(self->owner->chip))
		return NULL; /* Last element. */

	line = gpiod_chip_get_line(self->owner->chip, self->offset++);
	if (!line)
		return (gpiod_LineObject *)PyErr_SetFromErrno(PyExc_OSError);

	return gpiod_MakeLineObject(self->owner, line);
}

PyDoc_STRVAR(gpiod_LineIterType_doc,
"Allows to iterate over all lines exposed by a GPIO chip.\n"
"\n"
"New line iterator is created by passing a reference to an open gpiod.Chip\n"
"object to the constructor of gpiod.LineIter.\n"
"\n"
"Caller doesn't need to handle the resource management for lines as their\n"
"lifetime is managed by the owning chip.\n"
"\n"
"Example:\n"
"\n"
"    chip = gpiod.Chip('gpiochip0')\n"
"    for line in gpiod.LineIter(chip):\n"
"        do_stuff_with_line(line)");

static PyTypeObject gpiod_LineIterType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "gpiod.LineIter",
	.tp_basicsize = sizeof(gpiod_LineIterObject),
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_doc = gpiod_LineIterType_doc,
	.tp_new = PyType_GenericNew,
	.tp_init = (initproc)gpiod_LineIter_init,
	.tp_dealloc = (destructor)gpiod_LineIter_dealloc,
	.tp_iter = PyObject_SelfIter,
	.tp_iternext = (iternextfunc)gpiod_LineIter_next,
};

typedef struct {
	const char *name;
	PyTypeObject *typeobj;
} gpiod_PyType;

static gpiod_PyType gpiod_PyType_list[] = {
	{ .name = "Chip",	.typeobj = &gpiod_ChipType,		},
	{ .name = "Line",	.typeobj = &gpiod_LineType,		},
	{ .name = "LineEvent",	.typeobj = &gpiod_LineEventType,	},
	{ .name = "LineBulk",	.typeobj = &gpiod_LineBulkType,		},
	{ .name = "LineIter",	.typeobj = &gpiod_LineIterType,		},
	{ }
};

typedef struct {
	PyTypeObject *typeobj;
	const char *name;
	long int val;
} gpiod_ConstDescr;

static gpiod_ConstDescr gpiod_ConstList[] = {
	{
		.typeobj = &gpiod_LineType,
		.name = "DIRECTION_INPUT",
		.val = gpiod_DIRECTION_INPUT,
	},
	{
		.typeobj = &gpiod_LineType,
		.name = "DIRECTION_OUTPUT",
		.val = gpiod_DIRECTION_OUTPUT,
	},
	{
		.typeobj = &gpiod_LineType,
		.name = "BIAS_UNKNOWN",
		.val = gpiod_BIAS_UNKNOWN,
	},
	{
		.typeobj = &gpiod_LineType,
		.name = "BIAS_DISABLE",
		.val = gpiod_BIAS_DISABLE,
	},
	{
		.typeobj = &gpiod_LineType,
		.name = "BIAS_PULL_UP",
		.val = gpiod_BIAS_PULL_UP,
	},
	{
		.typeobj = &gpiod_LineType,
		.name = "BIAS_PULL_DOWN",
		.val = gpiod_BIAS_PULL_DOWN,
	},
	{
		.typeobj = &gpiod_LineEventType,
		.name = "RISING_EDGE",
		.val = gpiod_RISING_EDGE,
	},
	{
		.typeobj = &gpiod_LineEventType,
		.name = "FALLING_EDGE",
		.val = gpiod_FALLING_EDGE,
	},
	{ }
};

PyDoc_STRVAR(gpiod_Module_is_gpiochip_device_doc,
"is_gpiochip_device(path) -> boolean\n"
"\n"
"Check if the file pointed to by path is a GPIO chip character device.\n"
"Returns true if so, False otherwise.\n"
"\n"
"  path\n"
"    Path to the file that should be checked.\n");

static PyObject *
gpiod_Module_is_gpiochip_device(PyObject *Py_UNUSED(self), PyObject *args)
{
	const char *path;
	int ret;

	ret = PyArg_ParseTuple(args, "s", &path);
	if (!ret)
		return NULL;

	if (gpiod_is_gpiochip_device(path))
		Py_RETURN_TRUE;

	Py_RETURN_FALSE;
}

static PyMethodDef gpiod_module_methods[] = {
	{
		.ml_name = "is_gpiochip_device",
		.ml_meth = (PyCFunction)gpiod_Module_is_gpiochip_device,
		.ml_flags = METH_VARARGS,
		.ml_doc = gpiod_Module_is_gpiochip_device_doc,
	},
	{ }
};

PyDoc_STRVAR(gpiod_Module_doc,
"Python bindings for libgpiod.\n\
\n\
This module wraps the native C API of libgpiod in a set of python classes.");

static PyModuleDef gpiod_Module = {
	PyModuleDef_HEAD_INIT,
	.m_name = "gpiod",
	.m_doc = gpiod_Module_doc,
	.m_size = -1,
	.m_methods = gpiod_module_methods,
};

typedef struct {
	const char *name;
	long int value;
} gpiod_ModuleConst;

static gpiod_ModuleConst gpiod_ModuleConsts[] = {
	{
		.name = "LINE_REQ_DIR_AS_IS",
		.value = gpiod_LINE_REQ_DIR_AS_IS,
	},
	{
		.name = "LINE_REQ_DIR_IN",
		.value = gpiod_LINE_REQ_DIR_IN,
	},
	{
		.name = "LINE_REQ_DIR_OUT",
		.value = gpiod_LINE_REQ_DIR_OUT,
	},
	{
		.name = "LINE_REQ_EV_FALLING_EDGE",
		.value = gpiod_LINE_REQ_EV_FALLING_EDGE,
	},
	{
		.name = "LINE_REQ_EV_RISING_EDGE",
		.value = gpiod_LINE_REQ_EV_RISING_EDGE,
	},
	{
		.name = "LINE_REQ_EV_BOTH_EDGES",
		.value = gpiod_LINE_REQ_EV_BOTH_EDGES,
	},
	{
		.name = "LINE_REQ_FLAG_OPEN_DRAIN",
		.value = gpiod_LINE_REQ_FLAG_OPEN_DRAIN,
	},
	{
		.name = "LINE_REQ_FLAG_OPEN_SOURCE",
		.value = gpiod_LINE_REQ_FLAG_OPEN_SOURCE,
	},
	{
		.name = "LINE_REQ_FLAG_ACTIVE_LOW",
		.value = gpiod_LINE_REQ_FLAG_ACTIVE_LOW,
	},
	{
		.name = "LINE_REQ_FLAG_BIAS_DISABLE",
		.value = gpiod_LINE_REQ_FLAG_BIAS_DISABLE,
	},
	{
		.name = "LINE_REQ_FLAG_BIAS_PULL_DOWN",
		.value = gpiod_LINE_REQ_FLAG_BIAS_PULL_DOWN,
	},
	{
		.name = "LINE_REQ_FLAG_BIAS_PULL_UP",
		.value = gpiod_LINE_REQ_FLAG_BIAS_PULL_UP,
	},
	{ }
};

PyMODINIT_FUNC PyInit_gpiod(void)
{
	gpiod_ConstDescr *const_descr;
	gpiod_ModuleConst *mod_const;
	PyObject *module, *val;
	gpiod_PyType *type;
	unsigned int i;
	int rv;

	module = PyModule_Create(&gpiod_Module);
	if (!module)
		return NULL;

	for (i = 0; gpiod_PyType_list[i].typeobj; i++) {
		type = &gpiod_PyType_list[i];

		rv = PyType_Ready(type->typeobj);
		if (rv)
			return NULL;

		Py_INCREF(type->typeobj);
		rv = PyModule_AddObject(module, type->name,
					(PyObject *)type->typeobj);
		if (rv < 0)
			return NULL;
	}

	for (i = 0; gpiod_ConstList[i].name; i++) {
		const_descr = &gpiod_ConstList[i];

		val = PyLong_FromLong(const_descr->val);
		if (!val)
			return NULL;

		rv = PyDict_SetItemString(const_descr->typeobj->tp_dict,
					  const_descr->name, val);
		Py_DECREF(val);
		if (rv)
			return NULL;
	}

	for (i = 0; gpiod_ModuleConsts[i].name; i++) {
		mod_const = &gpiod_ModuleConsts[i];

		rv = PyModule_AddIntConstant(module,
					     mod_const->name, mod_const->value);
		if (rv < 0)
			return NULL;
	}

	rv = PyModule_AddStringConstant(module, "__version__",
					gpiod_version_string());
	if (rv < 0)
		return NULL;

	return module;
}
