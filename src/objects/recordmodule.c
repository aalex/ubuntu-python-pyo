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
#include "streammodule.h"
#include "servermodule.h"
#include "dummymodule.h"
#include "sndfile.h"
#include "interpolation.h"

/************/
/* Record */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input_list;
    PyObject *input_stream_list;
    int chnls;
    int buffering;
    int count;
    int listlen;
    char *recpath;
    SNDFILE *recfile;
    SF_INFO recinfo;
    MYFLT *buffer;
} Record;

static void
Record_process(Record *self) {
    int i, j, chnl, offset, totlen;
    MYFLT *in;

    totlen = self->chnls*self->bufsize*self->buffering;
    
    if (self->count == self->buffering) {
        self->count = 0;
        for (i=0; i<totlen; i++) {
            self->buffer[i] = 0.0;
        }
    }    

    offset = self->bufsize * self->chnls * self->count;
    
    for (j=0; j<self->listlen; j++) {
        chnl = j % self->chnls;
        in = Stream_getData((Stream *)PyList_GET_ITEM(self->input_stream_list, j));
        for (i=0; i<self->bufsize; i++) {
            self->buffer[i*self->chnls+chnl+offset] += in[i];
        }
    }
    self->count++;
    
    if (self->count == self->buffering)
        SF_WRITE(self->recfile, self->buffer, totlen);
}

static void
Record_setProcMode(Record *self)
{       
    self->proc_func_ptr = Record_process;   
}

static void
Record_compute_next_data_frame(Record *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
Record_traverse(Record *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input_list);
    Py_VISIT(self->input_stream_list);
    return 0;
}

static int 
Record_clear(Record *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input_list);
    Py_CLEAR(self->input_stream_list);
    return 0;
}

static void
Record_dealloc(Record* self)
{
    free(self->data);
    free(self->buffer);
    Record_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Record_deleteStream(Record *self) { DELETE_STREAM };

static PyObject *
Record_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Record *self;
    self = (Record *)type->tp_alloc(type, 0);
    
    self->chnls = 2;
    self->buffering = 4;
    self->count = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Record_compute_next_data_frame);
    self->mode_func_ptr = Record_setProcMode;
    return (PyObject *)self;
}

static int
Record_init(Record *self, PyObject *args, PyObject *kwds)
{
    int i, buflen;
    int fileformat = 0;
    int sampletype = 0;
    PyObject *input_listtmp;
    
    static char *kwlist[] = {"input", "filename", "chnls", "fileformat", "sampletype", "buffering", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os|iiii", kwlist, &input_listtmp, &self->recpath, &self->chnls, &fileformat, &sampletype, &self->buffering))
        return -1; 
    
    Py_XDECREF(self->input_list);
    self->input_list = input_listtmp;
    self->listlen = PyList_Size(self->input_list);
    self->input_stream_list = PyList_New(self->listlen);
    for (i=0; i<self->listlen; i++) {
        PyList_SET_ITEM(self->input_stream_list, i, PyObject_CallMethod(PyList_GET_ITEM(self->input_list, i), "_getStream", NULL));
    }

    /* Prepare sfinfo */
    self->recinfo.samplerate = (int)self->sr;
    self->recinfo.channels = self->chnls;
    
    switch (fileformat) {
        case 0:
            self->recinfo.format = SF_FORMAT_WAV;
            break;
        case 1:
            self->recinfo.format = SF_FORMAT_AIFF;
            break;
    }
    switch (sampletype) {
        case 0:
            self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_16;
            break;
        case 1:
            self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_24;
            break;
        case 2:
            self->recinfo.format = self->recinfo.format | SF_FORMAT_PCM_32;
            break;
        case 3:
            self->recinfo.format = self->recinfo.format | SF_FORMAT_FLOAT;
            break;
        case 4:
            self->recinfo.format = self->recinfo.format | SF_FORMAT_DOUBLE;
            break;
    }
    
    /* Open the output file. */
    if (! (self->recfile = sf_open(self->recpath, SFM_WRITE, &self->recinfo))) {   
        printf ("Not able to open output file %s.\n", self->recpath);
    }	

    buflen = self->bufsize * self->chnls * self->buffering;
    self->buffer = (MYFLT *)realloc(self->buffer, buflen * sizeof(MYFLT));
    for (i=0; i<buflen; i++) {
        self->buffer[i] = 0.;
    }    
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Record_getServer(Record* self) { GET_SERVER };
static PyObject * Record_getStream(Record* self) { GET_STREAM };

static PyObject * Record_play(Record *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Record_stop(Record *self) 
{ 
    sf_close(self->recfile);
    STOP
};

static PyMemberDef Record_members[] = {
{"server", T_OBJECT_EX, offsetof(Record, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Record, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Record, input_list), 0, "Input sound base object list."},
{NULL}  /* Sentinel */
};

static PyMethodDef Record_methods[] = {
{"getServer", (PyCFunction)Record_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Record_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Record_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Record_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)Record_stop, METH_NOARGS, "Stops computing."},
{NULL}  /* Sentinel */
};

PyTypeObject RecordType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.Record_base",                                   /*tp_name*/
sizeof(Record),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)Record_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,												/*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Record objects. Records its audio input in a file.",           /* tp_doc */
(traverseproc)Record_traverse,                  /* tp_traverse */
(inquiry)Record_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
Record_methods,                                 /* tp_methods */
Record_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)Record_init,                          /* tp_init */
0,                                              /* tp_alloc */
Record_new,                                     /* tp_new */
};

/************/
/* ControlRec */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *tmp_list;
    MYFLT dur;
    int rate;
    int modulo;
    long count;
    long time;
    long size;
    MYFLT *buffer;
} ControlRec;

static void
ControlRec_process(ControlRec *self) {
    int i;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    if (self->dur > 0.0) {
        for (i=0; i<self->bufsize; i++) {
            if ((self->time % self->modulo) == 0 && self->count < self->size) {
                self->buffer[self->count] = in[i];
                self->count++;
            }
            self->time++;
            if (self->count >= self->size)
                PyObject_CallMethod((PyObject *)self, "stop", NULL);
        }        
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            if ((self->time % self->modulo) == 0) {
                PyList_Append(self->tmp_list, PyFloat_FromDouble(in[i]));
            }
            self->time++;
        }
    }
}

static void
ControlRec_setProcMode(ControlRec *self)
{       
    self->proc_func_ptr = ControlRec_process;   
}

