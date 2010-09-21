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
#include <math.h>
#include "pyomodule.h"
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *input1;
    PyObject *input2;
    Stream *input1_stream;
    Stream *input2_stream;
    float fadetime;
    int switcher;
    float currentTime;
    float sampleToSec;
} InputFader;

static void InputFader_setProcMode(InputFader *self) {};

static void InputFader_process_only_first(InputFader *self) 
{
    int i;
    float *in = Stream_getData((Stream *)self->input1_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i];
    }
}

static void InputFader_process_only_second(InputFader *self) 
{
    int i;
    float *in = Stream_getData((Stream *)self->input2_stream);
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = in[i];
    }
}

static void InputFader_process_one(InputFader *self) 
{
    int i;
    float sclfade, val;
    float *in1 = Stream_getData((Stream *)self->input1_stream);
    float *in2 = Stream_getData((Stream *)self->input2_stream);
    
    val = 0.0;
    sclfade = 1. / self->fadetime;
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime < self->fadetime) {
            val = sqrtf(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }    
        else
            val = 1.;

        self->data[i] = in1[i] * val + in2[i] * (1 - val);
    }
    if (val == 1.)
        self->proc_func_ptr = InputFader_process_only_first;

}

static void InputFader_process_two(InputFader *self) 
{
    int i;
    float sclfade, val;
    float *in1 = Stream_getData((Stream *)self->input1_stream);
    float *in2 = Stream_getData((Stream *)self->input2_stream);

    val = 0.0;
    sclfade = 1. / self->fadetime;
    for (i=0; i<self->bufsize; i++) {
        if (self->currentTime < self->fadetime) {
            val = sqrtf(self->currentTime * sclfade);
            self->currentTime += self->sampleToSec;
        }    
        else
            val = 1.;
        
        self->data[i] = in2[i] * val + in1[i] * (1 - val);
    }
    if (val == 1.)
        self->proc_func_ptr = InputFader_process_only_second;
}

static void
InputFader_compute_next_data_frame(InputFader *self)
{   
    (*self->proc_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
InputFader_traverse(InputFader *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input1);
    Py_VISIT(self->input1_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int 
InputFader_clear(InputFader *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input1);
    Py_CLEAR(self->input1_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
InputFader_dealloc(InputFader* self)
{
    free(self->data);
    InputFader_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * InputFader_deleteStream(InputFader *self) { DELETE_STREAM };

static PyObject *
InputFader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    InputFader *self;
    self = (InputFader *)type->tp_alloc(type, 0);
    
    self->switcher = 0;
    self->fadetime = 0.05;
    self->currentTime = 0.0;
    
    INIT_OBJECT_COMMON

    self->sampleToSec = 1. / self->sr;
    
    Stream_setFunctionPtr(self->stream, InputFader_compute_next_data_frame);
    self->mode_func_ptr = InputFader_setProcMode;
    self->proc_func_ptr = InputFader_process_only_first;
    
    return (PyObject *)self;
}

static int
InputFader_init(InputFader *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp=NULL, *streamtmp;
    
    static char *kwlist[] = {"input", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &inputtmp))
        return -1; 

    Py_INCREF(inputtmp);
    Py_XDECREF(self->input1);
    self->input1 = inputtmp;
    streamtmp = PyObject_CallMethod((PyObject *)self->input1, "_getStream", NULL);
    Py_INCREF(streamtmp);
    Py_XDECREF(self->input1_stream);
    self->input1_stream = (Stream *)streamtmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    InputFader_compute_next_data_frame((InputFader *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject *
InputFader_setInput(InputFader *self, PyObject *args, PyObject *kwds)
{
	PyObject *tmp, *streamtmp;

    static char *kwlist[] = {"input", "fadetime", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|f", kwlist, &tmp, &self->fadetime))
        return PyInt_FromLong(-1);
    
    self->switcher = (self->switcher + 1) % 2;
    self->currentTime = 0.0;
    if (self->fadetime == 0)
        self->fadetime = 0.0001;
    
    Py_INCREF(tmp);

    if (self->switcher == 0) {
        Py_DECREF(self->input1);
        self->input1 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input1, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input1_stream);
        self->input1_stream = (Stream *)streamtmp;
        self->proc_func_ptr = InputFader_process_one;
	}
    else {
        Py_XDECREF(self->input2);
        self->input2 = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->input2_stream);
        self->input2_stream = (Stream *)streamtmp;
        self->proc_func_ptr = InputFader_process_two;
	}    
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject * InputFader_getServer(InputFader* self) { GET_SERVER };
static PyObject * InputFader_getStream(InputFader* self) { GET_STREAM };

static PyObject * InputFader_play(InputFader *self) { PLAY };
static PyObject * InputFader_out(InputFader *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * InputFader_stop(InputFader *self) { STOP };

static PyMemberDef InputFader_members[] = {
    {"server", T_OBJECT_EX, offsetof(InputFader, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(InputFader, stream), 0, "Stream object."},
    {"input1", T_OBJECT_EX, offsetof(InputFader, input1), 0, "First input."},
    {"input2", T_OBJECT_EX, offsetof(InputFader, input2), 0, "Second input."},
    {NULL}  /* Sentinel */
};

static PyMethodDef InputFader_methods[] = {
    {"getServer", (PyCFunction)InputFader_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)InputFader_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)InputFader_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)InputFader_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)InputFader_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"setInput", (PyCFunction)InputFader_setInput, METH_VARARGS|METH_KEYWORDS, "Crossfade between current stream and given stream."},
    {"stop", (PyCFunction)InputFader_stop, METH_NOARGS, "Stops computing."},
    {NULL}  /* Sentinel */
};

PyTypeObject InputFaderType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.InputFader_base",         /*tp_name*/
    sizeof(InputFader),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)InputFader_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "InputFader objects. Generates a crossfade between current input sound stream and new input sound stream.",  /* tp_doc */
    (traverseproc)InputFader_traverse,   /* tp_traverse */
    (inquiry)InputFader_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    InputFader_methods,             /* tp_methods */
    InputFader_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)InputFader_init,      /* tp_init */
    0,                         /* tp_alloc */
    InputFader_new,                 /* tp_new */
};

