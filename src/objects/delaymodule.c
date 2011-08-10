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
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    PyObject *feedback;
    Stream *feedback_stream;
    MYFLT maxdelay;
    long size;
    long in_count;
    int modebuffer[4];
    MYFLT *buffer; // samples memory
} Delay;

static void
Delay_process_ii(Delay *self) {
    MYFLT val, xind, frac;
    int i;
    long ind;

    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    MYFLT sampdel = del * self->sr;

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;
        
        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ai(Delay *self) {
    MYFLT val, xind, frac, sampdel, del;
    int i;
    long ind;

    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);    
    MYFLT feed = PyFloat_AS_DOUBLE(self->feedback);

    if (feed < 0)
        feed = 0;
    else if (feed > 1)
        feed = 1;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;
        
        self->buffer[self->in_count] = in[i]  + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_ia(Delay *self) {
    MYFLT val, xind, frac, feed;
    int i;
    long ind;
    
    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);    
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    MYFLT sampdel = del * self->sr;
       
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;

        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        
        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Delay_process_aa(Delay *self) {
    MYFLT val, xind, frac, sampdel, feed, del;
    int i;
    long ind;
    
    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);    
    MYFLT *fdb = Stream_getData((Stream *)self->feedback_stream);    
  
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = del * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] * (1.0 - frac) + self->buffer[ind+1] * frac;
        self->data[i] = val;
        
        feed = fdb[i];
        if (feed < 0)
            feed = 0;
        else if (feed > 1)
            feed = 1;
        
        self->buffer[self->in_count] = in[i] + (val * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[self->in_count];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Delay_postprocessing_ii(Delay *self) { POST_PROCESSING_II };
static void Delay_postprocessing_ai(Delay *self) { POST_PROCESSING_AI };
static void Delay_postprocessing_ia(Delay *self) { POST_PROCESSING_IA };
static void Delay_postprocessing_aa(Delay *self) { POST_PROCESSING_AA };
static void Delay_postprocessing_ireva(Delay *self) { POST_PROCESSING_IREVA };
static void Delay_postprocessing_areva(Delay *self) { POST_PROCESSING_AREVA };
static void Delay_postprocessing_revai(Delay *self) { POST_PROCESSING_REVAI };
static void Delay_postprocessing_revaa(Delay *self) { POST_PROCESSING_REVAA };
static void Delay_postprocessing_revareva(Delay *self) { POST_PROCESSING_REVAREVA };

static void
Delay_setProcMode(Delay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Delay_process_ii;
            break;
        case 1:    
            self->proc_func_ptr = Delay_process_ai;
            break;
        case 10:    
            self->proc_func_ptr = Delay_process_ia;
            break;
        case 11:    
            self->proc_func_ptr = Delay_process_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Delay_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Delay_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Delay_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Delay_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Delay_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Delay_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Delay_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Delay_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Delay_postprocessing_revareva;
            break;
    } 
}

static void
Delay_compute_next_data_frame(Delay *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Delay_traverse(Delay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->delay);    
    Py_VISIT(self->delay_stream);    
    Py_VISIT(self->feedback);    
    Py_VISIT(self->feedback_stream);    
    return 0;
}

static int 
Delay_clear(Delay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->delay);    
    Py_CLEAR(self->delay_stream);    
    Py_CLEAR(self->feedback);    
    Py_CLEAR(self->feedback_stream);    
    return 0;
}

static void
Delay_dealloc(Delay* self)
{
    free(self->data);
    free(self->buffer);
    Delay_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Delay_deleteStream(Delay *self) { DELETE_STREAM };

static PyObject *
Delay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Delay *self;
    self = (Delay *)type->tp_alloc(type, 0);

    self->delay = PyFloat_FromDouble(0.25);
    self->feedback = PyFloat_FromDouble(0);
    self->maxdelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Delay_compute_next_data_frame);
    self->mode_func_ptr = Delay_setProcMode;
    
    return (PyObject *)self;
}