static void
ControlRec_compute_next_data_frame(ControlRec *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
ControlRec_traverse(ControlRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->tmp_list);
    return 0;
}

static int 
ControlRec_clear(ControlRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->tmp_list);
    return 0;
}

static void
ControlRec_dealloc(ControlRec* self)
{
    free(self->data);
    if (self->buffer != NULL)
        free(self->buffer);
    ControlRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * ControlRec_deleteStream(ControlRec *self) { DELETE_STREAM };

static PyObject *
ControlRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    ControlRec *self;
    self = (ControlRec *)type->tp_alloc(type, 0);
    
    self->dur = 0.0;
    self->rate = 1000;
    self->tmp_list = PyList_New(0);
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ControlRec_compute_next_data_frame);
    self->mode_func_ptr = ControlRec_setProcMode;

    return (PyObject *)self;
}

static int
ControlRec_init(ControlRec *self, PyObject *args, PyObject *kwds)
{
    long i;
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "rate", "dur", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_IF, kwlist, &inputtmp, &self->rate, &self->dur))
        return -1; 
    
    INIT_INPUT_STREAM

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    if (self->dur > 0.0) {
        self->size = (long)(self->dur * self->rate + 1);
        self->buffer = (MYFLT *)realloc(self->buffer, self->size * sizeof(MYFLT));
        for (i=0; i<self->size; i++) {
            self->buffer[i] = 0.0;
        }        
    }    
    self->modulo = (int)(self->sr / self->rate);
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * ControlRec_getServer(ControlRec* self) { GET_SERVER };
static PyObject * ControlRec_getStream(ControlRec* self) { GET_STREAM };

static PyObject * ControlRec_play(ControlRec *self, PyObject *args, PyObject *kwds) { 
    self->count = self->time = 0;
    PLAY 
};

static PyObject * ControlRec_stop(ControlRec *self) { STOP };

static PyObject *
ControlRec_getData(ControlRec *self) {
    int i;
    PyObject *data, *point;
    MYFLT time, timescl = 1.0 / self->rate;
        
    if (self->dur > 0.0) {
        data = PyList_New(self->size);
        for (i=0; i<self->size; i++) {
            time = i * timescl;
            point = PyTuple_New(2);
            PyTuple_SET_ITEM(point, 0, PyFloat_FromDouble(time));
            PyTuple_SET_ITEM(point, 1, PyFloat_FromDouble(self->buffer[i]));
            PyList_SetItem(data, i, point);
        }        
    }
    else {
        Py_ssize_t size = PyList_Size(self->tmp_list);
        data = PyList_New(size);
        for (i=0; i<size; i++) {
            time = i * timescl;
            point = PyTuple_New(2);
            PyTuple_SET_ITEM(point, 0, PyFloat_FromDouble(time));
            PyTuple_SET_ITEM(point, 1, PyList_GET_ITEM(self->tmp_list, i));
            PyList_SetItem(data, i, point);
        }        
    }
	return data;
}

static PyMemberDef ControlRec_members[] = {
    {"server", T_OBJECT_EX, offsetof(ControlRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ControlRec, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(ControlRec, input), 0, "Input sound."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlRec_methods[] = {
    {"getServer", (PyCFunction)ControlRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ControlRec_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)ControlRec_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)ControlRec_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)ControlRec_stop, METH_NOARGS, "Stops computing."},
    {"getData", (PyCFunction)ControlRec_getData, METH_NOARGS, "Returns list of sampled points."},
    {NULL}  /* Sentinel */
};

PyTypeObject ControlRecType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.ControlRec_base",                                   /*tp_name*/
    sizeof(ControlRec),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)ControlRec_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    0,												/*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "ControlRec objects. Records control signal with user-defined sampling rate.",           /* tp_doc */
    (traverseproc)ControlRec_traverse,                  /* tp_traverse */
    (inquiry)ControlRec_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    ControlRec_methods,                                 /* tp_methods */
    ControlRec_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)ControlRec_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    ControlRec_new,                                     /* tp_new */
};

/**************/
/* ControlRead object */
/**************/
typedef struct {
    pyo_audio_HEAD
    MYFLT *values;
    int rate;
    int modulo;
    int loop;
    int go;
    int modebuffer[2];
    long count;
    long time;
    long size;
    MYFLT *trigsBuffer;
    MYFLT *tempTrigsBuffer;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
} ControlRead;

static void
ControlRead_readframes_i(ControlRead *self) {
    MYFLT fpart;
    long i, mod;
    MYFLT invmodulo = 1.0 / self->modulo;
    
    if (self->go == 0)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
    
    for (i=0; i<self->bufsize; i++) {
        if (self->go == 1) {
            mod = self->time % self->modulo;
            fpart = mod * invmodulo;
            self->data[i] = (*self->interp_func_ptr)(self->values, (int)self->count, fpart, (int)self->size);            
        }
        else {
            mod = -1;
            self->data[i] = 0.0;
        }

        if (mod == 0) {
            self->count++;
            if (self->count >= self->size) {
                self->trigsBuffer[i] = 1.0;
                if (self->loop == 1)
                    self->count = 0;
                else
                    self->go = 0;
            }
        }
        self->time++;
    }
}

static void ControlRead_postprocessing_ii(ControlRead *self) { POST_PROCESSING_II };
static void ControlRead_postprocessing_ai(ControlRead *self) { POST_PROCESSING_AI };
static void ControlRead_postprocessing_ia(ControlRead *self) { POST_PROCESSING_IA };
static void ControlRead_postprocessing_aa(ControlRead *self) { POST_PROCESSING_AA };
static void ControlRead_postprocessing_ireva(ControlRead *self) { POST_PROCESSING_IREVA };
static void ControlRead_postprocessing_areva(ControlRead *self) { POST_PROCESSING_AREVA };
static void ControlRead_postprocessing_revai(ControlRead *self) { POST_PROCESSING_REVAI };
static void ControlRead_postprocessing_revaa(ControlRead *self) { POST_PROCESSING_REVAA };
static void ControlRead_postprocessing_revareva(ControlRead *self) { POST_PROCESSING_REVAREVA };

static void
ControlRead_setProcMode(ControlRead *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = ControlRead_readframes_i;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = ControlRead_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = ControlRead_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = ControlRead_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = ControlRead_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = ControlRead_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = ControlRead_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = ControlRead_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = ControlRead_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = ControlRead_postprocessing_revareva;
            break;
    } 
}

