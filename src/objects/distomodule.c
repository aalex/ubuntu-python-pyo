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
    PyObject *drive;
    Stream *drive_stream;
    PyObject *slope;
    Stream *slope_stream;
    int init;
    int modebuffer[4];
    MYFLT y1; // sample memory
} Disto;

static MYFLT
_clip(MYFLT x)
{
    if (x < 0)
        return 0;
    else if (x > 1)
        return 1;
    else
        return x;
}

static void
Disto_transform_ii(Disto *self) {
    MYFLT val, coeff;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT drv = .4 - _clip(PyFloat_AS_DOUBLE(self->drive)) * .3999;
    MYFLT slp = _clip(PyFloat_AS_DOUBLE(self->slope));
    
    for (i=0; i<self->bufsize; i++) {
        val = MYATAN2(in[i], drv);
        self->data[i] = val;
    }
    coeff = 1.0 - slp;
    for (i=0; i<self->bufsize; i++) {
        val = self->data[i] * coeff + self->y1 * slp;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Disto_transform_ai(Disto *self) {
    MYFLT val, drv, coeff;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);

    MYFLT *drive = Stream_getData((Stream *)self->drive_stream);
    MYFLT slp = _clip(PyFloat_AS_DOUBLE(self->slope));
    
    for (i=0; i<self->bufsize; i++) {
        drv = .4 - _clip(drive[i]) * .3999;
        val = MYATAN2(in[i], drv);
        self->data[i] = val;
    }
    
    coeff = 1.0 - slp;
    for (i=0; i<self->bufsize; i++) {
        val = self->data[i] * coeff + self->y1 * slp;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Disto_transform_ia(Disto *self) {
    MYFLT val, coeff, slp;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    MYFLT drv = .4 - _clip(PyFloat_AS_DOUBLE(self->drive)) * .3999;
    MYFLT *slope = Stream_getData((Stream *)self->slope_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = MYATAN2(in[i], drv);
        self->data[i] = val;
    }
    for (i=0; i<self->bufsize; i++) {
        slp = _clip(slope[i]);
        coeff = 1.0 - slp;
        val = self->data[i] * coeff + self->y1 * slp;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void
Disto_transform_aa(Disto *self) {
    MYFLT val, drv, coeff, slp;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    
    MYFLT *drive = Stream_getData((Stream *)self->drive_stream);
    MYFLT *slope = Stream_getData((Stream *)self->slope_stream);
    
    for (i=0; i<self->bufsize; i++) {
        drv = .4 - _clip(drive[i]) * .3999;
        val = MYATAN2(in[i], drv);
        self->data[i] = val;
    }
    for (i=0; i<self->bufsize; i++) {
        slp = _clip(slope[i]);
        coeff = 1.0 - slp;
        val = self->data[i] * coeff + self->y1 * slp;
        self->y1 = val;
        self->data[i] = val;
    }
}

static void Disto_postprocessing_ii(Disto *self) { POST_PROCESSING_II };
static void Disto_postprocessing_ai(Disto *self) { POST_PROCESSING_AI };
static void Disto_postprocessing_ia(Disto *self) { POST_PROCESSING_IA };
static void Disto_postprocessing_aa(Disto *self) { POST_PROCESSING_AA };
static void Disto_postprocessing_ireva(Disto *self) { POST_PROCESSING_IREVA };
static void Disto_postprocessing_areva(Disto *self) { POST_PROCESSING_AREVA };
static void Disto_postprocessing_revai(Disto *self) { POST_PROCESSING_REVAI };
static void Disto_postprocessing_revaa(Disto *self) { POST_PROCESSING_REVAA };
static void Disto_postprocessing_revareva(Disto *self) { POST_PROCESSING_REVAREVA };

static void
Disto_setProcMode(Disto *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Disto_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Disto_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Disto_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Disto_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Disto_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Disto_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Disto_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Disto_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Disto_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Disto_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Disto_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Disto_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Disto_postprocessing_revareva;
            break;
    }   
}

static void
Disto_compute_next_data_frame(Disto *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Disto_traverse(Disto *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->drive);    
    Py_VISIT(self->drive_stream);    
    Py_VISIT(self->slope);    
    Py_VISIT(self->slope_stream);    
    return 0;
}

static int 
Disto_clear(Disto *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->drive);    
    Py_CLEAR(self->drive_stream);    
    Py_CLEAR(self->slope);    
    Py_CLEAR(self->slope_stream);    
    return 0;
}

static void
Disto_dealloc(Disto* self)
{
    free(self->data);
    Disto_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Disto_deleteStream(Disto *self) { DELETE_STREAM };

static PyObject *
Disto_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Disto *self;
    self = (Disto *)type->tp_alloc(type, 0);

    self->drive = PyFloat_FromDouble(.75);
    self->slope = PyFloat_FromDouble(.5);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    self->y1 = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Disto_compute_next_data_frame);
    self->mode_func_ptr = Disto_setProcMode;
    
    return (PyObject *)self;
}

static int
Disto_init(Disto *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *drivetmp=NULL, *slopetmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"input", "drive", "slope", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &drivetmp, &slopetmp, &multmp, &addtmp))
        return -1; 

    INIT_INPUT_STREAM
    
    if (drivetmp) {
        PyObject_CallMethod((PyObject *)self, "setDrive", "O", drivetmp);
    }

    if (slopetmp) {
        PyObject_CallMethod((PyObject *)self, "setSlope", "O", slopetmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Disto_getServer(Disto* self) { GET_SERVER };
static PyObject * Disto_getStream(Disto* self) { GET_STREAM };
static PyObject * Disto_setMul(Disto *self, PyObject *arg) { SET_MUL };	
static PyObject * Disto_setAdd(Disto *self, PyObject *arg) { SET_ADD };	
static PyObject * Disto_setSub(Disto *self, PyObject *arg) { SET_SUB };	
static PyObject * Disto_setDiv(Disto *self, PyObject *arg) { SET_DIV };	

static PyObject * Disto_play(Disto *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Disto_out(Disto *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Disto_stop(Disto *self) { STOP };

static PyObject * Disto_multiply(Disto *self, PyObject *arg) { MULTIPLY };
static PyObject * Disto_inplace_multiply(Disto *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Disto_add(Disto *self, PyObject *arg) { ADD };
static PyObject * Disto_inplace_add(Disto *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Disto_sub(Disto *self, PyObject *arg) { SUB };
static PyObject * Disto_inplace_sub(Disto *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Disto_div(Disto *self, PyObject *arg) { DIV };
static PyObject * Disto_inplace_div(Disto *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Disto_setDrive(Disto *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->drive);
	if (isNumber == 1) {
		self->drive = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->drive = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->drive, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->drive_stream);
        self->drive_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Disto_setSlope(Disto *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->slope);
	if (isNumber == 1) {
		self->slope = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->slope = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->slope, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->slope_stream);
        self->slope_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Disto_members[] = {
    {"server", T_OBJECT_EX, offsetof(Disto, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Disto, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Disto, input), 0, "Input sound object."},
    {"drive", T_OBJECT_EX, offsetof(Disto, drive), 0, "Cutoff driveuency in cycle per second."},
    {"slope", T_OBJECT_EX, offsetof(Disto, slope), 0, "Lowpass filter slope factor."},
    {"mul", T_OBJECT_EX, offsetof(Disto, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Disto, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Disto_methods[] = {
    {"getServer", (PyCFunction)Disto_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Disto_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Disto_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Disto_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Disto_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Disto_stop, METH_NOARGS, "Stops computing."},
	{"setDrive", (PyCFunction)Disto_setDrive, METH_O, "Sets distortion drive factor (0 -> 1)."},
    {"setSlope", (PyCFunction)Disto_setSlope, METH_O, "Sets lowpass filter slope factor."},
	{"setMul", (PyCFunction)Disto_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)Disto_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Disto_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Disto_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Disto_as_number = {
    (binaryfunc)Disto_add,                      /*nb_add*/
    (binaryfunc)Disto_sub,                 /*nb_subtract*/
    (binaryfunc)Disto_multiply,                 /*nb_multiply*/
    (binaryfunc)Disto_div,                   /*nb_divide*/
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
    (binaryfunc)Disto_inplace_add,              /*inplace_add*/
    (binaryfunc)Disto_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Disto_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Disto_inplace_div,           /*inplace_divide*/
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

PyTypeObject DistoType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Disto_base",         /*tp_name*/
    sizeof(Disto),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Disto_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Disto_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Disto objects. Arctan distortion.",           /* tp_doc */
    (traverseproc)Disto_traverse,   /* tp_traverse */
    (inquiry)Disto_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Disto_methods,             /* tp_methods */
    Disto_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Disto_init,      /* tp_init */
    0,                         /* tp_alloc */
    Disto_new,                 /* tp_new */
};

/*****************/
/** Clip object **/
/*****************/

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    Stream *min_stream;
    PyObject *max;
    Stream *max_stream;
    int modebuffer[4];
} Clip;

static void
Clip_transform_ii(Clip *self) {
    MYFLT val;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        if(val < mi)
            self->data[i] = mi;
        else if(val > ma)
            self->data[i] = ma;
        else
            self->data[i] = val;
    }
}

static void
Clip_transform_ai(Clip *self) {
    MYFLT val, mini;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        mini = mi[i];
        if(val < mini)
            self->data[i] = mini;
        else if(val > ma)
            self->data[i] = ma;
        else
            self->data[i] = val;
    }
}

static void
Clip_transform_ia(Clip *self) {
    MYFLT val, maxi;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        maxi = ma[i];
        if(val < mi)
            self->data[i] = mi;
        else if(val > maxi)
            self->data[i] = maxi;
        else
            self->data[i] = val;
    }
}
    
static void
Clip_transform_aa(Clip *self) {
    MYFLT val, mini, maxi;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mi = Stream_getData((Stream *)self->min_stream);
    MYFLT *ma = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        mini = mi[i];
        maxi = ma[i];
        if(val < mini)
            self->data[i] = mini;
        else if(val > maxi)
            self->data[i] = maxi;
        else
            self->data[i] = val;
    }
}

static void Clip_postprocessing_ii(Clip *self) { POST_PROCESSING_II };
static void Clip_postprocessing_ai(Clip *self) { POST_PROCESSING_AI };
static void Clip_postprocessing_ia(Clip *self) { POST_PROCESSING_IA };
static void Clip_postprocessing_aa(Clip *self) { POST_PROCESSING_AA };
static void Clip_postprocessing_ireva(Clip *self) { POST_PROCESSING_IREVA };
static void Clip_postprocessing_areva(Clip *self) { POST_PROCESSING_AREVA };
static void Clip_postprocessing_revai(Clip *self) { POST_PROCESSING_REVAI };
static void Clip_postprocessing_revaa(Clip *self) { POST_PROCESSING_REVAA };
static void Clip_postprocessing_revareva(Clip *self) { POST_PROCESSING_REVAREVA };

static void
Clip_setProcMode(Clip *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Clip_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Clip_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Clip_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Clip_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Clip_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Clip_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Clip_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Clip_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Clip_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Clip_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Clip_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Clip_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Clip_postprocessing_revareva;
            break;
    }   
}

static void
Clip_compute_next_data_frame(Clip *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Clip_traverse(Clip *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
Clip_clear(Clip *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
Clip_dealloc(Clip* self)
{
    free(self->data);
    Clip_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Clip_deleteStream(Clip *self) { DELETE_STREAM };

static PyObject *
Clip_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Clip *self;
    self = (Clip *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(-1.0);
    self->max = PyFloat_FromDouble(1.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Clip_compute_next_data_frame);
    self->mode_func_ptr = Clip_setProcMode;
    
    return (PyObject *)self;
}

static int
Clip_init(Clip *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &mintmp, &maxtmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Clip_getServer(Clip* self) { GET_SERVER };
static PyObject * Clip_getStream(Clip* self) { GET_STREAM };
static PyObject * Clip_setMul(Clip *self, PyObject *arg) { SET_MUL };	
static PyObject * Clip_setAdd(Clip *self, PyObject *arg) { SET_ADD };	
static PyObject * Clip_setSub(Clip *self, PyObject *arg) { SET_SUB };	
static PyObject * Clip_setDiv(Clip *self, PyObject *arg) { SET_DIV };	

static PyObject * Clip_play(Clip *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Clip_out(Clip *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Clip_stop(Clip *self) { STOP };

static PyObject * Clip_multiply(Clip *self, PyObject *arg) { MULTIPLY };
static PyObject * Clip_inplace_multiply(Clip *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Clip_add(Clip *self, PyObject *arg) { ADD };
static PyObject * Clip_inplace_add(Clip *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Clip_sub(Clip *self, PyObject *arg) { SUB };
static PyObject * Clip_inplace_sub(Clip *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Clip_div(Clip *self, PyObject *arg) { DIV };
static PyObject * Clip_inplace_div(Clip *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Clip_setMin(Clip *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Clip_setMax(Clip *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Clip_members[] = {
{"server", T_OBJECT_EX, offsetof(Clip, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Clip, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Clip, input), 0, "Input sound object."},
{"min", T_OBJECT_EX, offsetof(Clip, min), 0, "Minimum possible value."},
{"max", T_OBJECT_EX, offsetof(Clip, max), 0, "Maximum possible value."},
{"mul", T_OBJECT_EX, offsetof(Clip, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Clip, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Clip_methods[] = {
{"getServer", (PyCFunction)Clip_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Clip_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Clip_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Clip_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Clip_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Clip_stop, METH_NOARGS, "Stops computing."},
{"setMin", (PyCFunction)Clip_setMin, METH_O, "Sets the minimum value."},
{"setMax", (PyCFunction)Clip_setMax, METH_O, "Sets the maximum value."},
{"setMul", (PyCFunction)Clip_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Clip_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Clip_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Clip_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Clip_as_number = {
(binaryfunc)Clip_add,                      /*nb_add*/
(binaryfunc)Clip_sub,                 /*nb_subtract*/
(binaryfunc)Clip_multiply,                 /*nb_multiply*/
(binaryfunc)Clip_div,                   /*nb_divide*/
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
(binaryfunc)Clip_inplace_add,              /*inplace_add*/
(binaryfunc)Clip_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Clip_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Clip_inplace_div,           /*inplace_divide*/
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

PyTypeObject ClipType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Clip_base",         /*tp_name*/
sizeof(Clip),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Clip_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Clip_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Clip objects. Clips a signal to a predefined limit.",           /* tp_doc */
(traverseproc)Clip_traverse,   /* tp_traverse */
(inquiry)Clip_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Clip_methods,             /* tp_methods */
Clip_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Clip_init,      /* tp_init */
0,                         /* tp_alloc */
Clip_new,                 /* tp_new */
};

/*****************/
/* Mirror object */
/*****************/

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    Stream *min_stream;
    PyObject *max;
    Stream *max_stream;
    int modebuffer[4];
} Mirror;

static void
Mirror_transform_ii(Mirror *self) {
    MYFLT val, avg;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    if (mi >= ma) {
        avg = (mi + ma) * 0.5;
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = avg;
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            val = in[i];
            while ((val > ma) || (val < mi)) {
                if (val > ma)
                    val = ma + ma - val;
                else
                    val = mi + mi - val;
            }
            self->data[i] = val;
        }
    }
}

static void
Mirror_transform_ai(Mirror *self) {
    MYFLT val, avg, mi;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mini = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        mi = mini[i];
        if (mi >= ma) {
            avg = (mi + ma) * 0.5;
            self->data[i] = avg;
        }
        else {
            while ((val > ma) || (val < mi)) {
                if (val > ma)
                    val = ma + ma - val;
                else
                    val = mi + mi - val;
            }
            self->data[i] = val;            
        }
    }
}

static void
Mirror_transform_ia(Mirror *self) {
    MYFLT val, avg, ma;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *maxi = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        ma = maxi[i];
        if (mi >= ma) {
            avg = (mi + ma) * 0.5;
            self->data[i] = avg;
        }
        else {
            while ((val > ma) || (val < mi)) {
                if (val > ma)
                    val = ma + ma - val;
                else
                    val = mi + mi - val;
            }
            self->data[i] = val;            
        }
    }
}

static void
Mirror_transform_aa(Mirror *self) {
    MYFLT val, avg, mi, ma;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mini = Stream_getData((Stream *)self->min_stream);
    MYFLT *maxi = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        mi = mini[i];
        ma = maxi[i];
        if (mi >= ma) {
            avg = (mi + ma) * 0.5;
            self->data[i] = avg;
        }
        else {
            while ((val > ma) || (val < mi)) {
                if (val > ma)
                    val = ma + ma - val;
                else
                    val = mi + mi - val;
            }
            self->data[i] = val;            
        }
    }
}

static void Mirror_postprocessing_ii(Mirror *self) { POST_PROCESSING_II };
static void Mirror_postprocessing_ai(Mirror *self) { POST_PROCESSING_AI };
static void Mirror_postprocessing_ia(Mirror *self) { POST_PROCESSING_IA };
static void Mirror_postprocessing_aa(Mirror *self) { POST_PROCESSING_AA };
static void Mirror_postprocessing_ireva(Mirror *self) { POST_PROCESSING_IREVA };
static void Mirror_postprocessing_areva(Mirror *self) { POST_PROCESSING_AREVA };
static void Mirror_postprocessing_revai(Mirror *self) { POST_PROCESSING_REVAI };
static void Mirror_postprocessing_revaa(Mirror *self) { POST_PROCESSING_REVAA };
static void Mirror_postprocessing_revareva(Mirror *self) { POST_PROCESSING_REVAREVA };

static void
Mirror_setProcMode(Mirror *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Mirror_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Mirror_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Mirror_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Mirror_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Mirror_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Mirror_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Mirror_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Mirror_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Mirror_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Mirror_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Mirror_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Mirror_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Mirror_postprocessing_revareva;
            break;
    }   
}

static void
Mirror_compute_next_data_frame(Mirror *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Mirror_traverse(Mirror *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
Mirror_clear(Mirror *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
Mirror_dealloc(Mirror* self)
{
    free(self->data);
    Mirror_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Mirror_deleteStream(Mirror *self) { DELETE_STREAM };

static PyObject *
Mirror_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Mirror *self;
    self = (Mirror *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(0.0);
    self->max = PyFloat_FromDouble(1.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Mirror_compute_next_data_frame);
    self->mode_func_ptr = Mirror_setProcMode;
    
    return (PyObject *)self;
}

static int
Mirror_init(Mirror *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &mintmp, &maxtmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Mirror_getServer(Mirror* self) { GET_SERVER };
static PyObject * Mirror_getStream(Mirror* self) { GET_STREAM };
static PyObject * Mirror_setMul(Mirror *self, PyObject *arg) { SET_MUL };	
static PyObject * Mirror_setAdd(Mirror *self, PyObject *arg) { SET_ADD };	
static PyObject * Mirror_setSub(Mirror *self, PyObject *arg) { SET_SUB };	
static PyObject * Mirror_setDiv(Mirror *self, PyObject *arg) { SET_DIV };	

static PyObject * Mirror_play(Mirror *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Mirror_out(Mirror *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Mirror_stop(Mirror *self) { STOP };

static PyObject * Mirror_multiply(Mirror *self, PyObject *arg) { MULTIPLY };
static PyObject * Mirror_inplace_multiply(Mirror *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Mirror_add(Mirror *self, PyObject *arg) { ADD };
static PyObject * Mirror_inplace_add(Mirror *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Mirror_sub(Mirror *self, PyObject *arg) { SUB };
static PyObject * Mirror_inplace_sub(Mirror *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Mirror_div(Mirror *self, PyObject *arg) { DIV };
static PyObject * Mirror_inplace_div(Mirror *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Mirror_setMin(Mirror *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Mirror_setMax(Mirror *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Mirror_members[] = {
    {"server", T_OBJECT_EX, offsetof(Mirror, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Mirror, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Mirror, input), 0, "Input sound object."},
    {"min", T_OBJECT_EX, offsetof(Mirror, min), 0, "Minimum possible value."},
    {"max", T_OBJECT_EX, offsetof(Mirror, max), 0, "Maximum possible value."},
    {"mul", T_OBJECT_EX, offsetof(Mirror, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Mirror, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Mirror_methods[] = {
    {"getServer", (PyCFunction)Mirror_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Mirror_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Mirror_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Mirror_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Mirror_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Mirror_stop, METH_NOARGS, "Stops computing."},
    {"setMin", (PyCFunction)Mirror_setMin, METH_O, "Sets the minimum value."},
    {"setMax", (PyCFunction)Mirror_setMax, METH_O, "Sets the maximum value."},
    {"setMul", (PyCFunction)Mirror_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Mirror_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Mirror_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Mirror_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Mirror_as_number = {
    (binaryfunc)Mirror_add,                      /*nb_add*/
    (binaryfunc)Mirror_sub,                 /*nb_subtract*/
    (binaryfunc)Mirror_multiply,                 /*nb_multiply*/
    (binaryfunc)Mirror_div,                   /*nb_divide*/
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
    (binaryfunc)Mirror_inplace_add,              /*inplace_add*/
    (binaryfunc)Mirror_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Mirror_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Mirror_inplace_div,           /*inplace_divide*/
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

PyTypeObject MirrorType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Mirror_base",         /*tp_name*/
    sizeof(Mirror),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Mirror_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Mirror_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Mirror objects. Reflects the signal that exceeds the min and max thresholds.",           /* tp_doc */
    (traverseproc)Mirror_traverse,   /* tp_traverse */
    (inquiry)Mirror_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Mirror_methods,             /* tp_methods */
    Mirror_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Mirror_init,      /* tp_init */
    0,                         /* tp_alloc */
    Mirror_new,                 /* tp_new */
};

/*****************/
/** Wrap object **/
/*****************/

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *min;
    Stream *min_stream;
    PyObject *max;
    Stream *max_stream;
    int modebuffer[4];
} Wrap;

static void
Wrap_transform_ii(Wrap *self) {
    MYFLT val, avg, rng, tmp;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    if (mi >= ma) {
        avg = (mi + ma) * 0.5;
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = avg;
        }
    }
    else {
        rng = ma - mi;
        for (i=0; i<self->bufsize; i++) {
            val = in[i];
            tmp = (val - mi) / rng;
            if (tmp >= 1.0) {
                tmp -= (int)tmp;
                val = tmp * rng + mi;
            }
            else if (tmp < 0) {
                tmp += (int)(-tmp) + 1;
                val = tmp * rng + mi;
            }    
            self->data[i] = val;
        }
    }
}

static void
Wrap_transform_ai(Wrap *self) {
    MYFLT val, avg, rng, tmp, mi;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mini = Stream_getData((Stream *)self->min_stream);
    MYFLT ma = PyFloat_AS_DOUBLE(self->max);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        mi = mini[i];
        if (mi >= ma) {
            avg = (mi + ma) * 0.5;
            self->data[i] = avg;
        }
        else {
            rng = ma - mi;
            tmp = (val - mi) / rng;
            if (tmp >= 1.0) {
                tmp -= (int)tmp;
                val = tmp * rng + mi;
            }
            else if (tmp < 0) {
                tmp += (int)(-tmp) + 1;
                val = tmp * rng + mi;
            }    
            self->data[i] = val;            
        }
    }
}

static void
Wrap_transform_ia(Wrap *self) {
    MYFLT val, avg, rng, tmp, ma;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT mi = PyFloat_AS_DOUBLE(self->min);
    MYFLT *maxi = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        ma = maxi[i];
        if (mi >= ma) {
            avg = (mi + ma) * 0.5;
            self->data[i] = avg;
        }
        else {
            rng = ma - mi;
            tmp = (val - mi) / rng;
            if (tmp >= 1.0) {
                tmp -= (int)tmp;
                val = tmp * rng + mi;
            }
            else if (tmp < 0) {
                tmp += (int)(-tmp) + 1;
                val = tmp * rng + mi;
            }    
            self->data[i] = val;            
        }
    }
}

static void
Wrap_transform_aa(Wrap *self) {
    MYFLT val, avg, rng, tmp, mi, ma;
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *mini = Stream_getData((Stream *)self->min_stream);
    MYFLT *maxi = Stream_getData((Stream *)self->max_stream);
    
    for (i=0; i<self->bufsize; i++) {
        val = in[i];
        mi = mini[i];
        ma = maxi[i];
        if (mi >= ma) {
            avg = (mi + ma) * 0.5;
            self->data[i] = avg;
        }
        else {
            rng = ma - mi;
            tmp = (val - mi) / rng;
            if (tmp >= 1.0) {
                tmp -= (int)tmp;
                val = tmp * rng + mi;
            }
            else if (tmp < 0) {
                tmp += (int)(-tmp) + 1;
                val = tmp * rng + mi;
            }    
            self->data[i] = val;            
        }
    }
}

static void Wrap_postprocessing_ii(Wrap *self) { POST_PROCESSING_II };
static void Wrap_postprocessing_ai(Wrap *self) { POST_PROCESSING_AI };
static void Wrap_postprocessing_ia(Wrap *self) { POST_PROCESSING_IA };
static void Wrap_postprocessing_aa(Wrap *self) { POST_PROCESSING_AA };
static void Wrap_postprocessing_ireva(Wrap *self) { POST_PROCESSING_IREVA };
static void Wrap_postprocessing_areva(Wrap *self) { POST_PROCESSING_AREVA };
static void Wrap_postprocessing_revai(Wrap *self) { POST_PROCESSING_REVAI };
static void Wrap_postprocessing_revaa(Wrap *self) { POST_PROCESSING_REVAA };
static void Wrap_postprocessing_revareva(Wrap *self) { POST_PROCESSING_REVAREVA };

static void
Wrap_setProcMode(Wrap *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Wrap_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Wrap_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Wrap_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Wrap_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Wrap_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Wrap_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Wrap_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Wrap_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Wrap_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Wrap_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Wrap_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Wrap_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Wrap_postprocessing_revareva;
            break;
    }   
}

static void
Wrap_compute_next_data_frame(Wrap *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Wrap_traverse(Wrap *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->min);    
    Py_VISIT(self->min_stream);    
    Py_VISIT(self->max);    
    Py_VISIT(self->max_stream);    
    return 0;
}

static int 
Wrap_clear(Wrap *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->min);    
    Py_CLEAR(self->min_stream);    
    Py_CLEAR(self->max);    
    Py_CLEAR(self->max_stream);    
    return 0;
}

static void
Wrap_dealloc(Wrap* self)
{
    free(self->data);
    Wrap_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Wrap_deleteStream(Wrap *self) { DELETE_STREAM };

static PyObject *
Wrap_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Wrap *self;
    self = (Wrap *)type->tp_alloc(type, 0);
    
    self->min = PyFloat_FromDouble(0.0);
    self->max = PyFloat_FromDouble(1.0);
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Wrap_compute_next_data_frame);
    self->mode_func_ptr = Wrap_setProcMode;
    
    return (PyObject *)self;
}

static int
Wrap_init(Wrap *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *mintmp=NULL, *maxtmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "min", "max", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &mintmp, &maxtmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (mintmp) {
        PyObject_CallMethod((PyObject *)self, "setMin", "O", mintmp);
    }
    
    if (maxtmp) {
        PyObject_CallMethod((PyObject *)self, "setMax", "O", maxtmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Wrap_getServer(Wrap* self) { GET_SERVER };
static PyObject * Wrap_getStream(Wrap* self) { GET_STREAM };
static PyObject * Wrap_setMul(Wrap *self, PyObject *arg) { SET_MUL };	
static PyObject * Wrap_setAdd(Wrap *self, PyObject *arg) { SET_ADD };	
static PyObject * Wrap_setSub(Wrap *self, PyObject *arg) { SET_SUB };	
static PyObject * Wrap_setDiv(Wrap *self, PyObject *arg) { SET_DIV };	

static PyObject * Wrap_play(Wrap *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Wrap_out(Wrap *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Wrap_stop(Wrap *self) { STOP };

static PyObject * Wrap_multiply(Wrap *self, PyObject *arg) { MULTIPLY };
static PyObject * Wrap_inplace_multiply(Wrap *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Wrap_add(Wrap *self, PyObject *arg) { ADD };
static PyObject * Wrap_inplace_add(Wrap *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Wrap_sub(Wrap *self, PyObject *arg) { SUB };
static PyObject * Wrap_inplace_sub(Wrap *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Wrap_div(Wrap *self, PyObject *arg) { DIV };
static PyObject * Wrap_inplace_div(Wrap *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Wrap_setMin(Wrap *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->min);
	if (isNumber == 1) {
		self->min = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->min = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->min, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->min_stream);
        self->min_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Wrap_setMax(Wrap *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->max);
	if (isNumber == 1) {
		self->max = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->max = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->max, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->max_stream);
        self->max_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Wrap_members[] = {
    {"server", T_OBJECT_EX, offsetof(Wrap, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Wrap, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(Wrap, input), 0, "Input sound object."},
    {"min", T_OBJECT_EX, offsetof(Wrap, min), 0, "Minimum possible value."},
    {"max", T_OBJECT_EX, offsetof(Wrap, max), 0, "Maximum possible value."},
    {"mul", T_OBJECT_EX, offsetof(Wrap, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Wrap, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Wrap_methods[] = {
    {"getServer", (PyCFunction)Wrap_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Wrap_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Wrap_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Wrap_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Wrap_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Wrap_stop, METH_NOARGS, "Stops computing."},
    {"setMin", (PyCFunction)Wrap_setMin, METH_O, "Sets the minimum value."},
    {"setMax", (PyCFunction)Wrap_setMax, METH_O, "Sets the maximum value."},
    {"setMul", (PyCFunction)Wrap_setMul, METH_O, "Sets oscillator mul factor."},
    {"setAdd", (PyCFunction)Wrap_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)Wrap_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Wrap_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Wrap_as_number = {
    (binaryfunc)Wrap_add,                      /*nb_add*/
    (binaryfunc)Wrap_sub,                 /*nb_subtract*/
    (binaryfunc)Wrap_multiply,                 /*nb_multiply*/
    (binaryfunc)Wrap_div,                   /*nb_divide*/
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
    (binaryfunc)Wrap_inplace_add,              /*inplace_add*/
    (binaryfunc)Wrap_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Wrap_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Wrap_inplace_div,           /*inplace_divide*/
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

PyTypeObject WrapType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.Wrap_base",         /*tp_name*/
    sizeof(Wrap),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Wrap_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Wrap_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Wrap objects. Wraps-around the signal that exceeds the min and max thresholds.",           /* tp_doc */
    (traverseproc)Wrap_traverse,   /* tp_traverse */
    (inquiry)Wrap_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Wrap_methods,             /* tp_methods */
    Wrap_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Wrap_init,      /* tp_init */
    0,                         /* tp_alloc */
    Wrap_new,                 /* tp_new */
};

/*****************/
/** Degrade object **/
/*****************/

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    PyObject *bitdepth;
    Stream *bitdepth_stream;
    PyObject *srscale;
    Stream *srscale_stream;
    MYFLT value;
    int sampsCount;
    int modebuffer[4];
} Degrade;

static MYFLT 
_bit_clip(MYFLT x) {
    if (x < 1.0)
        return 1.0;
    else if (x > 32.0)
        return 32.0;
    else
        return x;
}

static MYFLT 
_sr_clip(MYFLT x) {
    // half sr ten times
    if (x <= 0.0009765625)
        return 0.0009765625;
    else if (x > 1.0)
        return 1.0;
    else
        return x;
}

static void
Degrade_transform_ii(Degrade *self) {
    MYFLT bitscl, ibitscl, newsr;
    int i, nsamps, tmp;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT bitdepth = _bit_clip(PyFloat_AS_DOUBLE(self->bitdepth));
    MYFLT srscale = _sr_clip(PyFloat_AS_DOUBLE(self->srscale));
    
    bitscl = MYPOW(2.0, bitdepth-1);
    ibitscl = 1.0 / bitscl;
    
    newsr = self->sr * srscale;
    nsamps = (int)(self->sr / newsr);
    
    for (i=0; i<self->bufsize; i++) {
        self->sampsCount++;
        if (self->sampsCount >= nsamps) {
            self->sampsCount = 0;
            tmp = (int)(in[i] * bitscl + 0.5);
            self->value = tmp * ibitscl;    
        }
        self->data[i] = self->value;
    }
}

static void
Degrade_transform_ai(Degrade *self) {
    MYFLT bitscl, ibitscl, newsr;
    int i, nsamps, tmp;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *bitdepth = Stream_getData((Stream *)self->bitdepth_stream);
    MYFLT srscale = _sr_clip(PyFloat_AS_DOUBLE(self->srscale));

    newsr = self->sr * srscale;
    nsamps = (int)(self->sr / newsr);
    
    for (i=0; i<self->bufsize; i++) {
        self->sampsCount++;
        if (self->sampsCount >= nsamps) {
            self->sampsCount = 0;
            bitscl = MYPOW(2.0, _bit_clip(bitdepth[i])-1);
            ibitscl = 1.0 / bitscl;
            tmp = (int)(in[i] * bitscl + 0.5);
            self->value = tmp * ibitscl;    
        }
        self->data[i] = self->value;
    }
}

static void
Degrade_transform_ia(Degrade *self) {
    MYFLT bitscl, ibitscl, newsr;
    int i, nsamps, tmp;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT bitdepth = _bit_clip(PyFloat_AS_DOUBLE(self->bitdepth));
    MYFLT *srscale = Stream_getData((Stream *)self->srscale_stream);
    
    bitscl = MYPOW(2.0, bitdepth-1);
    ibitscl = 1.0 / bitscl;

    for (i=0; i<self->bufsize; i++) {
        newsr = self->sr * _sr_clip(srscale[i]);
        nsamps = (int)(self->sr / newsr);
        self->sampsCount++;
        if (self->sampsCount >= nsamps) {
            self->sampsCount = 0;
            tmp = (int)(in[i] * bitscl + 0.5);
            self->value = tmp * ibitscl;    
        }
        self->data[i] = self->value;
    }
}

static void
Degrade_transform_aa(Degrade *self) {
    MYFLT bitscl, ibitscl, newsr;
    int i, nsamps, tmp;
    
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *bitdepth = Stream_getData((Stream *)self->bitdepth_stream);
    MYFLT *srscale = Stream_getData((Stream *)self->srscale_stream);

    for (i=0; i<self->bufsize; i++) {
        newsr = self->sr * _sr_clip(srscale[i]);
        nsamps = (int)(self->sr / newsr);
        self->sampsCount++;
        if (self->sampsCount >= nsamps) {
            self->sampsCount = 0;
            bitscl = MYPOW(2.0, _bit_clip(bitdepth[i])-1);
            ibitscl = 1.0 / bitscl;
            tmp = (int)(in[i] * bitscl + 0.5);
            self->value = tmp * ibitscl;    
        }
        self->data[i] = self->value;
    }
}

static void Degrade_postprocessing_ii(Degrade *self) { POST_PROCESSING_II };
static void Degrade_postprocessing_ai(Degrade *self) { POST_PROCESSING_AI };
static void Degrade_postprocessing_ia(Degrade *self) { POST_PROCESSING_IA };
static void Degrade_postprocessing_aa(Degrade *self) { POST_PROCESSING_AA };
static void Degrade_postprocessing_ireva(Degrade *self) { POST_PROCESSING_IREVA };
static void Degrade_postprocessing_areva(Degrade *self) { POST_PROCESSING_AREVA };
static void Degrade_postprocessing_revai(Degrade *self) { POST_PROCESSING_REVAI };
static void Degrade_postprocessing_revaa(Degrade *self) { POST_PROCESSING_REVAA };
static void Degrade_postprocessing_revareva(Degrade *self) { POST_PROCESSING_REVAREVA };

static void
Degrade_setProcMode(Degrade *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Degrade_transform_ii;
            break;
        case 1:    
            self->proc_func_ptr = Degrade_transform_ai;
            break;
        case 10:        
            self->proc_func_ptr = Degrade_transform_ia;
            break;
        case 11:    
            self->proc_func_ptr = Degrade_transform_aa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Degrade_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Degrade_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Degrade_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Degrade_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Degrade_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Degrade_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Degrade_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Degrade_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Degrade_postprocessing_revareva;
            break;
    }   
}

static void
Degrade_compute_next_data_frame(Degrade *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Degrade_traverse(Degrade *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);    
    Py_VISIT(self->bitdepth);    
    Py_VISIT(self->bitdepth_stream);    
    Py_VISIT(self->srscale);    
    Py_VISIT(self->srscale_stream);    
    return 0;
}

static int 
Degrade_clear(Degrade *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->bitdepth);    
    Py_CLEAR(self->bitdepth_stream);    
    Py_CLEAR(self->srscale);    
    Py_CLEAR(self->srscale_stream);    
    return 0;
}

static void
Degrade_dealloc(Degrade* self)
{
    free(self->data);
    Degrade_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Degrade_deleteStream(Degrade *self) { DELETE_STREAM };

static PyObject *
Degrade_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Degrade *self;
    self = (Degrade *)type->tp_alloc(type, 0);
    
    self->bitdepth = PyFloat_FromDouble(16);
    self->srscale = PyFloat_FromDouble(1.0);
    self->value = 0.0;
    self->sampsCount = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Degrade_compute_next_data_frame);
    self->mode_func_ptr = Degrade_setProcMode;
    
    return (PyObject *)self;
}

static int
Degrade_init(Degrade *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *bitdepthtmp=NULL, *srscaletmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "bitdepth", "srscale", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOO", kwlist, &inputtmp, &bitdepthtmp, &srscaletmp, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    if (bitdepthtmp) {
        PyObject_CallMethod((PyObject *)self, "setBitdepth", "O", bitdepthtmp);
    }
    
    if (srscaletmp) {
        PyObject_CallMethod((PyObject *)self, "setSrscale", "O", srscaletmp);
    }
    
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    (*self->mode_func_ptr)(self);
        
    Py_INCREF(self);
    return 0;
}

static PyObject * Degrade_getServer(Degrade* self) { GET_SERVER };
static PyObject * Degrade_getStream(Degrade* self) { GET_STREAM };
static PyObject * Degrade_setMul(Degrade *self, PyObject *arg) { SET_MUL };	
static PyObject * Degrade_setAdd(Degrade *self, PyObject *arg) { SET_ADD };	
static PyObject * Degrade_setSub(Degrade *self, PyObject *arg) { SET_SUB };	
static PyObject * Degrade_setDiv(Degrade *self, PyObject *arg) { SET_DIV };	

static PyObject * Degrade_play(Degrade *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Degrade_out(Degrade *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Degrade_stop(Degrade *self) { STOP };

static PyObject * Degrade_multiply(Degrade *self, PyObject *arg) { MULTIPLY };
static PyObject * Degrade_inplace_multiply(Degrade *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Degrade_add(Degrade *self, PyObject *arg) { ADD };
static PyObject * Degrade_inplace_add(Degrade *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Degrade_sub(Degrade *self, PyObject *arg) { SUB };
static PyObject * Degrade_inplace_sub(Degrade *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Degrade_div(Degrade *self, PyObject *arg) { DIV };
static PyObject * Degrade_inplace_div(Degrade *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Degrade_setBitdepth(Degrade *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->bitdepth);
	if (isNumber == 1) {
		self->bitdepth = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->bitdepth = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->bitdepth, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->bitdepth_stream);
        self->bitdepth_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Degrade_setSrscale(Degrade *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->srscale);
	if (isNumber == 1) {
		self->srscale = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->srscale = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->srscale, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->srscale_stream);
        self->srscale_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Degrade_members[] = {
{"server", T_OBJECT_EX, offsetof(Degrade, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Degrade, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(Degrade, input), 0, "Input sound object."},
{"bitdepth", T_OBJECT_EX, offsetof(Degrade, bitdepth), 0, "Number of bits for amplitude values."},
{"srscale", T_OBJECT_EX, offsetof(Degrade, srscale), 0, "Sampling depth factor."},
{"mul", T_OBJECT_EX, offsetof(Degrade, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Degrade, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Degrade_methods[] = {
{"getServer", (PyCFunction)Degrade_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Degrade_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Degrade_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Degrade_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Degrade_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Degrade_stop, METH_NOARGS, "Stops computing."},
{"setBitdepth", (PyCFunction)Degrade_setBitdepth, METH_O, "Sets the bitdepth value."},
{"setSrscale", (PyCFunction)Degrade_setSrscale, METH_O, "Sets the srscale value."},
{"setMul", (PyCFunction)Degrade_setMul, METH_O, "Sets oscillator mul factor."},
{"setAdd", (PyCFunction)Degrade_setAdd, METH_O, "Sets oscillator add factor."},
{"setSub", (PyCFunction)Degrade_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Degrade_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Degrade_as_number = {
(binaryfunc)Degrade_add,                      /*nb_add*/
(binaryfunc)Degrade_sub,                 /*nb_subtract*/
(binaryfunc)Degrade_multiply,                 /*nb_multiply*/
(binaryfunc)Degrade_div,                   /*nb_divide*/
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
(binaryfunc)Degrade_inplace_add,              /*inplace_add*/
(binaryfunc)Degrade_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Degrade_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Degrade_inplace_div,           /*inplace_divide*/
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

PyTypeObject DegradeType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Degrade_base",         /*tp_name*/
sizeof(Degrade),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Degrade_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Degrade_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Degrade objects. Applies different bitdepth and sr on a signal.",           /* tp_doc */
(traverseproc)Degrade_traverse,   /* tp_traverse */
(inquiry)Degrade_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Degrade_methods,             /* tp_methods */
Degrade_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Degrade_init,      /* tp_init */
0,                         /* tp_alloc */
Degrade_new,                 /* tp_new */
};