static int
Delay_init(Delay *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *feedbacktmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "delay", "feedback", "maxdelay", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OOFOO, kwlist, &inputtmp, &delaytmp, &feedbacktmp, &self->maxdelay, &multmp, &addtmp))
        return -1; 

    INIT_INPUT_STREAM

    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }

    if (feedbacktmp) {
        PyObject_CallMethod((PyObject *)self, "setFeedback", "O", feedbacktmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    self->size = (long)(self->maxdelay * self->sr + 0.5);

    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    

    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Delay_getServer(Delay* self) { GET_SERVER };
static PyObject * Delay_getStream(Delay* self) { GET_STREAM };
static PyObject * Delay_setMul(Delay *self, PyObject *arg) { SET_MUL };	
static PyObject * Delay_setAdd(Delay *self, PyObject *arg) { SET_ADD };	
static PyObject * Delay_setSub(Delay *self, PyObject *arg) { SET_SUB };	
static PyObject * Delay_setDiv(Delay *self, PyObject *arg) { SET_DIV };	

static PyObject * Delay_play(Delay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Delay_out(Delay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Delay_stop(Delay *self) { STOP };

static PyObject * Delay_multiply(Delay *self, PyObject *arg) { MULTIPLY };
static PyObject * Delay_inplace_multiply(Delay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Delay_add(Delay *self, PyObject *arg) { ADD };
static PyObject * Delay_inplace_add(Delay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Delay_sub(Delay *self, PyObject *arg) { SUB };
static PyObject * Delay_inplace_sub(Delay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Delay_div(Delay *self, PyObject *arg) { DIV };
static PyObject * Delay_inplace_div(Delay *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Delay_setDelay(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);

	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Delay_setFeedback(Delay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feedback);
	if (isNumber == 1) {
		self->feedback = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feedback = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feedback, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feedback_stream);
        self->feedback_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Delay_members[] = {
    {"server", T_OBJECT_EX, offsetof(Delay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Delay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Delay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(Delay, delay), 0, "Delay time in seconds."},
    {"feedback", T_OBJECT_EX, offsetof(Delay, feedback), 0, "Feedback value."},
    {"mul", T_OBJECT_EX, offsetof(Delay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Delay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Delay_methods[] = {
    {"getServer", (PyCFunction)Delay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Delay_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Delay_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Delay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Delay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Delay_stop, METH_NOARGS, "Stops computing."},
	{"setDelay", (PyCFunction)Delay_setDelay, METH_O, "Sets delay time in seconds."},
    {"setFeedback", (PyCFunction)Delay_setFeedback, METH_O, "Sets feedback value between 0 -> 1."},
	{"setMul", (PyCFunction)Delay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Delay_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Delay_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Delay_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Delay_as_number = {
    (binaryfunc)Delay_add,                      /*nb_add*/
    (binaryfunc)Delay_sub,                 /*nb_subtract*/
    (binaryfunc)Delay_multiply,                 /*nb_multiply*/
    (binaryfunc)Delay_div,                   /*nb_divide*/
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
    (binaryfunc)Delay_inplace_add,              /*inplace_add*/
    (binaryfunc)Delay_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Delay_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Delay_inplace_div,           /*inplace_divide*/
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

PyTypeObject DelayType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Delay_base",         /*tp_name*/
    sizeof(Delay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Delay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Delay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Delay objects. Delay signal by x samples.",           /* tp_doc */
    (traverseproc)Delay_traverse,   /* tp_traverse */
    (inquiry)Delay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Delay_methods,             /* tp_methods */
    Delay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Delay_init,      /* tp_init */
    0,                         /* tp_alloc */
    Delay_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *delay;
    Stream *delay_stream;
    MYFLT maxdelay;
    long size;
    long in_count;
    int modebuffer[3];
    MYFLT *buffer; // samples memory
} SDelay;

static void
SDelay_process_i(SDelay *self) {
    int i; 
    long ind;
    
    MYFLT del = PyFloat_AS_DOUBLE(self->delay);
    
    if (del < 0.)
        del = 0.;
    else if (del > self->maxdelay)
        del = self->maxdelay;
    long sampdel = (long)(del * self->sr);

    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        ind = self->in_count - sampdel;
        if (ind < 0)
            ind += (self->size-1);
        self->data[i] = self->buffer[ind];
        
        self->buffer[self->in_count] = in[i];
        self->in_count++;
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void
SDelay_process_a(SDelay *self) {
    MYFLT del;
    int i; 
    long ind, sampdel;
    
    MYFLT *delobj = Stream_getData((Stream *)self->delay_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        del = delobj[i];
        if (del < 0.)
            del = 0.;
        else if (del > self->maxdelay)
            del = self->maxdelay;
        sampdel = (long)(del * self->sr);
        ind = self->in_count - sampdel;
        if (ind < 0)
            ind += (self->size-1);
        self->data[i] = self->buffer[ind];
        
        self->buffer[self->in_count++] = in[i];
        if (self->in_count >= self->size)
            self->in_count = 0;
    }
}

static void SDelay_postprocessing_ii(SDelay *self) { POST_PROCESSING_II };
static void SDelay_postprocessing_ai(SDelay *self) { POST_PROCESSING_AI };
static void SDelay_postprocessing_ia(SDelay *self) { POST_PROCESSING_IA };
static void SDelay_postprocessing_aa(SDelay *self) { POST_PROCESSING_AA };
static void SDelay_postprocessing_ireva(SDelay *self) { POST_PROCESSING_IREVA };
static void SDelay_postprocessing_areva(SDelay *self) { POST_PROCESSING_AREVA };
static void SDelay_postprocessing_revai(SDelay *self) { POST_PROCESSING_REVAI };
static void SDelay_postprocessing_revaa(SDelay *self) { POST_PROCESSING_REVAA };
static void SDelay_postprocessing_revareva(SDelay *self) { POST_PROCESSING_REVAREVA };

static void
SDelay_setProcMode(SDelay *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = SDelay_process_i;
            break;
        case 1:    
            self->proc_func_ptr = SDelay_process_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = SDelay_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = SDelay_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = SDelay_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = SDelay_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = SDelay_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = SDelay_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = SDelay_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = SDelay_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = SDelay_postprocessing_revareva;
            break;
    } 
}

static void
SDelay_compute_next_data_frame(SDelay *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
SDelay_traverse(SDelay *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->delay);    
    Py_VISIT(self->delay_stream);    
    return 0;
}

static int 
SDelay_clear(SDelay *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->delay);    
    Py_CLEAR(self->delay_stream);    
    return 0;
}

static void
SDelay_dealloc(SDelay* self)
{
    free(self->data);
    free(self->buffer);
    SDelay_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * SDelay_deleteStream(SDelay *self) { DELETE_STREAM };

static PyObject *
SDelay_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    SDelay *self;
    self = (SDelay *)type->tp_alloc(type, 0);
    
    self->delay = PyFloat_FromDouble(0.25);
    self->maxdelay = 1;
    self->in_count = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, SDelay_compute_next_data_frame);
    self->mode_func_ptr = SDelay_setProcMode;
    
    return (PyObject *)self;
}

static int
SDelay_init(SDelay *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *delaytmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "delay", "maxdelay", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_O_OFOO, kwlist, &inputtmp, &delaytmp, &self->maxdelay, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (delaytmp) {
        PyObject_CallMethod((PyObject *)self, "setDelay", "O", delaytmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->size = (long)(self->maxdelay * self->sr + 0.5);
    
    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * SDelay_getServer(SDelay* self) { GET_SERVER };
static PyObject * SDelay_getStream(SDelay* self) { GET_STREAM };
static PyObject * SDelay_setMul(SDelay *self, PyObject *arg) { SET_MUL };	
static PyObject * SDelay_setAdd(SDelay *self, PyObject *arg) { SET_ADD };	
static PyObject * SDelay_setSub(SDelay *self, PyObject *arg) { SET_SUB };	
static PyObject * SDelay_setDiv(SDelay *self, PyObject *arg) { SET_DIV };	

static PyObject * SDelay_play(SDelay *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * SDelay_out(SDelay *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * SDelay_stop(SDelay *self) { STOP };

static PyObject * SDelay_multiply(SDelay *self, PyObject *arg) { MULTIPLY };
static PyObject * SDelay_inplace_multiply(SDelay *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * SDelay_add(SDelay *self, PyObject *arg) { ADD };
static PyObject * SDelay_inplace_add(SDelay *self, PyObject *arg) { INPLACE_ADD };
static PyObject * SDelay_sub(SDelay *self, PyObject *arg) { SUB };
static PyObject * SDelay_inplace_sub(SDelay *self, PyObject *arg) { INPLACE_SUB };
static PyObject * SDelay_div(SDelay *self, PyObject *arg) { DIV };
static PyObject * SDelay_inplace_div(SDelay *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
SDelay_setDelay(SDelay *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->delay);
	if (isNumber == 1) {
		self->delay = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->delay = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->delay, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->delay_stream);
        self->delay_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef SDelay_members[] = {
    {"server", T_OBJECT_EX, offsetof(SDelay, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(SDelay, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(SDelay, input), 0, "Input sound object."},
    {"delay", T_OBJECT_EX, offsetof(SDelay, delay), 0, "Delay time in seconds."},
    {"mul", T_OBJECT_EX, offsetof(SDelay, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(SDelay, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef SDelay_methods[] = {
    {"getServer", (PyCFunction)SDelay_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)SDelay_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)SDelay_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)SDelay_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)SDelay_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)SDelay_stop, METH_NOARGS, "Stops computing."},
	{"setDelay", (PyCFunction)SDelay_setDelay, METH_O, "Sets delay time in seconds."},
	{"setMul", (PyCFunction)SDelay_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)SDelay_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)SDelay_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)SDelay_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods SDelay_as_number = {
    (binaryfunc)SDelay_add,                      /*nb_add*/
    (binaryfunc)SDelay_sub,                 /*nb_subtract*/
    (binaryfunc)SDelay_multiply,                 /*nb_multiply*/
    (binaryfunc)SDelay_div,                   /*nb_divide*/
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
    (binaryfunc)SDelay_inplace_add,              /*inplace_add*/
    (binaryfunc)SDelay_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)SDelay_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)SDelay_inplace_div,           /*inplace_divide*/
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

PyTypeObject SDelayType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.SDelay_base",         /*tp_name*/
    sizeof(SDelay),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)SDelay_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &SDelay_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "SDelay objects. Simple Delay with no interpolation and no feedback.",           /* tp_doc */
    (traverseproc)SDelay_traverse,   /* tp_traverse */
    (inquiry)SDelay_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    SDelay_methods,             /* tp_methods */
    SDelay_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)SDelay_init,      /* tp_init */
    0,                         /* tp_alloc */
    SDelay_new,                 /* tp_new */
};

/*********************/
/***** Waveguide *****/
/*********************/
typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *dur;
    Stream *dur_stream;
    MYFLT minfreq;
    MYFLT lastFreq;
    MYFLT lastSampDel;
    MYFLT lastDur;
    MYFLT lastFeed;
    long size;
    int in_count;
    int modebuffer[4];
    MYFLT lpsamp; // lowpass sample memory
    MYFLT coeffs[5]; // lagrange coefficients
    MYFLT lagrange[4]; // lagrange samples memories
    MYFLT xn1; // dc block input delay
    MYFLT yn1; // dc block output delay
    MYFLT *buffer; // samples memory
} Waveguide;

static void
Waveguide_process_ii(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed;
    int i, ind, isamp;
    
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur); 
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    if (dur <= 0)
        dur = 0.1;
    
    
    sampdel = self->lastSampDel;
    feed = self->lastFeed;
    /* lagrange coeffs and feedback coeff */
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        sampdel = 1.0 / fr * self->sr - 0.5;
        self->lastSampDel = sampdel;
        isamp = (int)sampdel;
        frac = sampdel - isamp;
        self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
        self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
        self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
        self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
        self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
        
        self->lastDur = dur;
        feed = MYPOW(100, -(1.0/fr)/dur);
        self->lastFeed = feed;
    } 
    else if (dur != self->lastDur) {
        self->lastDur = dur;
        feed = MYPOW(100, -(1.0/fr)/dur);
        self->lastFeed = feed;
    }
    
    /* pick a new value in th delay line */
    isamp = (int)sampdel;  
    for (i=0; i<self->bufsize; i++) {
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;

        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;

        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void
Waveguide_process_ai(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, freq;
    int i, ind, isamp;
    
    MYFLT *fr =Stream_getData((Stream *)self->freq_stream);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur); 
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    /* Check dur boundary */
    if (dur <= 0)
        dur = 0.1;

    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        /* Check frequency boundary */
        if (freq < self->minfreq)
            freq = self->minfreq;

        sampdel = self->lastSampDel;
        feed = self->lastFeed;
        /* lagrange coeffs and feedback coeff */
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            sampdel = 1.0 / freq * self->sr - 0.5;
            self->lastSampDel = sampdel;
            isamp = (int)sampdel;
            frac = sampdel - isamp;
            self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
            self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
            self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
            self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
            self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
            
            self->lastDur = dur;
            feed = MYPOW(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }
        else if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = MYPOW(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }

        /* pick a new value in th delay line */
        isamp = (int)sampdel;        
        
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;
        
        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;
        
        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}


static void
Waveguide_process_ia(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, dur;
    int i, ind, isamp;
    
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *du = Stream_getData((Stream *)self->dur_stream);
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;

    sampdel = self->lastSampDel;
    /* lagrange coeffs and feedback coeff */
    if (fr != self->lastFreq) {
        self->lastFreq = fr;
        sampdel = 1.0 / fr * self->sr - 0.5;
        self->lastSampDel = sampdel;
        isamp = (int)sampdel;
        frac = sampdel - isamp;
        self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
        self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
        self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
        self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
        self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
    }
    
    /* pick a new value in th delay line */
    isamp = (int)sampdel;  
    for (i=0; i<self->bufsize; i++) {
        feed = self->lastFeed;
        dur = du[i];
        if (dur <= 0)
            dur = 0.1;
        if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = MYPOW(100, -(1.0/fr)/dur);
            self->lastFeed = feed;
        }
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;
        
        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;

        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;
        
        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}


static void
Waveguide_process_aa(Waveguide *self) {
    MYFLT val, x, y, sampdel, frac, feed, freq, dur;
    int i, ind, isamp;
    
    MYFLT *fr = Stream_getData((Stream *)self->freq_stream);
    MYFLT *du = Stream_getData((Stream *)self->dur_stream); 
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    for (i=0; i<self->bufsize; i++) {
        freq = fr[i];
        dur = du[i];
        /* Check boundaries */
        if (freq < self->minfreq)
            freq = self->minfreq;
        if (dur <= 0)
            dur = 0.1;
        
        sampdel = self->lastSampDel;
        feed = self->lastFeed;
        /* lagrange coeffs and feedback coeff */
        if (freq != self->lastFreq) {
            self->lastFreq = freq;
            sampdel = 1.0 / freq * self->sr - 0.5;
            self->lastSampDel = sampdel;
            isamp = (int)sampdel;
            frac = sampdel - isamp;
            self->coeffs[0] = (frac-1)*(frac-2)*(frac-3)*(frac-4)/24.0;
            self->coeffs[1] = -frac*(frac-2)*(frac-3)*(frac-4)/6.0;
            self->coeffs[2] = frac*(frac-1)*(frac-3)*(frac-4)/4.0;
            self->coeffs[3] = -frac*(frac-1)*(frac-2)*(frac-4)/6.0;
            self->coeffs[4] = frac*(frac-1)*(frac-2)*(frac-3)/24.0;
            
            self->lastDur = dur;
            feed = MYPOW(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }
        else if (dur != self->lastDur) {
            self->lastDur = dur;
            feed = MYPOW(100, -(1.0/freq)/dur);
            self->lastFeed = feed;
        }
        
        /* pick a new value in th delay line */
        isamp = (int)sampdel;        
        
        ind = self->in_count - isamp;
        if (ind < 0)
            ind += self->size;
        val = self->buffer[ind];
        
        /* simple lowpass filtering */
        val = (val + self->lpsamp) * 0.5;
        self->lpsamp = val;
        
        /* lagrange filtering */
        x = (val*self->coeffs[0])+(self->lagrange[0]*self->coeffs[1])+(self->lagrange[1]*self->coeffs[2])+
            (self->lagrange[2]*self->coeffs[3])+(self->lagrange[3]*self->coeffs[4]);
        self->lagrange[3] = self->lagrange[2];
        self->lagrange[2] = self->lagrange[1];
        self->lagrange[1] = self->lagrange[0];
        self->lagrange[0] = val;
  
        /* DC filtering */
        y = x - self->xn1 + 0.995 * self->yn1;
        self->xn1 = x;
        self->yn1 = y;
        
        self->data[i] = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + (x * feed);
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;
    }
}

static void Waveguide_postprocessing_ii(Waveguide *self) { POST_PROCESSING_II };
static void Waveguide_postprocessing_ai(Waveguide *self) { POST_PROCESSING_AI };
static void Waveguide_postprocessing_ia(Waveguide *self) { POST_PROCESSING_IA };
static void Waveguide_postprocessing_aa(Waveguide *self) { POST_PROCESSING_AA };
static void Waveguide_postprocessing_ireva(Waveguide *self) { POST_PROCESSING_IREVA };
static void Waveguide_postprocessing_areva(Waveguide *self) { POST_PROCESSING_AREVA };
static void Waveguide_postprocessing_revai(Waveguide *self) { POST_PROCESSING_REVAI };
static void Waveguide_postprocessing_revaa(Waveguide *self) { POST_PROCESSING_REVAA };
static void Waveguide_postprocessing_revareva(Waveguide *self) { POST_PROCESSING_REVAREVA };

static void
Waveguide_setProcMode(Waveguide *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Waveguide_process_ii;
            break;
        case 1:    
            self->proc_func_ptr = Waveguide_process_ai;
            break;
        case 10:    
            self->proc_func_ptr = Waveguide_process_ia;
            break;
        case 11:    
            self->proc_func_ptr = Waveguide_process_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Waveguide_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Waveguide_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Waveguide_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Waveguide_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Waveguide_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Waveguide_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Waveguide_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Waveguide_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Waveguide_postprocessing_revareva;
            break;
    } 
}

static void
Waveguide_compute_next_data_frame(Waveguide *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Waveguide_traverse(Waveguide *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    return 0;
}

static int 
Waveguide_clear(Waveguide *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    return 0;
}

static void
Waveguide_dealloc(Waveguide* self)
{
    free(self->data);
    free(self->buffer);
    Waveguide_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Waveguide_deleteStream(Waveguide *self) { DELETE_STREAM };

static PyObject *
Waveguide_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Waveguide *self;
    self = (Waveguide *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->dur = PyFloat_FromDouble(0.99);
    self->minfreq = 20;
    self->lastFreq = -1.0;
    self->lastSampDel = -1.0;
    self->lastDur = -1.0;
    self->lastFeed = 0.0;
    self->in_count = 0;
    self->lpsamp = 0.0;
    for(i=0; i<4; i++) {
        self->lagrange[i] = 0.0;
    }    
    self->xn1 = 0.0;
    self->yn1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Waveguide_compute_next_data_frame);
    self->mode_func_ptr = Waveguide_setProcMode;
    
    return (PyObject *)self;
}

static int
Waveguide_init(Waveguide *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i;
    
    static char *kwlist[] = {"input", "freq", "dur", "minfreq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOfOO", kwlist, &inputtmp, &freqtmp, &durtmp, &self->minfreq, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->size = (long)(1.0 / self->minfreq * self->sr + 0.5);
    
    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Waveguide_getServer(Waveguide* self) { GET_SERVER };
static PyObject * Waveguide_getStream(Waveguide* self) { GET_STREAM };
static PyObject * Waveguide_setMul(Waveguide *self, PyObject *arg) { SET_MUL };	
static PyObject * Waveguide_setAdd(Waveguide *self, PyObject *arg) { SET_ADD };	
static PyObject * Waveguide_setSub(Waveguide *self, PyObject *arg) { SET_SUB };	
static PyObject * Waveguide_setDiv(Waveguide *self, PyObject *arg) { SET_DIV };	

static PyObject * Waveguide_play(Waveguide *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Waveguide_out(Waveguide *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Waveguide_stop(Waveguide *self) { STOP };

static PyObject * Waveguide_multiply(Waveguide *self, PyObject *arg) { MULTIPLY };
static PyObject * Waveguide_inplace_multiply(Waveguide *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Waveguide_add(Waveguide *self, PyObject *arg) { ADD };
static PyObject * Waveguide_inplace_add(Waveguide *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Waveguide_sub(Waveguide *self, PyObject *arg) { SUB };
static PyObject * Waveguide_inplace_sub(Waveguide *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Waveguide_div(Waveguide *self, PyObject *arg) { DIV };
static PyObject * Waveguide_inplace_div(Waveguide *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Waveguide_setFreq(Waveguide *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Waveguide_setDur(Waveguide *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->dur);
	if (isNumber == 1) {
		self->dur = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Waveguide_members[] = {
{"server", T_OBJECT_EX, offsetof(Waveguide, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Waveguide, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Waveguide, input), 0, "Input sound object."},
{"freq", T_OBJECT_EX, offsetof(Waveguide, freq), 0, "Waveguide time in seconds."},
{"dur", T_OBJECT_EX, offsetof(Waveguide, dur), 0, "Feedback value."},
{"mul", T_OBJECT_EX, offsetof(Waveguide, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Waveguide, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Waveguide_methods[] = {
{"getServer", (PyCFunction)Waveguide_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Waveguide_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Waveguide_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Waveguide_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Waveguide_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Waveguide_stop, METH_NOARGS, "Stops computing."},
{"setFreq", (PyCFunction)Waveguide_setFreq, METH_O, "Sets freq time in seconds."},
{"setDur", (PyCFunction)Waveguide_setDur, METH_O, "Sets dur value between 0 -> 1."},
{"setMul", (PyCFunction)Waveguide_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Waveguide_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Waveguide_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Waveguide_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Waveguide_as_number = {
(binaryfunc)Waveguide_add,                      /*nb_add*/
(binaryfunc)Waveguide_sub,                 /*nb_subtract*/
(binaryfunc)Waveguide_multiply,                 /*nb_multiply*/
(binaryfunc)Waveguide_div,                   /*nb_divide*/
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
(binaryfunc)Waveguide_inplace_add,              /*inplace_add*/
(binaryfunc)Waveguide_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Waveguide_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Waveguide_inplace_div,           /*inplace_divide*/
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

PyTypeObject WaveguideType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Waveguide_base",         /*tp_name*/
sizeof(Waveguide),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Waveguide_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Waveguide_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Waveguide objects. Waveguide signal by x samples.",           /* tp_doc */
(traverseproc)Waveguide_traverse,   /* tp_traverse */
(inquiry)Waveguide_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Waveguide_methods,             /* tp_methods */
Waveguide_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Waveguide_init,      /* tp_init */
0,                         /* tp_alloc */
Waveguide_new,                 /* tp_new */
};

/*********************/
/***** AllpassWG *****/
/*********************/
static const MYFLT alp_chorus_factor[3] = {1.0, 0.9981, 0.9957};
static const MYFLT alp_feedback = 0.3;

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *freq;
    Stream *freq_stream;
    PyObject *feed;
    Stream *feed_stream;
    PyObject *detune;
    Stream *detune_stream;
    MYFLT minfreq;
    long size;
    int alpsize;
    int in_count;
    int alp_in_count[3];
    int modebuffer[5];
    MYFLT *alpbuffer[3]; // allpass samples memories
    MYFLT xn1; // dc block input delay
    MYFLT yn1; // dc block output delay
    MYFLT *buffer; // samples memory
} AllpassWG;

static void
AllpassWG_process_iii(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed); 
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);
    
    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;
    
    sampdel = 1.0 / (fr * freqshift) * self->sr;
    alpdetune = detune * self->alpsize;

    for (i=0; i<self->bufsize; i++) {
        /* pick a new value in the delay line */
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_aii(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed); 
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);
    
    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;
    
    alpdetune = detune * self->alpsize;
    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        
        /* pick a new value in the delay line */
        sampdel = 1.0 / (fr * freqshift) * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_iai(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, feed, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream); 
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);
    
    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;
    
    sampdel = 1.0 / (fr * freqshift) * self->sr;
    alpdetune = detune * self->alpsize;
    
    for (i=0; i<self->bufsize; i++) {
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        /* pick a new value in the delay line */
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_aai(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, feed, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream); 
    MYFLT detune = PyFloat_AS_DOUBLE(self->detune);
    
    freqshift = detune * 0.5 + 1.;
    detune = detune * 0.95 + 0.05;
    if (detune < 0.05)
        detune = 0.05;
    else if (detune > 1.0)
        detune = 1.0;
    
    alpdetune = detune * self->alpsize;
    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        
        /* pick a new value in the delay line */
        sampdel = 1.0 / (fr * freqshift) * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_iia(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed); 
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);
    
    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;
    
    for (i=0; i<self->bufsize; i++) {
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;
        
        /* pick a new value in the delay line */
        sampdel = 1.0 / (fr * freqshift) * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }

        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;

        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_aia(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT feed = PyFloat_AS_DOUBLE(self->feed); 
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);
    
    feed *= 0.4525;
    if (feed > 0.4525)
        feed = 0.4525;
    else if (feed < 0)
        feed = 0;
    
    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;
        
        /* pick a new value in the delay line */
        sampdel = 1.0 / (fr * freqshift) * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_iaa(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, feed, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT fr = PyFloat_AS_DOUBLE(self->freq);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream); 
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);
    
    /* Check boundaries */
    if (fr < self->minfreq)
        fr = self->minfreq;
    
    for (i=0; i<self->bufsize; i++) {
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;
        
        /* pick a new value in the delay line */
        sampdel = 1.0 / (fr * freqshift) * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void
AllpassWG_process_aaa(AllpassWG *self) {
    int i, j;
    long ind;
    MYFLT val, y, xind, sampdel, frac, fr, feed, detune, freqshift, alpsampdel, alpsampdelin, alpdetune;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *freq = Stream_getData((Stream *)self->freq_stream);
    MYFLT *fdb = Stream_getData((Stream *)self->feed_stream); 
    MYFLT *det = Stream_getData((Stream *)self->detune_stream);

    for (i=0; i<self->bufsize; i++) {
        fr = freq[i];
        if (fr < self->minfreq)
            fr = self->minfreq;
        feed = fdb[i] * 0.4525;
        if (feed > 0.4525)
            feed = 0.4525;
        else if (feed < 0)
            feed = 0;
        detune = det[i];
        freqshift = detune * 0.5 + 1.;
        detune = detune * 0.95 + 0.05;
        if (detune < 0.05)
            detune = 0.05;
        else if (detune > 1.0)
            detune = 1.0;
        
        /* pick a new value in the delay line */
        sampdel = 1.0 / (fr * freqshift) * self->sr;
        xind = self->in_count - sampdel;
        if (xind < 0)
            xind += self->size;
        ind = (long)xind;
        frac = xind - ind;
        val = self->buffer[ind] + (self->buffer[ind+1] - self->buffer[ind]) * frac;
        
        /* all-pass filter */
        alpdetune = detune * self->alpsize;
        for (j=0; j<3; j++) {
            xind = self->alp_in_count[j] - (alpdetune * alp_chorus_factor[j]);
            if (xind < 0)
                xind += self->alpsize;
            ind = (long)xind;
            frac = xind - ind;
            alpsampdel = self->alpbuffer[j][ind] + (self->alpbuffer[j][ind+1] - self->alpbuffer[j][ind]) * frac;
            alpsampdelin = val + ((val - alpsampdel) * alp_feedback);
            val = alpsampdelin * alp_feedback + alpsampdel;
            /* write current allpass value in the allpass delay line */
            self->alpbuffer[j][self->alp_in_count[j]] = alpsampdelin;
            if (self->alp_in_count[j] == 0)
                self->alpbuffer[j][self->alpsize] = alpsampdelin;
            self->alp_in_count[j]++;
            if (self->alp_in_count[j] == self->alpsize)
                self->alp_in_count[j] = 0;
        }
        
        /* DC filtering and output */
        y = val - self->xn1 + 0.995 * self->yn1;
        self->xn1 = val;
        self->data[i] = self->yn1 = y;
        
        /* write current value in the delay line */
        self->buffer[self->in_count] = in[i] + val * feed;
        if (self->in_count == 0)
            self->buffer[self->size] = self->buffer[0];
        self->in_count++;
        if (self->in_count == self->size)
            self->in_count = 0;        
    }
}

static void AllpassWG_postprocessing_ii(AllpassWG *self) { POST_PROCESSING_II };
static void AllpassWG_postprocessing_ai(AllpassWG *self) { POST_PROCESSING_AI };
static void AllpassWG_postprocessing_ia(AllpassWG *self) { POST_PROCESSING_IA };
static void AllpassWG_postprocessing_aa(AllpassWG *self) { POST_PROCESSING_AA };
static void AllpassWG_postprocessing_ireva(AllpassWG *self) { POST_PROCESSING_IREVA };
static void AllpassWG_postprocessing_areva(AllpassWG *self) { POST_PROCESSING_AREVA };
static void AllpassWG_postprocessing_revai(AllpassWG *self) { POST_PROCESSING_REVAI };
static void AllpassWG_postprocessing_revaa(AllpassWG *self) { POST_PROCESSING_REVAA };
static void AllpassWG_postprocessing_revareva(AllpassWG *self) { POST_PROCESSING_REVAREVA };

static void
AllpassWG_setProcMode(AllpassWG *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = AllpassWG_process_iii;
            break;
        case 1:    
            self->proc_func_ptr = AllpassWG_process_aii;
            break;
        case 10:    
            self->proc_func_ptr = AllpassWG_process_iai;
            break;
        case 11:    
            self->proc_func_ptr = AllpassWG_process_aai;
            break;
        case 100:    
            self->proc_func_ptr = AllpassWG_process_iia;
            break;
        case 101:    
            self->proc_func_ptr = AllpassWG_process_aia;
            break;
        case 110:    
            self->proc_func_ptr = AllpassWG_process_iaa;
            break;
        case 111:    
            self->proc_func_ptr = AllpassWG_process_aaa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = AllpassWG_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = AllpassWG_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = AllpassWG_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = AllpassWG_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = AllpassWG_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = AllpassWG_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = AllpassWG_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = AllpassWG_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = AllpassWG_postprocessing_revareva;
            break;
    } 
}

static void
AllpassWG_compute_next_data_frame(AllpassWG *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
AllpassWG_traverse(AllpassWG *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->freq);    
    Py_VISIT(self->freq_stream);    
    Py_VISIT(self->feed);    
    Py_VISIT(self->feed_stream);    
    Py_VISIT(self->detune);    
    Py_VISIT(self->detune_stream);    
    return 0;
}

static int 
AllpassWG_clear(AllpassWG *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);    
    Py_CLEAR(self->freq);    
    Py_CLEAR(self->freq_stream);    
    Py_CLEAR(self->feed);    
    Py_CLEAR(self->feed_stream);    
    Py_CLEAR(self->detune);    
    Py_CLEAR(self->detune_stream);    
    return 0;
}

static void
AllpassWG_dealloc(AllpassWG* self)
{
    int i;
    free(self->data);
    free(self->buffer);
    for(i=0; i<3; i++) {
        free(self->alpbuffer[i]);
    }
    AllpassWG_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * AllpassWG_deleteStream(AllpassWG *self) { DELETE_STREAM };

static PyObject *
AllpassWG_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    AllpassWG *self;
    self = (AllpassWG *)type->tp_alloc(type, 0);
    
    self->freq = PyFloat_FromDouble(100);
    self->feed = PyFloat_FromDouble(0.);
    self->detune = PyFloat_FromDouble(0.5);
    self->minfreq = 20;
    self->in_count = self->alp_in_count[0] = self->alp_in_count[1] = self->alp_in_count[2] = 0;
    self->xn1 = 0.0;
    self->yn1 = 0.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, AllpassWG_compute_next_data_frame);
    self->mode_func_ptr = AllpassWG_setProcMode;
    
    return (PyObject *)self;
}

static int
AllpassWG_init(AllpassWG *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *freqtmp=NULL, *feedtmp=NULL, *detunetmp=NULL, *multmp=NULL, *addtmp=NULL;
    int i, j;
    
    static char *kwlist[] = {"input", "freq", "feed", "detune", "minfreq", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOfOO", kwlist, &inputtmp, &freqtmp, &feedtmp, &detunetmp, &self->minfreq, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (freqtmp) {
        PyObject_CallMethod((PyObject *)self, "setFreq", "O", freqtmp);
    }
    
    if (feedtmp) {
        PyObject_CallMethod((PyObject *)self, "setFeed", "O", feedtmp);
    }
    if (detunetmp) {
        PyObject_CallMethod((PyObject *)self, "setDetune", "O", detunetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->size = (long)(1.0 / self->minfreq * self->sr + 0.5);    
    self->buffer = (MYFLT *)realloc(self->buffer, (self->size+1) * sizeof(MYFLT));
    for (i=0; i<(self->size+1); i++) {
        self->buffer[i] = 0.;
    }    

    self->alpsize = (int)(self->sr * 0.0025);
    for (i=0; i<3; i++) {
        self->alpbuffer[i] = (MYFLT *)realloc(self->alpbuffer[i], (self->alpsize+1) * sizeof(MYFLT));
        for (j=0; j<(self->alpsize+1); j++) {
            self->alpbuffer[i][j] = 0.;
        }
    }    
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * AllpassWG_getServer(AllpassWG* self) { GET_SERVER };
static PyObject * AllpassWG_getStream(AllpassWG* self) { GET_STREAM };
static PyObject * AllpassWG_setMul(AllpassWG *self, PyObject *arg) { SET_MUL };	
static PyObject * AllpassWG_setAdd(AllpassWG *self, PyObject *arg) { SET_ADD };	
static PyObject * AllpassWG_setSub(AllpassWG *self, PyObject *arg) { SET_SUB };	
static PyObject * AllpassWG_setDiv(AllpassWG *self, PyObject *arg) { SET_DIV };	

static PyObject * AllpassWG_play(AllpassWG *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * AllpassWG_out(AllpassWG *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * AllpassWG_stop(AllpassWG *self) { STOP };

static PyObject * AllpassWG_multiply(AllpassWG *self, PyObject *arg) { MULTIPLY };
static PyObject * AllpassWG_inplace_multiply(AllpassWG *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * AllpassWG_add(AllpassWG *self, PyObject *arg) { ADD };
static PyObject * AllpassWG_inplace_add(AllpassWG *self, PyObject *arg) { INPLACE_ADD };
static PyObject * AllpassWG_sub(AllpassWG *self, PyObject *arg) { SUB };
static PyObject * AllpassWG_inplace_sub(AllpassWG *self, PyObject *arg) { INPLACE_SUB };
static PyObject * AllpassWG_div(AllpassWG *self, PyObject *arg) { DIV };
static PyObject * AllpassWG_inplace_div(AllpassWG *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
AllpassWG_setFreq(AllpassWG *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->freq);
	if (isNumber == 1) {
		self->freq = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->freq = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->freq, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->freq_stream);
        self->freq_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
AllpassWG_setFeed(AllpassWG *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->feed);
	if (isNumber == 1) {
		self->feed = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->feed = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->feed, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->feed_stream);
        self->feed_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
AllpassWG_setDetune(AllpassWG *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
    
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->detune);
	if (isNumber == 1) {
		self->detune = PyNumber_Float(tmp);
        self->modebuffer[4] = 0;
	}
	else {
		self->detune = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->detune, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->detune_stream);
        self->detune_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef AllpassWG_members[] = {
    {"server", T_OBJECT_EX, offsetof(AllpassWG, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(AllpassWG, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(AllpassWG, input), 0, "Input sound object."},
    {"freq", T_OBJECT_EX, offsetof(AllpassWG, freq), 0, "AllpassWG time in seconds."},
    {"feed", T_OBJECT_EX, offsetof(AllpassWG, feed), 0, "Feedback value."},
    {"detune", T_OBJECT_EX, offsetof(AllpassWG, detune), 0, "Detune value between 0 and 1."},
    {"mul", T_OBJECT_EX, offsetof(AllpassWG, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(AllpassWG, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef AllpassWG_methods[] = {
    {"getServer", (PyCFunction)AllpassWG_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)AllpassWG_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)AllpassWG_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)AllpassWG_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)AllpassWG_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)AllpassWG_stop, METH_NOARGS, "Stops computing."},
    {"setFreq", (PyCFunction)AllpassWG_setFreq, METH_O, "Sets freq time in seconds."},
    {"setFeed", (PyCFunction)AllpassWG_setFeed, METH_O, "Sets feed value between 0 -> 1."},
    {"setDetune", (PyCFunction)AllpassWG_setDetune, METH_O, "Sets detune value between 0 -> 1."},
    {"setMul", (PyCFunction)AllpassWG_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)AllpassWG_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)AllpassWG_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)AllpassWG_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods AllpassWG_as_number = {
    (binaryfunc)AllpassWG_add,                      /*nb_add*/
    (binaryfunc)AllpassWG_sub,                 /*nb_subtract*/
    (binaryfunc)AllpassWG_multiply,                 /*nb_multiply*/
    (binaryfunc)AllpassWG_div,                   /*nb_divide*/
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
    (binaryfunc)AllpassWG_inplace_add,              /*inplace_add*/
    (binaryfunc)AllpassWG_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)AllpassWG_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)AllpassWG_inplace_div,           /*inplace_divide*/
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

PyTypeObject AllpassWGType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.AllpassWG_base",         /*tp_name*/
    sizeof(AllpassWG),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)AllpassWG_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &AllpassWG_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "AllpassWG objects. Waveguide model with builtin allpass circuit to detune resonance frequencies.", /* tp_doc */
    (traverseproc)AllpassWG_traverse,   /* tp_traverse */
    (inquiry)AllpassWG_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    AllpassWG_methods,             /* tp_methods */
    AllpassWG_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)AllpassWG_init,      /* tp_init */
    0,                         /* tp_alloc */
    AllpassWG_new,                 /* tp_new */
};