static void
ControlRead_compute_next_data_frame(ControlRead *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
ControlRead_traverse(ControlRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
ControlRead_clear(ControlRead *self)
{
    pyo_CLEAR
    return 0;
}

static void
ControlRead_dealloc(ControlRead* self)
{
    free(self->data);
    free(self->values);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    ControlRead_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * ControlRead_deleteStream(ControlRead *self) { DELETE_STREAM };

static PyObject *
ControlRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    ControlRead *self;
    self = (ControlRead *)type->tp_alloc(type, 0);
    
    self->loop = 0;
    self->rate = 1000;
    self->interp = 2;
    self->go = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ControlRead_compute_next_data_frame);
    self->mode_func_ptr = ControlRead_setProcMode;
    
    return (PyObject *)self;
}

static int
ControlRead_init(ControlRead *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuestmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"values", "rate", "loop", "interp", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiOO", kwlist, &valuestmp, &self->rate, &self->loop, &self->interp, &multmp, &addtmp))
        return -1; 

    if (valuestmp) {
        PyObject_CallMethod((PyObject *)self, "setValues", "O", valuestmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
        
    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->tempTrigsBuffer = (MYFLT *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(MYFLT));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    self->modulo = (int)(self->sr / self->rate);

    (*self->mode_func_ptr)(self);
    
    SET_INTERP_POINTER

    Py_INCREF(self);
    return 0;
}

static PyObject * ControlRead_getServer(ControlRead* self) { GET_SERVER };
static PyObject * ControlRead_getStream(ControlRead* self) { GET_STREAM };
static PyObject * ControlRead_setMul(ControlRead *self, PyObject *arg) { SET_MUL };	
static PyObject * ControlRead_setAdd(ControlRead *self, PyObject *arg) { SET_ADD };	
static PyObject * ControlRead_setSub(ControlRead *self, PyObject *arg) { SET_SUB };	
static PyObject * ControlRead_setDiv(ControlRead *self, PyObject *arg) { SET_DIV };	

static PyObject * ControlRead_play(ControlRead *self, PyObject *args, PyObject *kwds) 
{ 
    self->count = self->time = 0;
    self->go = 1;
    PLAY 
};

static PyObject * ControlRead_stop(ControlRead *self) 
{ 
    self->go = 0;
    STOP 
};

static PyObject * ControlRead_multiply(ControlRead *self, PyObject *arg) { MULTIPLY };
static PyObject * ControlRead_inplace_multiply(ControlRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ControlRead_add(ControlRead *self, PyObject *arg) { ADD };
static PyObject * ControlRead_inplace_add(ControlRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ControlRead_sub(ControlRead *self, PyObject *arg) { SUB };
static PyObject * ControlRead_inplace_sub(ControlRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ControlRead_div(ControlRead *self, PyObject *arg) { DIV };
static PyObject * ControlRead_inplace_div(ControlRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
ControlRead_setValues(ControlRead *self, PyObject *arg)
{
    Py_ssize_t i;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->size = PyList_Size(arg);
    self->values = (MYFLT *)realloc(self->values, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++) {
        self->values[i] = PyFloat_AS_DOUBLE(PyList_GET_ITEM(arg, i));
    }
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
ControlRead_setRate(ControlRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->rate = PyInt_AsLong(arg);
    self->modulo = (int)(self->sr / self->rate);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ControlRead_setLoop(ControlRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->loop = PyInt_AsLong(arg);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
ControlRead_setInterp(ControlRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isNumber = PyNumber_Check(arg);
    
	if (isNumber == 1) {
		self->interp = PyInt_AsLong(PyNumber_Int(arg));
    }  
    
    SET_INTERP_POINTER
    
    Py_INCREF(Py_None);
    return Py_None;
}

MYFLT *
ControlRead_getTrigsBuffer(ControlRead *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (MYFLT *)self->tempTrigsBuffer;
}    

static PyMemberDef ControlRead_members[] = {
    {"server", T_OBJECT_EX, offsetof(ControlRead, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ControlRead, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(ControlRead, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(ControlRead, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlRead_methods[] = {
    {"getServer", (PyCFunction)ControlRead_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ControlRead_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)ControlRead_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)ControlRead_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)ControlRead_stop, METH_NOARGS, "Stops computing."},
    {"setValues", (PyCFunction)ControlRead_setValues, METH_O, "Fill buffer with values in input."},
    {"setRate", (PyCFunction)ControlRead_setRate, METH_O, "Sets reading rate."},
    {"setLoop", (PyCFunction)ControlRead_setLoop, METH_O, "Sets the looping mode."},
    {"setInterp", (PyCFunction)ControlRead_setInterp, METH_O, "Sets reader interpolation mode."},
    {"setMul", (PyCFunction)ControlRead_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)ControlRead_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)ControlRead_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)ControlRead_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods ControlRead_as_number = {
    (binaryfunc)ControlRead_add,                      /*nb_add*/
    (binaryfunc)ControlRead_sub,                 /*nb_subtract*/
    (binaryfunc)ControlRead_multiply,                 /*nb_multiply*/
    (binaryfunc)ControlRead_div,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)ControlRead_inplace_add,              /*inplace_add*/
    (binaryfunc)ControlRead_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)ControlRead_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)ControlRead_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject ControlReadType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.ControlRead_base",         /*tp_name*/
    sizeof(ControlRead),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ControlRead_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &ControlRead_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "ControlRead objects. Generates an oscillatory waveform.",           /* tp_doc */
    (traverseproc)ControlRead_traverse,   /* tp_traverse */
    (inquiry)ControlRead_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    ControlRead_methods,             /* tp_methods */
    ControlRead_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ControlRead_init,      /* tp_init */
    0,                         /* tp_alloc */
    ControlRead_new,                 /* tp_new */
};

/************************************************************************************************/
/* ControlRead trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    ControlRead *mainReader;
    int modebuffer[2];
} ControlReadTrig;

static void ControlReadTrig_postprocessing_ii(ControlReadTrig *self) { POST_PROCESSING_II };
static void ControlReadTrig_postprocessing_ai(ControlReadTrig *self) { POST_PROCESSING_AI };
static void ControlReadTrig_postprocessing_ia(ControlReadTrig *self) { POST_PROCESSING_IA };
static void ControlReadTrig_postprocessing_aa(ControlReadTrig *self) { POST_PROCESSING_AA };
static void ControlReadTrig_postprocessing_ireva(ControlReadTrig *self) { POST_PROCESSING_IREVA };
static void ControlReadTrig_postprocessing_areva(ControlReadTrig *self) { POST_PROCESSING_AREVA };
static void ControlReadTrig_postprocessing_revai(ControlReadTrig *self) { POST_PROCESSING_REVAI };
static void ControlReadTrig_postprocessing_revaa(ControlReadTrig *self) { POST_PROCESSING_REVAA };
static void ControlReadTrig_postprocessing_revareva(ControlReadTrig *self) { POST_PROCESSING_REVAREVA };

static void
ControlReadTrig_setProcMode(ControlReadTrig *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = ControlReadTrig_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = ControlReadTrig_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = ControlReadTrig_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = ControlReadTrig_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = ControlReadTrig_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = ControlReadTrig_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = ControlReadTrig_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = ControlReadTrig_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = ControlReadTrig_postprocessing_revareva;
            break;
    }  
}

static void
ControlReadTrig_compute_next_data_frame(ControlReadTrig *self)
{
    int i;
    MYFLT *tmp;
    tmp = ControlRead_getTrigsBuffer((ControlRead *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }    
    (*self->muladd_func_ptr)(self);
}

static int
ControlReadTrig_traverse(ControlReadTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
ControlReadTrig_clear(ControlReadTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
ControlReadTrig_dealloc(ControlReadTrig* self)
{
    free(self->data);
    ControlReadTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * ControlReadTrig_deleteStream(ControlReadTrig *self) { DELETE_STREAM };

static PyObject *
ControlReadTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    ControlReadTrig *self;
    self = (ControlReadTrig *)type->tp_alloc(type, 0);
    
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, ControlReadTrig_compute_next_data_frame);
    self->mode_func_ptr = ControlReadTrig_setProcMode;
    
    return (PyObject *)self;
}

static int
ControlReadTrig_init(ControlReadTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (ControlRead *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * ControlReadTrig_getServer(ControlReadTrig* self) { GET_SERVER };
static PyObject * ControlReadTrig_getStream(ControlReadTrig* self) { GET_STREAM };
static PyObject * ControlReadTrig_setMul(ControlReadTrig *self, PyObject *arg) { SET_MUL };	
static PyObject * ControlReadTrig_setAdd(ControlReadTrig *self, PyObject *arg) { SET_ADD };	
static PyObject * ControlReadTrig_setSub(ControlReadTrig *self, PyObject *arg) { SET_SUB };	
static PyObject * ControlReadTrig_setDiv(ControlReadTrig *self, PyObject *arg) { SET_DIV };	

static PyObject * ControlReadTrig_play(ControlReadTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * ControlReadTrig_stop(ControlReadTrig *self) { STOP };

static PyObject * ControlReadTrig_multiply(ControlReadTrig *self, PyObject *arg) { MULTIPLY };
static PyObject * ControlReadTrig_inplace_multiply(ControlReadTrig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * ControlReadTrig_add(ControlReadTrig *self, PyObject *arg) { ADD };
static PyObject * ControlReadTrig_inplace_add(ControlReadTrig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * ControlReadTrig_sub(ControlReadTrig *self, PyObject *arg) { SUB };
static PyObject * ControlReadTrig_inplace_sub(ControlReadTrig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * ControlReadTrig_div(ControlReadTrig *self, PyObject *arg) { DIV };
static PyObject * ControlReadTrig_inplace_div(ControlReadTrig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef ControlReadTrig_members[] = {
    {"server", T_OBJECT_EX, offsetof(ControlReadTrig, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(ControlReadTrig, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(ControlReadTrig, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(ControlReadTrig, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef ControlReadTrig_methods[] = {
    {"getServer", (PyCFunction)ControlReadTrig_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)ControlReadTrig_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)ControlReadTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)ControlReadTrig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)ControlReadTrig_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)ControlReadTrig_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)ControlReadTrig_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)ControlReadTrig_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)ControlReadTrig_setDiv, METH_O, "Sets inverse mul factor."},        
    {NULL}  /* Sentinel */
};

static PyNumberMethods ControlReadTrig_as_number = {
    (binaryfunc)ControlReadTrig_add,                         /*nb_add*/
    (binaryfunc)ControlReadTrig_sub,                         /*nb_subtract*/
    (binaryfunc)ControlReadTrig_multiply,                    /*nb_multiply*/
    (binaryfunc)ControlReadTrig_div,                                              /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)ControlReadTrig_inplace_add,                 /*inplace_add*/
    (binaryfunc)ControlReadTrig_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)ControlReadTrig_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)ControlReadTrig_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject ControlReadTrigType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.ControlReadTrig_base",         /*tp_name*/
    sizeof(ControlReadTrig),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)ControlReadTrig_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &ControlReadTrig_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "ControlReadTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
    (traverseproc)ControlReadTrig_traverse,   /* tp_traverse */
    (inquiry)ControlReadTrig_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    ControlReadTrig_methods,             /* tp_methods */
    ControlReadTrig_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)ControlReadTrig_init,      /* tp_init */
    0,                         /* tp_alloc */
    ControlReadTrig_new,                 /* tp_new */
};

/************/
/* NoteinRec */
/************/
typedef struct {
    pyo_audio_HEAD
    PyObject *inputp;
    Stream *inputp_stream;
    PyObject *inputv;
    Stream *inputv_stream;
    PyObject *tmp_list_p;
    PyObject *tmp_list_v;
    PyObject *tmp_list_t;
    MYFLT last_pitch;
    MYFLT last_vel;
    long time;
} NoteinRec;

static void
NoteinRec_process(NoteinRec *self) {
    int i;
    MYFLT pit, vel;
    
    MYFLT *inp = Stream_getData((Stream *)self->inputp_stream);
    MYFLT *inv = Stream_getData((Stream *)self->inputv_stream);
    
    for (i=0; i<self->bufsize; i++) {
        pit = inp[i];
        vel = inv[i];
        if (pit != self->last_pitch || vel != self->last_vel) {
            self->last_pitch = pit;
            self->last_vel = vel;
            PyList_Append(self->tmp_list_p, PyFloat_FromDouble(pit));
            PyList_Append(self->tmp_list_v, PyFloat_FromDouble(vel));
            PyList_Append(self->tmp_list_t, PyFloat_FromDouble( (float)self->time / self->sr) );
        }
        self->time++;
    }
}

static void
NoteinRec_setProcMode(NoteinRec *self)
{       
    self->proc_func_ptr = NoteinRec_process;   
}

static void
NoteinRec_compute_next_data_frame(NoteinRec *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
NoteinRec_traverse(NoteinRec *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inputp);
    Py_VISIT(self->inputp_stream);
    Py_VISIT(self->inputv);
    Py_VISIT(self->inputv_stream);
    Py_VISIT(self->tmp_list_p);
    Py_VISIT(self->tmp_list_v);
    Py_VISIT(self->tmp_list_t);
    return 0;
}

static int 
NoteinRec_clear(NoteinRec *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inputp);
    Py_CLEAR(self->inputp_stream);
    Py_CLEAR(self->inputv);
    Py_CLEAR(self->inputv_stream);
    Py_CLEAR(self->tmp_list_p);
    Py_CLEAR(self->tmp_list_v);
    Py_CLEAR(self->tmp_list_t);
    return 0;
}

static void
NoteinRec_dealloc(NoteinRec* self)
{
    free(self->data);
    NoteinRec_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * NoteinRec_deleteStream(NoteinRec *self) { DELETE_STREAM };

static PyObject *
NoteinRec_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    NoteinRec *self;
    self = (NoteinRec *)type->tp_alloc(type, 0);
    
    self->tmp_list_p = PyList_New(0);
    self->tmp_list_v = PyList_New(0);
    self->tmp_list_t = PyList_New(0);
    self->last_pitch = self->last_vel = 0.0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, NoteinRec_compute_next_data_frame);
    self->mode_func_ptr = NoteinRec_setProcMode;
    
    return (PyObject *)self;
}

