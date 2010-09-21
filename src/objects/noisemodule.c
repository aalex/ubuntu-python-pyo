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

typedef struct {
    pyo_audio_HEAD
    int modebuffer[2];
} Noise;

static void
Noise_generate(Noise *self) {
    float val;
    int i;

    for (i=0; i<self->bufsize; i++) {
        val = rand()/((float)(RAND_MAX)+1)*1.98-0.99;
        self->data[i] = val;
    }
}

static void Noise_postprocessing_ii(Noise *self) { POST_PROCESSING_II };
static void Noise_postprocessing_ai(Noise *self) { POST_PROCESSING_AI };
static void Noise_postprocessing_ia(Noise *self) { POST_PROCESSING_IA };
static void Noise_postprocessing_aa(Noise *self) { POST_PROCESSING_AA };
static void Noise_postprocessing_ireva(Noise *self) { POST_PROCESSING_IREVA };
static void Noise_postprocessing_areva(Noise *self) { POST_PROCESSING_AREVA };
static void Noise_postprocessing_revai(Noise *self) { POST_PROCESSING_REVAI };
static void Noise_postprocessing_revaa(Noise *self) { POST_PROCESSING_REVAA };
static void Noise_postprocessing_revareva(Noise *self) { POST_PROCESSING_REVAREVA };

static void
Noise_setProcMode(Noise *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Noise_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Noise_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Noise_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Noise_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Noise_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Noise_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Noise_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Noise_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Noise_postprocessing_revareva;
            break;
    }
}

static void
Noise_compute_next_data_frame(Noise *self)
{
    Noise_generate(self); 
    (*self->muladd_func_ptr)(self);
    Stream_setData(self->stream, self->data);
}

static int
Noise_traverse(Noise *self, visitproc visit, void *arg)
{
    pyo_VISIT
    return 0;
}

static int 
Noise_clear(Noise *self)
{
    pyo_CLEAR
    return 0;
}

static void
Noise_dealloc(Noise* self)
{
    free(self->data);
    Noise_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Noise_deleteStream(Noise *self) { DELETE_STREAM };

static PyObject *
Noise_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Noise *self;
    self = (Noise *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Noise_compute_next_data_frame);
    self->mode_func_ptr = Noise_setProcMode;
    
    return (PyObject *)self;
}

static int
Noise_init(Noise *self, PyObject *args, PyObject *kwds)
{
    PyObject *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &multmp, &addtmp))
        return -1; 
 
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }
    
    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    (*self->mode_func_ptr)(self);
    
    srand((unsigned)(time(0)));
    Noise_compute_next_data_frame((Noise *)self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * Noise_getServer(Noise* self) { GET_SERVER };
static PyObject * Noise_getStream(Noise* self) { GET_STREAM };
static PyObject * Noise_setMul(Noise *self, PyObject *arg) { SET_MUL };	
static PyObject * Noise_setAdd(Noise *self, PyObject *arg) { SET_ADD };	
static PyObject * Noise_setSub(Noise *self, PyObject *arg) { SET_SUB };	
static PyObject * Noise_setDiv(Noise *self, PyObject *arg) { SET_DIV };	

static PyObject * Noise_play(Noise *self) { PLAY };
static PyObject * Noise_out(Noise *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Noise_stop(Noise *self) { STOP };

static PyObject * Noise_multiply(Noise *self, PyObject *arg) { MULTIPLY };
static PyObject * Noise_inplace_multiply(Noise *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Noise_add(Noise *self, PyObject *arg) { ADD };
static PyObject * Noise_inplace_add(Noise *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Noise_sub(Noise *self, PyObject *arg) { SUB };
static PyObject * Noise_inplace_sub(Noise *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Noise_div(Noise *self, PyObject *arg) { DIV };
static PyObject * Noise_inplace_div(Noise *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef Noise_members[] = {
{"server", T_OBJECT_EX, offsetof(Noise, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(Noise, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(Noise, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(Noise, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef Noise_methods[] = {
{"getServer", (PyCFunction)Noise_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)Noise_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)Noise_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)Noise_play, METH_NOARGS, "Starts computing without sending sound to soundcard."},
{"out", (PyCFunction)Noise_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
{"stop", (PyCFunction)Noise_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)Noise_setMul, METH_O, "Sets Noise mul factor."},
{"setAdd", (PyCFunction)Noise_setAdd, METH_O, "Sets Noise add factor."},
{"setSub", (PyCFunction)Noise_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)Noise_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods Noise_as_number = {
(binaryfunc)Noise_add,                      /*nb_add*/
(binaryfunc)Noise_sub,                 /*nb_subtract*/
(binaryfunc)Noise_multiply,                 /*nb_multiply*/
(binaryfunc)Noise_div,                   /*nb_divide*/
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
(binaryfunc)Noise_inplace_add,              /*inplace_add*/
(binaryfunc)Noise_inplace_sub,         /*inplace_subtract*/
(binaryfunc)Noise_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)Noise_inplace_div,           /*inplace_divide*/
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

PyTypeObject NoiseType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.Noise_base",         /*tp_name*/
sizeof(Noise),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)Noise_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&Noise_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"Noise objects. White noise generator.",           /* tp_doc */
(traverseproc)Noise_traverse,   /* tp_traverse */
(inquiry)Noise_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
Noise_methods,             /* tp_methods */
Noise_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)Noise_init,      /* tp_init */
0,                         /* tp_alloc */
Noise_new,                 /* tp_new */
};

