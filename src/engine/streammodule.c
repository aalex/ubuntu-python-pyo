/*************************************************************************
 * Copyright 2010 Olivier Belanger                                        *                  
 *                                                                        * 
 * This file is part of pyo, a python module to help digital signal       *
 * processing script creation.                                            *  
 *                                                                        * 
 * pyo is free software: you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    * 
 *                                                                        *
 * pyo is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *    
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with pyo.  If not, see <http://www.gnu.org/licenses/>.           *
 *************************************************************************/

#include <Python.h>
#include "structmember.h"
#include "pyomodule.h"

#define __STREAM_MODULE
#include "streammodule.h"
#undef __STREAM_MODULE

int stream_id = 1;

int 
Stream_getNewStreamId() 
{
    return stream_id++;
}

static void
Stream_dealloc(Stream* self)
{
    free(self->data);
    Py_XDECREF(self->streamobject);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
Stream_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Stream *self;
    MAKE_NEW_STREAM(self, type, NULL);
    return (PyObject *)self;
}

static int
Stream_init(Stream *self, PyObject *args, PyObject *kwds)
{
    PyObject *object=NULL, *tmp;
     
    static char *kwlist[] = {"streamobject", NULL};
 
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &object)){
        return -1;
    }
 
    if (object) {
        tmp = self->streamobject;
        Py_INCREF(object);
        self->streamobject = object;
        self->active = 0;
        self->chnl = 0;
        self->todac = 0;
        self->duration = 0;
        self->bufferCountWait = 0;
        self->bufferCount = 0;
        Py_DECREF(tmp);
    }
 
    return 0;
}

PyObject *
Stream_getStreamObject(Stream *self)
{
    Py_INCREF(self->streamobject);
    return self->streamobject;
}

int
Stream_getStreamId(Stream *self)
{
    return self->sid;
}

int
Stream_getStreamActive(Stream *self)
{
    return self->active;
}

int
Stream_getStreamChnl(Stream *self)
{
    return self->chnl;
}

int
Stream_getStreamToDac(Stream *self)
{
    return self->todac;
}

int
Stream_getBufferCountWait(Stream *self)
{
    return self->bufferCountWait;
}

int
Stream_getDuration(Stream *self)
{
    return self->duration;
}

MYFLT *
Stream_getData(Stream *self)
{
    return (MYFLT *)self->data;
}    

void
Stream_setData(Stream *self, MYFLT *data)
{
    self->data = data;
}    

void Stream_setFunctionPtr(Stream *self, void *ptr)
{
    self->funcptr = ptr;
}

void Stream_callFunction(Stream *self)
{
    (*self->funcptr)(self->streamobject);
}    

void Stream_IncrementBufferCount(Stream *self) 
{
    self->bufferCount++;
    if (self->bufferCount >= self->bufferCountWait) {
        self->active = 1;
        self->bufferCountWait = self->bufferCount = 0;
    }
}

void Stream_IncrementDurationCount(Stream *self) 
{
    self->bufferCount++;
    if (self->bufferCount >= self->duration) {
        PyObject_CallMethod((PyObject *)Stream_getStreamObject(self), "stop", NULL);
        self->duration = self->bufferCount = 0;
    }
}

static PyObject *
Stream_getValue(Stream *self) {
    return Py_BuildValue(TYPE_F, self->data[self->bufsize-1]);
}

static PyMethodDef Stream_methods[] = {
{"getValue", (PyCFunction)Stream_getValue, METH_NOARGS, "Returns the first sample of the current buffer."},
{NULL}  /* Sentinel */
};

PyTypeObject StreamType = {
    PyObject_HEAD_INIT(NULL)
    0, /*ob_size*/
    "pyo.Stream", /*tp_name*/
    sizeof(Stream), /*tp_basicsize*/
    0, /*tp_itemsize*/
    (destructor)Stream_dealloc, /*tp_dealloc*/
    0, /*tp_print*/
    0, /*tp_getattr*/
    0, /*tp_setattr*/
    0, /*tp_compare*/
    0, /*tp_repr*/
    0, /*tp_as_number*/
    0, /*tp_as_sequence*/
    0, /*tp_as_mapping*/
    0, /*tp_hash */
    0, /*tp_call*/
    0, /*tp_str*/
    0, /*tp_getattro*/
    0, /*tp_setattro*/
    0, /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
"\n\
Audio stream objects. For internal use only. \n\n\
A Stream object must never be instantiated by the user. \n\n\
A Stream is a mono buffer of audio samples. It is used to pass \n\
audio between objects and the server. A PyoObject can manage many \n\
streams if, for example, a list is given to a parameter. \n\n\
A Sine object with only one stream:\n\n\
    a = Sine(freq=1000)\n\n\
    len(a)\n\n\
    1\n\n\n\
A Sine object with four streams:\n\n\
    a = Sine(freq=[250,500,750,100])\n\n\
    len(a)\n\n\
    4\n\n\n\
The first stream of this object contains the samples from the 250Hz waveform.\n\
The second stream contains the samples from the 500Hz waveform, and so on.\n\n\
User can call a specific stream of an object by giving the position of the stream\n\
between brackets, beginning at 0. To retrieve only the third stream of our object:\n\n\
    a[2].out()\n\
", /* tp_doc */
    0, /* tp_traverse */
    0, /* tp_clear */
    0, /* tp_richcompare */
    0, /* tp_weaklistoffset */
    0, /* tp_iter */
    0, /* tp_iternext */
    Stream_methods, /* tp_methods */
    0, /* tp_members */
    0, /* tp_getset */
    0, /* tp_base */
    0, /* tp_dict */
    0, /* tp_descr_get */
    0, /* tp_descr_set */
    0, /* tp_dictoffset */
    (initproc)Stream_init, /* tp_init */
    0, /* tp_alloc */
    Stream_new, /* tp_new */
};