static int
NoteinRec_init(NoteinRec *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputptmp, *inputp_streamtmp, *inputvtmp, *inputv_streamtmp;
    
    static char *kwlist[] = {"inputp", "inputv", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO", kwlist, &inputptmp, &inputvtmp))
        return -1; 
    
    Py_XDECREF(self->inputp);
    self->inputp = inputptmp;
    inputp_streamtmp = PyObject_CallMethod((PyObject *)self->inputp, "_getStream", NULL);
    Py_INCREF(inputp_streamtmp);
    Py_XDECREF(self->inputp_stream);
    self->inputp_stream = (Stream *)inputp_streamtmp;

    Py_XDECREF(self->inputv);
    self->inputv = inputvtmp;
    inputv_streamtmp = PyObject_CallMethod((PyObject *)self->inputv, "_getStream", NULL);
    Py_INCREF(inputv_streamtmp);
    Py_XDECREF(self->inputv_stream);
    self->inputv_stream = (Stream *)inputv_streamtmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * NoteinRec_getServer(NoteinRec* self) { GET_SERVER };
static PyObject * NoteinRec_getStream(NoteinRec* self) { GET_STREAM };

static PyObject * NoteinRec_play(NoteinRec *self, PyObject *args, PyObject *kwds) { 
    self->time = 0;
    PLAY 
};

static PyObject * NoteinRec_stop(NoteinRec *self) { STOP };

static PyObject *
NoteinRec_getData(NoteinRec *self) {
    int i;
    PyObject *data, *point;
    
    Py_ssize_t size = PyList_Size(self->tmp_list_p);
    data = PyList_New(size);
    
    for (i=0; i<size; i++) {
        point = PyTuple_New(3);
        PyTuple_SET_ITEM(point, 0, PyList_GET_ITEM(self->tmp_list_t, i));
        PyTuple_SET_ITEM(point, 1, PyList_GET_ITEM(self->tmp_list_p, i));
        PyTuple_SET_ITEM(point, 2, PyList_GET_ITEM(self->tmp_list_v, i));
        PyList_SetItem(data, i, point);
    }
	return data;
}

static PyMemberDef NoteinRec_members[] = {
    {"server", T_OBJECT_EX, offsetof(NoteinRec, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NoteinRec, stream), 0, "Stream object."},
    {"inputp", T_OBJECT_EX, offsetof(NoteinRec, inputp), 0, "Pitch input."},
    {"inputv", T_OBJECT_EX, offsetof(NoteinRec, inputv), 0, "Velocity input."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinRec_methods[] = {
    {"getServer", (PyCFunction)NoteinRec_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NoteinRec_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)NoteinRec_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)NoteinRec_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NoteinRec_stop, METH_NOARGS, "Stops computing."},
    {"getData", (PyCFunction)NoteinRec_getData, METH_NOARGS, "Returns list of sampled points."},
    {NULL}  /* Sentinel */
};

PyTypeObject NoteinRecType = {
    PyObject_HEAD_INIT(NULL)
    0,                                              /*ob_size*/
    "_pyo.NoteinRec_base",                                   /*tp_name*/
    sizeof(NoteinRec),                                 /*tp_basicsize*/
    0,                                              /*tp_itemsize*/
    (destructor)NoteinRec_dealloc,                     /*tp_dealloc*/
    0,                                              /*tp_print*/
    0,                                              /*tp_getattr*/
    0,                                              /*tp_setattr*/
    0,                                              /*tp_compare*/
    0,                                              /*tp_repr*/
    0,												/*tp_as_number*/
    0,                                              /*tp_as_sequence*/
    0,                                              /*tp_as_mapping*/
    0,                                              /*tp_hash */
    0,                                              /*tp_call*/
    0,                                              /*tp_str*/
    0,                                              /*tp_getattro*/
    0,                                              /*tp_setattro*/
    0,                                              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "NoteinRec objects. Records Notein signal with user-defined sampling rate.",           /* tp_doc */
    (traverseproc)NoteinRec_traverse,                  /* tp_traverse */
    (inquiry)NoteinRec_clear,                          /* tp_clear */
    0,                                              /* tp_richcompare */
    0,                                              /* tp_weaklistoffset */
    0,                                              /* tp_iter */
    0,                                              /* tp_iternext */
    NoteinRec_methods,                                 /* tp_methods */
    NoteinRec_members,                                 /* tp_members */
    0,                                              /* tp_getset */
    0,                                              /* tp_base */
    0,                                              /* tp_dict */
    0,                                              /* tp_descr_get */
    0,                                              /* tp_descr_set */
    0,                                              /* tp_dictoffset */
    (initproc)NoteinRec_init,                          /* tp_init */
    0,                                              /* tp_alloc */
    NoteinRec_new,                                     /* tp_new */
};

/**************/
/* NoteinRead object */
/**************/
typedef struct {
    pyo_audio_HEAD
    MYFLT *values;
    long *timestamps;
    MYFLT value;
    int loop;
    int go;
    int modebuffer[2];
    long count;
    long time;
    long size;
    MYFLT *trigsBuffer;
    MYFLT *tempTrigsBuffer;
} NoteinRead;

static void
NoteinRead_readframes_i(NoteinRead *self) {
    long i;
    
    if (self->go == 0)
        PyObject_CallMethod((PyObject *)self, "stop", NULL);
    
    for (i=0; i<self->bufsize; i++) {
        if (self->go == 1) {
            if (self->time >= self->timestamps[self->count]) {
                self->value = self->values[self->count];
                self->data[i] = self->value;
                self->count++;            
            }
            else
                self->data[i] = self->value;
        }
        else
            self->data[i] = 0.0;

        if (self->count >= self->size) {
            self->trigsBuffer[i] = 1.0;
            if (self->loop == 1)
                self->time = self->count = 0;
            else
                self->go = 0;
        }
        self->time++;
    }
}

static void NoteinRead_postprocessing_ii(NoteinRead *self) { POST_PROCESSING_II };
static void NoteinRead_postprocessing_ai(NoteinRead *self) { POST_PROCESSING_AI };
static void NoteinRead_postprocessing_ia(NoteinRead *self) { POST_PROCESSING_IA };
static void NoteinRead_postprocessing_aa(NoteinRead *self) { POST_PROCESSING_AA };
static void NoteinRead_postprocessing_ireva(NoteinRead *self) { POST_PROCESSING_IREVA };
static void NoteinRead_postprocessing_areva(NoteinRead *self) { POST_PROCESSING_AREVA };
static void NoteinRead_postprocessing_revai(NoteinRead *self) { POST_PROCESSING_REVAI };
static void NoteinRead_postprocessing_revaa(NoteinRead *self) { POST_PROCESSING_REVAA };
static void NoteinRead_postprocessing_revareva(NoteinRead *self) { POST_PROCESSING_REVAREVA };

static void
NoteinRead_setProcMode(NoteinRead *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = NoteinRead_readframes_i;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = NoteinRead_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = NoteinRead_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = NoteinRead_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = NoteinRead_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = NoteinRead_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = NoteinRead_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = NoteinRead_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = NoteinRead_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = NoteinRead_postprocessing_revareva;
            break;
    } 
}

static void
NoteinRead_compute_next_data_frame(NoteinRead *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
NoteinRead_traverse(NoteinRead *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
NoteinRead_clear(NoteinRead *self)
{
    pyo_CLEAR
    return 0;
}

static void
NoteinRead_dealloc(NoteinRead* self)
{
    free(self->data);
    free(self->values);
    free(self->timestamps);
    free(self->tempTrigsBuffer);
    free(self->trigsBuffer);
    NoteinRead_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * NoteinRead_deleteStream(NoteinRead *self) { DELETE_STREAM };

static PyObject *
NoteinRead_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    NoteinRead *self;
    self = (NoteinRead *)type->tp_alloc(type, 0);
    
    self->value = 0.0;
    self->loop = 0;
    self->go = 1;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, NoteinRead_compute_next_data_frame);
    self->mode_func_ptr = NoteinRead_setProcMode;
    
    return (PyObject *)self;
}

static int
NoteinRead_init(NoteinRead *self, PyObject *args, PyObject *kwds)
{
    int i;
    PyObject *valuestmp, *timestampstmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"values", "timestamps", "loop", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iOO", kwlist, &valuestmp, &timestampstmp, &self->loop, &multmp, &addtmp))
        return -1; 

    if (valuestmp) {
        PyObject_CallMethod((PyObject *)self, "setValues", "O", valuestmp);
    }
    
    if (timestampstmp) {
        PyObject_CallMethod((PyObject *)self, "setTimestamps", "O", timestampstmp);
    }

     if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
        
    self->trigsBuffer = (MYFLT *)realloc(self->trigsBuffer, self->bufsize * sizeof(MYFLT));
    self->tempTrigsBuffer = (MYFLT *)realloc(self->tempTrigsBuffer, self->bufsize * sizeof(MYFLT));
    
    for (i=0; i<self->bufsize; i++) {
        self->trigsBuffer[i] = 0.0;
    }    
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * NoteinRead_getServer(NoteinRead* self) { GET_SERVER };
static PyObject * NoteinRead_getStream(NoteinRead* self) { GET_STREAM };
static PyObject * NoteinRead_setMul(NoteinRead *self, PyObject *arg) { SET_MUL };	
static PyObject * NoteinRead_setAdd(NoteinRead *self, PyObject *arg) { SET_ADD };	
static PyObject * NoteinRead_setSub(NoteinRead *self, PyObject *arg) { SET_SUB };	
static PyObject * NoteinRead_setDiv(NoteinRead *self, PyObject *arg) { SET_DIV };	

static PyObject * NoteinRead_play(NoteinRead *self, PyObject *args, PyObject *kwds) 
{ 
    self->count = self->time = 0;
    self->go = 1;
    PLAY 
};

static PyObject * NoteinRead_stop(NoteinRead *self) 
{ 
    self->go = 0;
    STOP 
};

static PyObject * NoteinRead_multiply(NoteinRead *self, PyObject *arg) { MULTIPLY };
static PyObject * NoteinRead_inplace_multiply(NoteinRead *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * NoteinRead_add(NoteinRead *self, PyObject *arg) { ADD };
static PyObject * NoteinRead_inplace_add(NoteinRead *self, PyObject *arg) { INPLACE_ADD };
static PyObject * NoteinRead_sub(NoteinRead *self, PyObject *arg) { SUB };
static PyObject * NoteinRead_inplace_sub(NoteinRead *self, PyObject *arg) { INPLACE_SUB };
static PyObject * NoteinRead_div(NoteinRead *self, PyObject *arg) { DIV };
static PyObject * NoteinRead_inplace_div(NoteinRead *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
NoteinRead_setValues(NoteinRead *self, PyObject *arg)
{
    Py_ssize_t i;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->size = PyList_Size(arg);
    self->values = (MYFLT *)realloc(self->values, self->size * sizeof(MYFLT));
    for (i=0; i<self->size; i++) {
        self->values[i] = PyFloat_AS_DOUBLE(PyList_GET_ITEM(arg, i));
    }
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
NoteinRead_setTimestamps(NoteinRead *self, PyObject *arg)
{
    Py_ssize_t i;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->size = PyList_Size(arg);
    self->timestamps = (long *)realloc(self->timestamps, self->size * sizeof(long));
    for (i=0; i<self->size; i++) {
        self->timestamps[i] = (long)(PyFloat_AS_DOUBLE(PyList_GET_ITEM(arg, i)) * self->sr);
    }
    
	Py_INCREF(Py_None);
	return Py_None;
}
    
static PyObject *
NoteinRead_setLoop(NoteinRead *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    self->loop = PyInt_AsLong(arg);
    
    Py_INCREF(Py_None);
    return Py_None;
}

MYFLT *
NoteinRead_getTrigsBuffer(NoteinRead *self)
{
    int i;
    for (i=0; i<self->bufsize; i++) {
        self->tempTrigsBuffer[i] = self->trigsBuffer[i];
        self->trigsBuffer[i] = 0.0;
    }    
    return (MYFLT *)self->tempTrigsBuffer;
}    

static PyMemberDef NoteinRead_members[] = {
    {"server", T_OBJECT_EX, offsetof(NoteinRead, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NoteinRead, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(NoteinRead, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(NoteinRead, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinRead_methods[] = {
    {"getServer", (PyCFunction)NoteinRead_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NoteinRead_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)NoteinRead_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)NoteinRead_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NoteinRead_stop, METH_NOARGS, "Stops computing."},
    {"setValues", (PyCFunction)NoteinRead_setValues, METH_O, "Fill buffer with values in input."},
    {"setTimestamps", (PyCFunction)NoteinRead_setTimestamps, METH_O, "Fill buffer with timestamps in input."},
    {"setLoop", (PyCFunction)NoteinRead_setLoop, METH_O, "Sets the looping mode."},
    {"setMul", (PyCFunction)NoteinRead_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)NoteinRead_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)NoteinRead_setSub, METH_O, "Sets oscillator inverse add factor."},
    {"setDiv", (PyCFunction)NoteinRead_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods NoteinRead_as_number = {
    (binaryfunc)NoteinRead_add,                      /*nb_add*/
    (binaryfunc)NoteinRead_sub,                 /*nb_subtract*/
    (binaryfunc)NoteinRead_multiply,                 /*nb_multiply*/
    (binaryfunc)NoteinRead_div,                   /*nb_divide*/
    0,                /*nb_remainder*/
    0,                   /*nb_divmod*/
    0,                   /*nb_power*/
    0,                  /*nb_neg*/
    0,                /*nb_pos*/
    0,                  /*(unaryfunc)array_abs,*/
    0,                    /*nb_nonzero*/
    0,                    /*nb_invert*/
    0,               /*nb_lshift*/
    0,              /*nb_rshift*/
    0,              /*nb_and*/
    0,              /*nb_xor*/
    0,               /*nb_or*/
    0,                                          /*nb_coerce*/
    0,                       /*nb_int*/
    0,                      /*nb_long*/
    0,                     /*nb_float*/
    0,                       /*nb_oct*/
    0,                       /*nb_hex*/
    (binaryfunc)NoteinRead_inplace_add,              /*inplace_add*/
    (binaryfunc)NoteinRead_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)NoteinRead_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)NoteinRead_inplace_div,           /*inplace_divide*/
    0,        /*inplace_remainder*/
    0,           /*inplace_power*/
    0,       /*inplace_lshift*/
    0,      /*inplace_rshift*/
    0,      /*inplace_and*/
    0,      /*inplace_xor*/
    0,       /*inplace_or*/
    0,             /*nb_floor_divide*/
    0,              /*nb_true_divide*/
    0,     /*nb_inplace_floor_divide*/
    0,      /*nb_inplace_true_divide*/
    0,                     /* nb_index */
};

PyTypeObject NoteinReadType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.NoteinRead_base",         /*tp_name*/
    sizeof(NoteinRead),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NoteinRead_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &NoteinRead_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "NoteinRead objects. Reads a NoteinRec file.",           /* tp_doc */
    (traverseproc)NoteinRead_traverse,   /* tp_traverse */
    (inquiry)NoteinRead_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    NoteinRead_methods,             /* tp_methods */
    NoteinRead_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)NoteinRead_init,      /* tp_init */
    0,                         /* tp_alloc */
    NoteinRead_new,                 /* tp_new */
};

/************************************************************************************************/
/* NoteinRead trig streamer */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    NoteinRead *mainReader;
    int modebuffer[2];
} NoteinReadTrig;

static void NoteinReadTrig_postprocessing_ii(NoteinReadTrig *self) { POST_PROCESSING_II };
static void NoteinReadTrig_postprocessing_ai(NoteinReadTrig *self) { POST_PROCESSING_AI };
static void NoteinReadTrig_postprocessing_ia(NoteinReadTrig *self) { POST_PROCESSING_IA };
static void NoteinReadTrig_postprocessing_aa(NoteinReadTrig *self) { POST_PROCESSING_AA };
static void NoteinReadTrig_postprocessing_ireva(NoteinReadTrig *self) { POST_PROCESSING_IREVA };
static void NoteinReadTrig_postprocessing_areva(NoteinReadTrig *self) { POST_PROCESSING_AREVA };
static void NoteinReadTrig_postprocessing_revai(NoteinReadTrig *self) { POST_PROCESSING_REVAI };
static void NoteinReadTrig_postprocessing_revaa(NoteinReadTrig *self) { POST_PROCESSING_REVAA };
static void NoteinReadTrig_postprocessing_revareva(NoteinReadTrig *self) { POST_PROCESSING_REVAREVA };

static void
NoteinReadTrig_setProcMode(NoteinReadTrig *self) {
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = NoteinReadTrig_postprocessing_revareva;
            break;
    }  
}

static void
NoteinReadTrig_compute_next_data_frame(NoteinReadTrig *self)
{
    int i;
    MYFLT *tmp;
    tmp = NoteinRead_getTrigsBuffer((NoteinRead *)self->mainReader);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i];
    }    
    (*self->muladd_func_ptr)(self);
}

static int
NoteinReadTrig_traverse(NoteinReadTrig *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainReader);
    return 0;
}

static int 
NoteinReadTrig_clear(NoteinReadTrig *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainReader);    
    return 0;
}

static void
NoteinReadTrig_dealloc(NoteinReadTrig* self)
{
    free(self->data);
    NoteinReadTrig_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * NoteinReadTrig_deleteStream(NoteinReadTrig *self) { DELETE_STREAM };

static PyObject *
NoteinReadTrig_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    NoteinReadTrig *self;
    self = (NoteinReadTrig *)type->tp_alloc(type, 0);
    
    self->modebuffer[0] = 0;
    self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, NoteinReadTrig_compute_next_data_frame);
    self->mode_func_ptr = NoteinReadTrig_setProcMode;
    
    return (PyObject *)self;
}

static int
NoteinReadTrig_init(NoteinReadTrig *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL;
    
    static char *kwlist[] = {"mainReader", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &maintmp))
        return -1; 
    
    Py_XDECREF(self->mainReader);
    Py_INCREF(maintmp);
    self->mainReader = (NoteinRead *)maintmp;
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * NoteinReadTrig_getServer(NoteinReadTrig* self) { GET_SERVER };
static PyObject * NoteinReadTrig_getStream(NoteinReadTrig* self) { GET_STREAM };
static PyObject * NoteinReadTrig_setMul(NoteinReadTrig *self, PyObject *arg) { SET_MUL };	
static PyObject * NoteinReadTrig_setAdd(NoteinReadTrig *self, PyObject *arg) { SET_ADD };	
static PyObject * NoteinReadTrig_setSub(NoteinReadTrig *self, PyObject *arg) { SET_SUB };	
static PyObject * NoteinReadTrig_setDiv(NoteinReadTrig *self, PyObject *arg) { SET_DIV };	

static PyObject * NoteinReadTrig_play(NoteinReadTrig *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * NoteinReadTrig_stop(NoteinReadTrig *self) { STOP };

static PyObject * NoteinReadTrig_multiply(NoteinReadTrig *self, PyObject *arg) { MULTIPLY };
static PyObject * NoteinReadTrig_inplace_multiply(NoteinReadTrig *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * NoteinReadTrig_add(NoteinReadTrig *self, PyObject *arg) { ADD };
static PyObject * NoteinReadTrig_inplace_add(NoteinReadTrig *self, PyObject *arg) { INPLACE_ADD };
static PyObject * NoteinReadTrig_sub(NoteinReadTrig *self, PyObject *arg) { SUB };
static PyObject * NoteinReadTrig_inplace_sub(NoteinReadTrig *self, PyObject *arg) { INPLACE_SUB };
static PyObject * NoteinReadTrig_div(NoteinReadTrig *self, PyObject *arg) { DIV };
static PyObject * NoteinReadTrig_inplace_div(NoteinReadTrig *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef NoteinReadTrig_members[] = {
    {"server", T_OBJECT_EX, offsetof(NoteinReadTrig, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(NoteinReadTrig, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(NoteinReadTrig, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(NoteinReadTrig, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef NoteinReadTrig_methods[] = {
    {"getServer", (PyCFunction)NoteinReadTrig_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)NoteinReadTrig_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)NoteinReadTrig_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)NoteinReadTrig_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)NoteinReadTrig_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)NoteinReadTrig_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)NoteinReadTrig_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)NoteinReadTrig_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)NoteinReadTrig_setDiv, METH_O, "Sets inverse mul factor."},        
    {NULL}  /* Sentinel */
};

static PyNumberMethods NoteinReadTrig_as_number = {
    (binaryfunc)NoteinReadTrig_add,                         /*nb_add*/
    (binaryfunc)NoteinReadTrig_sub,                         /*nb_subtract*/
    (binaryfunc)NoteinReadTrig_multiply,                    /*nb_multiply*/
    (binaryfunc)NoteinReadTrig_div,                                              /*nb_divide*/
    0,                                              /*nb_remainder*/
    0,                                              /*nb_divmod*/
    0,                                              /*nb_power*/
    0,                                              /*nb_neg*/
    0,                                              /*nb_pos*/
    0,                                              /*(unaryfunc)array_abs,*/
    0,                                              /*nb_nonzero*/
    0,                                              /*nb_invert*/
    0,                                              /*nb_lshift*/
    0,                                              /*nb_rshift*/
    0,                                              /*nb_and*/
    0,                                              /*nb_xor*/
    0,                                              /*nb_or*/
    0,                                              /*nb_coerce*/
    0,                                              /*nb_int*/
    0,                                              /*nb_long*/
    0,                                              /*nb_float*/
    0,                                              /*nb_oct*/
    0,                                              /*nb_hex*/
    (binaryfunc)NoteinReadTrig_inplace_add,                 /*inplace_add*/
    (binaryfunc)NoteinReadTrig_inplace_sub,                 /*inplace_subtract*/
    (binaryfunc)NoteinReadTrig_inplace_multiply,            /*inplace_multiply*/
    (binaryfunc)NoteinReadTrig_inplace_div,                                              /*inplace_divide*/
    0,                                              /*inplace_remainder*/
    0,                                              /*inplace_power*/
    0,                                              /*inplace_lshift*/
    0,                                              /*inplace_rshift*/
    0,                                              /*inplace_and*/
    0,                                              /*inplace_xor*/
    0,                                              /*inplace_or*/
    0,                                              /*nb_floor_divide*/
    0,                                              /*nb_true_divide*/
    0,                                              /*nb_inplace_floor_divide*/
    0,                                              /*nb_inplace_true_divide*/
    0,                                              /* nb_index */
};

PyTypeObject NoteinReadTrigType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.NoteinReadTrig_base",         /*tp_name*/
    sizeof(NoteinReadTrig),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)NoteinReadTrig_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &NoteinReadTrig_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "NoteinReadTrig objects. Sends trigger at the end of playback.",           /* tp_doc */
    (traverseproc)NoteinReadTrig_traverse,   /* tp_traverse */
    (inquiry)NoteinReadTrig_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    NoteinReadTrig_methods,             /* tp_methods */
    NoteinReadTrig_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)NoteinReadTrig_init,      /* tp_init */
    0,                         /* tp_alloc */
    NoteinReadTrig_new,                 /* tp_new */
};


