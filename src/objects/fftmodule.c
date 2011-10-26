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
#include "fft.h"
#include "wind.h"

int isPowerOfTwo(int x) {
    return (x != 0) && ((x & (x - 1)) == 0);
}

typedef struct {
    pyo_audio_HEAD
    PyObject *input;
    Stream *input_stream;
    int size;
    int hsize;
    int hopsize;
    int wintype;
    int incount;
    MYFLT *inframe;
    MYFLT *outframe;    
    MYFLT *window;
    MYFLT **twiddle;
    MYFLT *twiddle2;
    MYFLT *buffer_streams;
} FFTMain;

static void
FFTMain_realloc_memories(FFTMain *self) {
    int i, n8;
    self->hsize = self->size / 2;
    n8 = self->size >> 3;
    self->inframe = (MYFLT *)realloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size * sizeof(MYFLT));    
    for (i=0; i<self->size; i++)
        self->inframe[i] = self->outframe[i] = 0.0;
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, 3 * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->bufsize*3); i++)
        self->buffer_streams[i] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size);
    self->twiddle2 = (MYFLT *)realloc(self->twiddle2, self->size * sizeof(MYFLT));
    fft_compute_radix2_twiddle(self->twiddle2, self->size);
    self->window = (MYFLT *)realloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);
    self->incount = -self->hopsize;
}

static void
FFTMain_filters(FFTMain *self) {
    int i, incount;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
   
    incount = self->incount;

    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            self->inframe[incount] = in[i] * self->window[incount];
            if (incount < self->hsize) {
                self->buffer_streams[i] = self->outframe[incount];
                if (incount)
                    self->buffer_streams[i+self->bufsize] = self->outframe[self->size - incount];
                else
                    self->buffer_streams[i+self->bufsize] = 0.0;
            }
            else if (incount == self->hsize)
                self->buffer_streams[i] = self->outframe[incount];
            else
                self->buffer_streams[i] = self->buffer_streams[i+self->bufsize] = 0.0;
            self->buffer_streams[i+self->bufsize*2] = (MYFLT)incount;
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;      
            realfft_split(self->inframe, self->outframe, self->size, self->twiddle);
        }
    } 

    /*
    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            self->inframe[incount] = in[i] * self->window[incount];
            if (incount < self->hsize) {
                self->buffer_streams[i] = self->outframe[incount*2];
                self->buffer_streams[i+self->bufsize] = self->outframe[incount*2+1];
            }
            else
                self->buffer_streams[i] = self->buffer_streams[i+self->bufsize] = 0.0; 
            self->buffer_streams[i+self->bufsize*2] = (MYFLT)incount;
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;      
            realfft_packed(self->inframe, self->outframe, self->size, self->twiddle2);
        }
    }
    */
    self->incount = incount;
}

MYFLT *
FFTMain_getSamplesBuffer(FFTMain *self)
{
    return (MYFLT *)self->buffer_streams;
}    

static void
FFTMain_setProcMode(FFTMain *self)
{        
    self->proc_func_ptr = FFTMain_filters;  
}

static void
FFTMain_compute_next_data_frame(FFTMain *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
FFTMain_traverse(FFTMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    return 0;
}

static int 
FFTMain_clear(FFTMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    return 0;
}

static void
FFTMain_dealloc(FFTMain* self)
{
    int i;
    free(self->data);
    free(self->inframe);
    free(self->outframe);
    free(self->window);
    free(self->buffer_streams);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    free(self->twiddle2);
    FFTMain_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * FFTMain_deleteStream(FFTMain *self) { DELETE_STREAM };

static PyObject *
FFTMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    FFTMain *self;
    self = (FFTMain *)type->tp_alloc(type, 0);
    
    self->size = 1024;
    self->wintype = 2;
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FFTMain_compute_next_data_frame);
    self->mode_func_ptr = FFTMain_setProcMode;

    return (PyObject *)self;
}

static int
FFTMain_init(FFTMain *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp;
    
    static char *kwlist[] = {"input", "size", "hopsize", "wintype", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|iii", kwlist, &inputtmp, &self->size, &self->hopsize, &self->wintype))
        return -1; 

    INIT_INPUT_STREAM
 
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    FFTMain_realloc_memories(self);

    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * FFTMain_getServer(FFTMain* self) { GET_SERVER };
static PyObject * FFTMain_getStream(FFTMain* self) { GET_STREAM };

static PyObject * FFTMain_play(FFTMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FFTMain_stop(FFTMain *self) { STOP };

static PyObject *
FFTMain_setSize(FFTMain *self, PyObject *args, PyObject *kwds)
{
    int size, hopsize;    

    static char *kwlist[] = {"size", "hopsize", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &size, &hopsize)) {
	    Py_INCREF(Py_None);
	    return Py_None;
    }

    if (isPowerOfTwo(size)) {
        self->size = size;
        self->hopsize = hopsize;
        FFTMain_realloc_memories(self);
    }
    else
        printf("FFT size must be a power of two!\n");
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
FFTMain_setWinType(FFTMain *self, PyObject *arg)
{	
	if (PyLong_Check(arg) || PyInt_Check(arg)) {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }    
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef FFTMain_members[] = {
{"server", T_OBJECT_EX, offsetof(FFTMain, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(FFTMain, stream), 0, "Stream object."},
{"input", T_OBJECT_EX, offsetof(FFTMain, input), 0, "FFT sound object."},
{NULL}  /* Sentinel */
};

static PyMethodDef FFTMain_methods[] = {
{"getServer", (PyCFunction)FFTMain_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)FFTMain_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)FFTMain_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)FFTMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)FFTMain_stop, METH_NOARGS, "Stops computing."},
{"setSize", (PyCFunction)FFTMain_setSize, METH_VARARGS|METH_KEYWORDS, "Sets a new FFT size."},
{"setWinType", (PyCFunction)FFTMain_setWinType, METH_O, "Sets a new window."},
{NULL}  /* Sentinel */
};

PyTypeObject FFTMainType = {
PyObject_HEAD_INIT(NULL)
0,                                              /*ob_size*/
"_pyo.FFTMain_base",                                   /*tp_name*/
sizeof(FFTMain),                                 /*tp_basicsize*/
0,                                              /*tp_itemsize*/
(destructor)FFTMain_dealloc,                     /*tp_dealloc*/
0,                                              /*tp_print*/
0,                                              /*tp_getattr*/
0,                                              /*tp_setattr*/
0,                                              /*tp_compare*/
0,                                              /*tp_repr*/
0,                              /*tp_as_number*/
0,                                              /*tp_as_sequence*/
0,                                              /*tp_as_mapping*/
0,                                              /*tp_hash */
0,                                              /*tp_call*/
0,                                              /*tp_str*/
0,                                              /*tp_getattro*/
0,                                              /*tp_setattro*/
0,                                              /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
"FFTMain objects. FFT transform.",           /* tp_doc */
(traverseproc)FFTMain_traverse,                  /* tp_traverse */
(inquiry)FFTMain_clear,                          /* tp_clear */
0,                                              /* tp_richcompare */
0,                                              /* tp_weaklistoffset */
0,                                              /* tp_iter */
0,                                              /* tp_iternext */
FFTMain_methods,                                 /* tp_methods */
FFTMain_members,                                 /* tp_members */
0,                                              /* tp_getset */
0,                                              /* tp_base */
0,                                              /* tp_dict */
0,                                              /* tp_descr_get */
0,                                              /* tp_descr_set */
0,                                              /* tp_dictoffset */
(initproc)FFTMain_init,                          /* tp_init */
0,                                              /* tp_alloc */
FFTMain_new,                                     /* tp_new */
};

/************************************************************************************************/
/* FFT streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FFTMain *mainSplitter;
    int modebuffer[2];
    int chnl; // 0 = real, 1 = imag, 2 = bin
} FFT;

static void FFT_postprocessing_ii(FFT *self) { POST_PROCESSING_II };
static void FFT_postprocessing_ai(FFT *self) { POST_PROCESSING_AI };
static void FFT_postprocessing_ia(FFT *self) { POST_PROCESSING_IA };
static void FFT_postprocessing_aa(FFT *self) { POST_PROCESSING_AA };
static void FFT_postprocessing_ireva(FFT *self) { POST_PROCESSING_IREVA };
static void FFT_postprocessing_areva(FFT *self) { POST_PROCESSING_AREVA };
static void FFT_postprocessing_revai(FFT *self) { POST_PROCESSING_REVAI };
static void FFT_postprocessing_revaa(FFT *self) { POST_PROCESSING_REVAA };
static void FFT_postprocessing_revareva(FFT *self) { POST_PROCESSING_REVAREVA };

static void
FFT_setProcMode(FFT *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = FFT_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = FFT_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = FFT_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = FFT_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = FFT_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = FFT_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = FFT_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = FFT_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = FFT_postprocessing_revareva;
            break;
    }
}

static void
FFT_compute_next_data_frame(FFT *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FFTMain_getSamplesBuffer((FFTMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
}

static int
FFT_traverse(FFT *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int 
FFT_clear(FFT *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);    
    return 0;
}

static void
FFT_dealloc(FFT* self)
{
    free(self->data);
    FFT_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * FFT_deleteStream(FFT *self) { DELETE_STREAM };

static PyObject *
FFT_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    FFT *self;
    self = (FFT *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FFT_compute_next_data_frame);
    self->mode_func_ptr = FFT_setProcMode;
    
    return (PyObject *)self;
}

static int
FFT_init(FFT *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FFTMain *)maintmp;
    
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

static PyObject * FFT_getServer(FFT* self) { GET_SERVER };
static PyObject * FFT_getStream(FFT* self) { GET_STREAM };
static PyObject * FFT_setMul(FFT *self, PyObject *arg) { SET_MUL };	
static PyObject * FFT_setAdd(FFT *self, PyObject *arg) { SET_ADD };	
static PyObject * FFT_setSub(FFT *self, PyObject *arg) { SET_SUB };	
static PyObject * FFT_setDiv(FFT *self, PyObject *arg) { SET_DIV };	

static PyObject * FFT_play(FFT *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FFT_stop(FFT *self) { STOP };

static PyObject * FFT_multiply(FFT *self, PyObject *arg) { MULTIPLY };
static PyObject * FFT_inplace_multiply(FFT *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FFT_add(FFT *self, PyObject *arg) { ADD };
static PyObject * FFT_inplace_add(FFT *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FFT_sub(FFT *self, PyObject *arg) { SUB };
static PyObject * FFT_inplace_sub(FFT *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FFT_div(FFT *self, PyObject *arg) { DIV };
static PyObject * FFT_inplace_div(FFT *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FFT_members[] = {
{"server", T_OBJECT_EX, offsetof(FFT, server), 0, "Pyo server."},
{"stream", T_OBJECT_EX, offsetof(FFT, stream), 0, "Stream object."},
{"mul", T_OBJECT_EX, offsetof(FFT, mul), 0, "Mul factor."},
{"add", T_OBJECT_EX, offsetof(FFT, add), 0, "Add factor."},
{NULL}  /* Sentinel */
};

static PyMethodDef FFT_methods[] = {
{"getServer", (PyCFunction)FFT_getServer, METH_NOARGS, "Returns server object."},
{"_getStream", (PyCFunction)FFT_getStream, METH_NOARGS, "Returns stream object."},
{"deleteStream", (PyCFunction)FFT_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
{"play", (PyCFunction)FFT_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
{"stop", (PyCFunction)FFT_stop, METH_NOARGS, "Stops computing."},
{"setMul", (PyCFunction)FFT_setMul, METH_O, "Sets FFT mul factor."},
{"setAdd", (PyCFunction)FFT_setAdd, METH_O, "Sets FFT add factor."},
{"setSub", (PyCFunction)FFT_setSub, METH_O, "Sets inverse add factor."},
{"setDiv", (PyCFunction)FFT_setDiv, METH_O, "Sets inverse mul factor."},
{NULL}  /* Sentinel */
};

static PyNumberMethods FFT_as_number = {
(binaryfunc)FFT_add,                      /*nb_add*/
(binaryfunc)FFT_sub,                 /*nb_subtract*/
(binaryfunc)FFT_multiply,                 /*nb_multiply*/
(binaryfunc)FFT_div,                   /*nb_divide*/
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
(binaryfunc)FFT_inplace_add,              /*inplace_add*/
(binaryfunc)FFT_inplace_sub,         /*inplace_subtract*/
(binaryfunc)FFT_inplace_multiply,         /*inplace_multiply*/
(binaryfunc)FFT_inplace_div,           /*inplace_divide*/
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

PyTypeObject FFTType = {
PyObject_HEAD_INIT(NULL)
0,                         /*ob_size*/
"_pyo.FFT_base",         /*tp_name*/
sizeof(FFT),         /*tp_basicsize*/
0,                         /*tp_itemsize*/
(destructor)FFT_dealloc, /*tp_dealloc*/
0,                         /*tp_print*/
0,                         /*tp_getattr*/
0,                         /*tp_setattr*/
0,                         /*tp_compare*/
0,                         /*tp_repr*/
&FFT_as_number,             /*tp_as_number*/
0,                         /*tp_as_sequence*/
0,                         /*tp_as_mapping*/
0,                         /*tp_hash */
0,                         /*tp_call*/
0,                         /*tp_str*/
0,                         /*tp_getattro*/
0,                         /*tp_setattro*/
0,                         /*tp_as_buffer*/
Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
"FFT objects. Reads one band (real, imag or bins) from a FFT transform.",           /* tp_doc */
(traverseproc)FFT_traverse,   /* tp_traverse */
(inquiry)FFT_clear,           /* tp_clear */
0,		               /* tp_richcompare */
0,		               /* tp_weaklistoffset */
0,		               /* tp_iter */
0,		               /* tp_iternext */
FFT_methods,             /* tp_methods */
FFT_members,             /* tp_members */
0,                      /* tp_getset */
0,                         /* tp_base */
0,                         /* tp_dict */
0,                         /* tp_descr_get */
0,                         /* tp_descr_set */
0,                         /* tp_dictoffset */
(initproc)FFT_init,      /* tp_init */
0,                         /* tp_alloc */
FFT_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *inreal;
    Stream *inreal_stream;
    PyObject *inimag;
    Stream *inimag_stream;
    int size;
    int hsize;
    int hopsize;
    int wintype;
    int incount;
    MYFLT *inframe;
    MYFLT *outframe;    
    MYFLT *window;
    MYFLT **twiddle;
    MYFLT *twiddle2;
    int modebuffer[2];
} IFFT;

static void
IFFT_realloc_memories(IFFT *self) {
    int i, n8;
    self->hsize = self->size / 2;
    n8 = self->size >> 3;
    self->inframe = (MYFLT *)realloc(self->inframe, self->size * sizeof(MYFLT));
    self->outframe = (MYFLT *)realloc(self->outframe, self->size * sizeof(MYFLT));    
    for (i=0; i<self->size; i++)
        self->inframe[i] = self->outframe[i] = 0.0;
    self->twiddle = (MYFLT **)realloc(self->twiddle, 4 * sizeof(MYFLT));
    for(i=0; i<4; i++)
        self->twiddle[i] = (MYFLT *)malloc(n8 * sizeof(MYFLT));
    fft_compute_split_twiddle(self->twiddle, self->size);
    self->twiddle2 = (MYFLT *)realloc(self->twiddle2, self->size * sizeof(MYFLT));
    fft_compute_radix2_twiddle(self->twiddle2, self->size);
    self->window = (MYFLT *)realloc(self->window, self->size * sizeof(MYFLT));
    gen_window(self->window, self->size, self->wintype);
    self->incount = -self->hopsize;
}

static void
IFFT_filters(IFFT *self) {
    int i, incount;
    MYFLT data;
    MYFLT *inreal = Stream_getData((Stream *)self->inreal_stream);
    MYFLT *inimag = Stream_getData((Stream *)self->inimag_stream);

    incount = self->incount;
    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            if (incount < self->hsize) {
                self->inframe[incount] = inreal[i];
                if (incount)
                    self->inframe[self->size - incount] = inimag[i];
            }
            else if (incount == self->hsize)
                self->inframe[incount] = inreal[i];
            data = self->outframe[incount] * self->window[incount];
            self->data[i] = data;
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;      
            irealfft_split(self->inframe, self->outframe, self->size, self->twiddle);
        }
    }
    /*
    for (i=0; i<self->bufsize; i++) {
        if (incount >= 0) {
            if (incount < self->hsize) {
                self->inframe[incount*2] = inreal[i];
                self->inframe[incount*2+1] = inimag[i];
            }
            self->data[i] = self->outframe[incount] * self->window[incount];
        }
        incount++;
        if (incount >= self->size) {
            incount -= self->size;      
            irealfft_packed(self->inframe, self->outframe, self->size, self->twiddle2);
        }
    }
    */ 
    self->incount = incount;
}

static void IFFT_postprocessing_ii(IFFT *self) { POST_PROCESSING_II };
static void IFFT_postprocessing_ai(IFFT *self) { POST_PROCESSING_AI };
static void IFFT_postprocessing_ia(IFFT *self) { POST_PROCESSING_IA };
static void IFFT_postprocessing_aa(IFFT *self) { POST_PROCESSING_AA };
static void IFFT_postprocessing_ireva(IFFT *self) { POST_PROCESSING_IREVA };
static void IFFT_postprocessing_areva(IFFT *self) { POST_PROCESSING_AREVA };
static void IFFT_postprocessing_revai(IFFT *self) { POST_PROCESSING_REVAI };
static void IFFT_postprocessing_revaa(IFFT *self) { POST_PROCESSING_REVAA };
static void IFFT_postprocessing_revareva(IFFT *self) { POST_PROCESSING_REVAREVA };

static void
IFFT_setProcMode(IFFT *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

    self->proc_func_ptr = IFFT_filters;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = IFFT_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = IFFT_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = IFFT_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = IFFT_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = IFFT_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = IFFT_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = IFFT_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = IFFT_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = IFFT_postprocessing_revareva;
            break;
    } 
}

static void
IFFT_compute_next_data_frame(IFFT *self)
{   
    (*self->proc_func_ptr)(self);    
    (*self->muladd_func_ptr)(self);
}

static int
IFFT_traverse(IFFT *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->inreal);
    Py_VISIT(self->inreal_stream);
    Py_VISIT(self->inimag);
    Py_VISIT(self->inimag_stream);
    return 0;
}

static int 
IFFT_clear(IFFT *self)
{
    pyo_CLEAR
    Py_CLEAR(self->inreal);
    Py_CLEAR(self->inreal_stream);
    Py_CLEAR(self->inimag);
    Py_CLEAR(self->inimag_stream);
    return 0;
}

static void
IFFT_dealloc(IFFT* self)
{
    int i;
    free(self->data);
    free(self->inframe);
    free(self->outframe);
    free(self->window);
    for(i=0; i<4; i++) {
        free(self->twiddle[i]);
    }
    free(self->twiddle);
    free(self->twiddle2);

    IFFT_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * IFFT_deleteStream(IFFT *self) { DELETE_STREAM };

static PyObject *
IFFT_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    IFFT *self;
    self = (IFFT *)type->tp_alloc(type, 0);

    self->size = 1024;
    self->wintype = 2;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, IFFT_compute_next_data_frame);
    self->mode_func_ptr = IFFT_setProcMode;
    
    return (PyObject *)self;
}

static int
IFFT_init(IFFT *self, PyObject *args, PyObject *kwds)
{
    PyObject *inrealtmp, *inreal_streamtmp, *inimagtmp, *inimag_streamtmp, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"inreal", "inimag", "size", "hopsize", "wintype", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OO|iiiOO", kwlist, &inrealtmp, &inimagtmp, &self->size, &self->hopsize, &self->wintype, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->inimag);
    self->inimag = inimagtmp;
    inimag_streamtmp = PyObject_CallMethod((PyObject *)self->inimag, "_getStream", NULL);
    Py_INCREF(inimag_streamtmp);
    Py_XDECREF(self->inimag_stream);
    self->inimag_stream = (Stream *)inimag_streamtmp;

    Py_XDECREF(self->inreal);
    self->inreal = inrealtmp;
    inreal_streamtmp = PyObject_CallMethod((PyObject *)self->inreal, "_getStream", NULL);
    Py_INCREF(inreal_streamtmp);
    Py_XDECREF(self->inreal_stream);
    self->inreal_stream = (Stream *)inreal_streamtmp;
 
    if (multmp) {
        PyObject_CallMethod((PyObject *)self, "setMul", "O", multmp);
    }

    if (addtmp) {
        PyObject_CallMethod((PyObject *)self, "setAdd", "O", addtmp);
    }
            
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);

    IFFT_realloc_memories(self);    

    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * IFFT_getServer(IFFT* self) { GET_SERVER };
static PyObject * IFFT_getStream(IFFT* self) { GET_STREAM };
static PyObject * IFFT_setMul(IFFT *self, PyObject *arg) { SET_MUL };	
static PyObject * IFFT_setAdd(IFFT *self, PyObject *arg) { SET_ADD };	
static PyObject * IFFT_setSub(IFFT *self, PyObject *arg) { SET_SUB };	
static PyObject * IFFT_setDiv(IFFT *self, PyObject *arg) { SET_DIV };	

static PyObject * IFFT_play(IFFT *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * IFFT_out(IFFT *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * IFFT_stop(IFFT *self) { STOP };

static PyObject * IFFT_multiply(IFFT *self, PyObject *arg) { MULTIPLY };
static PyObject * IFFT_inplace_multiply(IFFT *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * IFFT_add(IFFT *self, PyObject *arg) { ADD };
static PyObject * IFFT_inplace_add(IFFT *self, PyObject *arg) { INPLACE_ADD };
static PyObject * IFFT_sub(IFFT *self, PyObject *arg) { SUB };
static PyObject * IFFT_inplace_sub(IFFT *self, PyObject *arg) { INPLACE_SUB };
static PyObject * IFFT_div(IFFT *self, PyObject *arg) { DIV };
static PyObject * IFFT_inplace_div(IFFT *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
IFFT_setSize(IFFT *self, PyObject *args, PyObject *kwds)
{
    int size, hopsize;    

    static char *kwlist[] = {"size", "hopsize", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &size, &hopsize)) {
	    Py_INCREF(Py_None);
	    return Py_None;
    }

    if (isPowerOfTwo(size)) {
        self->size = size;
        self->hopsize = hopsize;
        IFFT_realloc_memories(self);
    }
    else
        printf("IFFT size must be a power of two!\n");
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
IFFT_setWinType(IFFT *self, PyObject *arg)
{	
	if (PyLong_Check(arg) || PyInt_Check(arg)) {
        self->wintype = PyLong_AsLong(arg);
        gen_window(self->window, self->size, self->wintype);
    }    
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef IFFT_members[] = {
    {"server", T_OBJECT_EX, offsetof(IFFT, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(IFFT, stream), 0, "Stream object."},
    {"inreal", T_OBJECT_EX, offsetof(IFFT, inreal), 0, "Real input."},
    {"inimag", T_OBJECT_EX, offsetof(IFFT, inimag), 0, "Imaginary input."},
    {"mul", T_OBJECT_EX, offsetof(IFFT, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(IFFT, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef IFFT_methods[] = {
    {"getServer", (PyCFunction)IFFT_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)IFFT_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)IFFT_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)IFFT_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)IFFT_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)IFFT_stop, METH_NOARGS, "Stops computing."},
    {"setSize", (PyCFunction)IFFT_setSize, METH_VARARGS|METH_KEYWORDS, "Sets a new IFFT size."},
    {"setWinType", (PyCFunction)IFFT_setWinType, METH_O, "Sets a new window."},
	{"setMul", (PyCFunction)IFFT_setMul, METH_O, "Sets oscillator mul factor."},
	{"setAdd", (PyCFunction)IFFT_setAdd, METH_O, "Sets oscillator add factor."},
    {"setSub", (PyCFunction)IFFT_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)IFFT_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods IFFT_as_number = {
    (binaryfunc)IFFT_add,                      /*nb_add*/
    (binaryfunc)IFFT_sub,                 /*nb_subtract*/
    (binaryfunc)IFFT_multiply,                 /*nb_multiply*/
    (binaryfunc)IFFT_div,                   /*nb_divide*/
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
    (binaryfunc)IFFT_inplace_add,              /*inplace_add*/
    (binaryfunc)IFFT_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)IFFT_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)IFFT_inplace_div,           /*inplace_divide*/
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

PyTypeObject IFFTType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.IFFT_base",         /*tp_name*/
    sizeof(IFFT),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)IFFT_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &IFFT_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "IFFT objects. Synthesize audio from an FFT real and imaginary parts.",           /* tp_doc */
    (traverseproc)IFFT_traverse,   /* tp_traverse */
    (inquiry)IFFT_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    IFFT_methods,             /* tp_methods */
    IFFT_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)IFFT_init,      /* tp_init */
    0,                         /* tp_alloc */
    IFFT_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input; /* real input */
    Stream *input_stream;
    PyObject *input2; /* imag input */
    Stream *input2_stream;
    int modebuffer[2];
    int chnl; // 0 = mag, 1 = ang
} CarToPol;

static void
CarToPol_generate(CarToPol *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);

    if (self->chnl == 0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYSQRT(in[i]*in[i] + in2[i]*in2[i]);
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = MYATAN2(in2[i], in[i]);
        }
    }
}

static void CarToPol_postprocessing_ii(CarToPol *self) { POST_PROCESSING_II };
static void CarToPol_postprocessing_ai(CarToPol *self) { POST_PROCESSING_AI };
static void CarToPol_postprocessing_ia(CarToPol *self) { POST_PROCESSING_IA };
static void CarToPol_postprocessing_aa(CarToPol *self) { POST_PROCESSING_AA };
static void CarToPol_postprocessing_ireva(CarToPol *self) { POST_PROCESSING_IREVA };
static void CarToPol_postprocessing_areva(CarToPol *self) { POST_PROCESSING_AREVA };
static void CarToPol_postprocessing_revai(CarToPol *self) { POST_PROCESSING_REVAI };
static void CarToPol_postprocessing_revaa(CarToPol *self) { POST_PROCESSING_REVAA };
static void CarToPol_postprocessing_revareva(CarToPol *self) { POST_PROCESSING_REVAREVA };

static void
CarToPol_setProcMode(CarToPol *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = CarToPol_generate;

	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = CarToPol_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = CarToPol_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = CarToPol_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = CarToPol_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = CarToPol_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = CarToPol_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = CarToPol_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = CarToPol_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = CarToPol_postprocessing_revareva;
            break;
    }
}

static void
CarToPol_compute_next_data_frame(CarToPol *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
CarToPol_traverse(CarToPol *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int 
CarToPol_clear(CarToPol *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
CarToPol_dealloc(CarToPol* self)
{
    free(self->data);
    CarToPol_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * CarToPol_deleteStream(CarToPol *self) { DELETE_STREAM };

static PyObject *
CarToPol_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    CarToPol *self;
    self = (CarToPol *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, CarToPol_compute_next_data_frame);
    self->mode_func_ptr = CarToPol_setProcMode;
    
    return (PyObject *)self;
}

static int
CarToPol_init(CarToPol *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "input2", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &input2tmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
    self->input2_stream = (Stream *)input2_streamtmp;
    
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

static PyObject * CarToPol_getServer(CarToPol* self) { GET_SERVER };
static PyObject * CarToPol_getStream(CarToPol* self) { GET_STREAM };
static PyObject * CarToPol_setMul(CarToPol *self, PyObject *arg) { SET_MUL };	
static PyObject * CarToPol_setAdd(CarToPol *self, PyObject *arg) { SET_ADD };	
static PyObject * CarToPol_setSub(CarToPol *self, PyObject *arg) { SET_SUB };	
static PyObject * CarToPol_setDiv(CarToPol *self, PyObject *arg) { SET_DIV };	

static PyObject * CarToPol_play(CarToPol *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * CarToPol_stop(CarToPol *self) { STOP };

static PyObject * CarToPol_multiply(CarToPol *self, PyObject *arg) { MULTIPLY };
static PyObject * CarToPol_inplace_multiply(CarToPol *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * CarToPol_add(CarToPol *self, PyObject *arg) { ADD };
static PyObject * CarToPol_inplace_add(CarToPol *self, PyObject *arg) { INPLACE_ADD };
static PyObject * CarToPol_sub(CarToPol *self, PyObject *arg) { SUB };
static PyObject * CarToPol_inplace_sub(CarToPol *self, PyObject *arg) { INPLACE_SUB };
static PyObject * CarToPol_div(CarToPol *self, PyObject *arg) { DIV };
static PyObject * CarToPol_inplace_div(CarToPol *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef CarToPol_members[] = {
    {"server", T_OBJECT_EX, offsetof(CarToPol, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(CarToPol, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(CarToPol, input), 0, "Real sound object."},
    {"input2", T_OBJECT_EX, offsetof(CarToPol, input2), 0, "Imaginary sound object."},
    {"mul", T_OBJECT_EX, offsetof(CarToPol, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(CarToPol, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef CarToPol_methods[] = {
    {"getServer", (PyCFunction)CarToPol_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)CarToPol_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)CarToPol_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)CarToPol_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)CarToPol_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)CarToPol_setMul, METH_O, "Sets CarToPol mul factor."},
    {"setAdd", (PyCFunction)CarToPol_setAdd, METH_O, "Sets CarToPol add factor."},
    {"setSub", (PyCFunction)CarToPol_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)CarToPol_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods CarToPol_as_number = {
    (binaryfunc)CarToPol_add,                      /*nb_add*/
    (binaryfunc)CarToPol_sub,                 /*nb_subtract*/
    (binaryfunc)CarToPol_multiply,                 /*nb_multiply*/
    (binaryfunc)CarToPol_div,                   /*nb_divide*/
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
    (binaryfunc)CarToPol_inplace_add,              /*inplace_add*/
    (binaryfunc)CarToPol_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)CarToPol_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)CarToPol_inplace_div,           /*inplace_divide*/
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

PyTypeObject CarToPolType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.CarToPol_base",         /*tp_name*/
    sizeof(CarToPol),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)CarToPol_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &CarToPol_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "CarToPol objects. Cartesian to polar transform.",           /* tp_doc */
    (traverseproc)CarToPol_traverse,   /* tp_traverse */
    (inquiry)CarToPol_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    CarToPol_methods,             /* tp_methods */
    CarToPol_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)CarToPol_init,      /* tp_init */
    0,                         /* tp_alloc */
    CarToPol_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input; /* mag input */
    Stream *input_stream;
    PyObject *input2; /* ang input */
    Stream *input2_stream;
    int modebuffer[2];
    int chnl; // 0 = real, 1 = imag
} PolToCar;

static void
PolToCar_generate(PolToCar *self) {
    int i;
    MYFLT *in = Stream_getData((Stream *)self->input_stream);
    MYFLT *in2 = Stream_getData((Stream *)self->input2_stream);
    
    if (self->chnl == 0) {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = in[i] * MYCOS(in2[i]);
        }
    }
    else {
        for (i=0; i<self->bufsize; i++) {
            self->data[i] = in[i] * MYSIN(in2[i]);
        }
    }
}

static void PolToCar_postprocessing_ii(PolToCar *self) { POST_PROCESSING_II };
static void PolToCar_postprocessing_ai(PolToCar *self) { POST_PROCESSING_AI };
static void PolToCar_postprocessing_ia(PolToCar *self) { POST_PROCESSING_IA };
static void PolToCar_postprocessing_aa(PolToCar *self) { POST_PROCESSING_AA };
static void PolToCar_postprocessing_ireva(PolToCar *self) { POST_PROCESSING_IREVA };
static void PolToCar_postprocessing_areva(PolToCar *self) { POST_PROCESSING_AREVA };
static void PolToCar_postprocessing_revai(PolToCar *self) { POST_PROCESSING_REVAI };
static void PolToCar_postprocessing_revaa(PolToCar *self) { POST_PROCESSING_REVAA };
static void PolToCar_postprocessing_revareva(PolToCar *self) { POST_PROCESSING_REVAREVA };

static void
PolToCar_setProcMode(PolToCar *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
    self->proc_func_ptr = PolToCar_generate;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = PolToCar_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = PolToCar_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = PolToCar_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = PolToCar_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = PolToCar_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = PolToCar_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = PolToCar_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = PolToCar_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = PolToCar_postprocessing_revareva;
            break;
    }
}

static void
PolToCar_compute_next_data_frame(PolToCar *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
PolToCar_traverse(PolToCar *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    Py_VISIT(self->input_stream);
    Py_VISIT(self->input2);
    Py_VISIT(self->input2_stream);
    return 0;
}

static int 
PolToCar_clear(PolToCar *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    Py_CLEAR(self->input_stream);
    Py_CLEAR(self->input2);
    Py_CLEAR(self->input2_stream);
    return 0;
}

static void
PolToCar_dealloc(PolToCar* self)
{
    free(self->data);
    PolToCar_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * PolToCar_deleteStream(PolToCar *self) { DELETE_STREAM };

static PyObject *
PolToCar_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    PolToCar *self;
    self = (PolToCar *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, PolToCar_compute_next_data_frame);
    self->mode_func_ptr = PolToCar_setProcMode;
    
    return (PyObject *)self;
}

static int
PolToCar_init(PolToCar *self, PyObject *args, PyObject *kwds)
{
    PyObject *inputtmp, *input_streamtmp, *input2tmp, *input2_streamtmp, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"input", "input2", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "OOi|OO", kwlist, &inputtmp, &input2tmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    INIT_INPUT_STREAM
    
    Py_XDECREF(self->input2);
    self->input2 = input2tmp;
    input2_streamtmp = PyObject_CallMethod((PyObject *)self->input2, "_getStream", NULL);
    Py_INCREF(input2_streamtmp);
    Py_XDECREF(self->input2_stream);
    self->input2_stream = (Stream *)input2_streamtmp;
    
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

static PyObject * PolToCar_getServer(PolToCar* self) { GET_SERVER };
static PyObject * PolToCar_getStream(PolToCar* self) { GET_STREAM };
static PyObject * PolToCar_setMul(PolToCar *self, PyObject *arg) { SET_MUL };	
static PyObject * PolToCar_setAdd(PolToCar *self, PyObject *arg) { SET_ADD };	
static PyObject * PolToCar_setSub(PolToCar *self, PyObject *arg) { SET_SUB };	
static PyObject * PolToCar_setDiv(PolToCar *self, PyObject *arg) { SET_DIV };	

static PyObject * PolToCar_play(PolToCar *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * PolToCar_stop(PolToCar *self) { STOP };

static PyObject * PolToCar_multiply(PolToCar *self, PyObject *arg) { MULTIPLY };
static PyObject * PolToCar_inplace_multiply(PolToCar *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * PolToCar_add(PolToCar *self, PyObject *arg) { ADD };
static PyObject * PolToCar_inplace_add(PolToCar *self, PyObject *arg) { INPLACE_ADD };
static PyObject * PolToCar_sub(PolToCar *self, PyObject *arg) { SUB };
static PyObject * PolToCar_inplace_sub(PolToCar *self, PyObject *arg) { INPLACE_SUB };
static PyObject * PolToCar_div(PolToCar *self, PyObject *arg) { DIV };
static PyObject * PolToCar_inplace_div(PolToCar *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef PolToCar_members[] = {
    {"server", T_OBJECT_EX, offsetof(PolToCar, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(PolToCar, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(PolToCar, input), 0, "Magnitude sound object."},
    {"input2", T_OBJECT_EX, offsetof(PolToCar, input2), 0, "Angle sound object."},
    {"mul", T_OBJECT_EX, offsetof(PolToCar, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(PolToCar, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef PolToCar_methods[] = {
    {"getServer", (PyCFunction)PolToCar_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)PolToCar_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)PolToCar_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)PolToCar_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)PolToCar_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)PolToCar_setMul, METH_O, "Sets PolToCar mul factor."},
    {"setAdd", (PyCFunction)PolToCar_setAdd, METH_O, "Sets PolToCar add factor."},
    {"setSub", (PyCFunction)PolToCar_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)PolToCar_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods PolToCar_as_number = {
    (binaryfunc)PolToCar_add,                      /*nb_add*/
    (binaryfunc)PolToCar_sub,                 /*nb_subtract*/
    (binaryfunc)PolToCar_multiply,                 /*nb_multiply*/
    (binaryfunc)PolToCar_div,                   /*nb_divide*/
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
    (binaryfunc)PolToCar_inplace_add,              /*inplace_add*/
    (binaryfunc)PolToCar_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)PolToCar_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)PolToCar_inplace_div,           /*inplace_divide*/
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

PyTypeObject PolToCarType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.PolToCar_base",         /*tp_name*/
    sizeof(PolToCar),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)PolToCar_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &PolToCar_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "PolToCar objects. Polar to cartesian transform.",           /* tp_doc */
    (traverseproc)PolToCar_traverse,   /* tp_traverse */
    (inquiry)PolToCar_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    PolToCar_methods,             /* tp_methods */
    PolToCar_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PolToCar_init,      /* tp_init */
    0,                         /* tp_alloc */
    PolToCar_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input; 
    int inputSize;
    int modebuffer[2];
    int frameSize; 
    int overlaps;
    int hopsize;
    int count;
    MYFLT **frameBuffer;
    MYFLT *buffer_streams;
} FrameDeltaMain;

static void
FrameDeltaMain_generate(FrameDeltaMain *self) {
    int i, j, which, where;
    MYFLT curPhase, lastPhase, diff;

    MYFLT ins[self->overlaps][self->bufsize];
    for (j=0; j<self->overlaps; j++) {
        MYFLT *in = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, j), "_getStream", NULL));
        for (i=0; i<self->bufsize; i++) {
            ins[j][i] = in[i];
        }
    }   
    for (i=0; i<self->bufsize; i++) {
        for (j=0; j<self->overlaps; j++) {
            curPhase = ins[j][i];
            which = j - 1;
            if (which < 0)
                which = self->overlaps - 1;
            where = self->count - self->hopsize;
            if (where < 0)
                where += self->frameSize;
            lastPhase = self->frameBuffer[which][where];
            diff = curPhase - lastPhase;
            while (diff < -PI) {
                diff += TWOPI;
            }
            while (diff > PI) {
                diff -= TWOPI;
            }
            self->frameBuffer[j][self->count] = curPhase;            
            self->buffer_streams[i+j*self->bufsize] = diff;
        }
        self->count++;
        if (self->count >= self->frameSize)
            self->count = 0;
    }
}

MYFLT *
FrameDeltaMain_getSamplesBuffer(FrameDeltaMain *self)
{
    return (MYFLT *)self->buffer_streams;
}    

static void
FrameDeltaMain_setProcMode(FrameDeltaMain *self)
{    
    self->proc_func_ptr = FrameDeltaMain_generate;
}

static void
FrameDeltaMain_compute_next_data_frame(FrameDeltaMain *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
FrameDeltaMain_traverse(FrameDeltaMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int 
FrameDeltaMain_clear(FrameDeltaMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
FrameDeltaMain_dealloc(FrameDeltaMain* self)
{
    int i;
    free(self->data);
    for (i=0; i<self->overlaps; i++) {
        free(self->frameBuffer[i]);
    }
    free(self->frameBuffer);
    free(self->buffer_streams);
    FrameDeltaMain_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * FrameDeltaMain_deleteStream(FrameDeltaMain *self) { DELETE_STREAM };

static PyObject *
FrameDeltaMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    FrameDeltaMain *self;
    self = (FrameDeltaMain *)type->tp_alloc(type, 0);
    
    self->count = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameDeltaMain_compute_next_data_frame);
    self->mode_func_ptr = FrameDeltaMain_setProcMode;
    
    return (PyObject *)self;
}

static int
FrameDeltaMain_init(FrameDeltaMain *self, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp;
    
    static char *kwlist[] = {"input", "frameSize", "overlaps", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oii", kwlist, &inputtmp, &self->frameSize, &self->overlaps))
        return -1; 
    
    if (inputtmp) {
        PyObject_CallMethod((PyObject *)self, "setInput", "O", inputtmp);
    }

    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->hopsize = self->frameSize / self->overlaps;
    self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT));
    for(i=0; i<self->overlaps; i++) {
        self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
        for (j=0; j<self->frameSize; j++) {
            self->frameBuffer[i][j] = 0.0;
        }
    }
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->overlaps * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->overlaps*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }

    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * FrameDeltaMain_getServer(FrameDeltaMain* self) { GET_SERVER };
static PyObject * FrameDeltaMain_getStream(FrameDeltaMain* self) { GET_STREAM };

static PyObject * FrameDeltaMain_play(FrameDeltaMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameDeltaMain_stop(FrameDeltaMain *self) { STOP };

static PyObject *
FrameDeltaMain_setInput(FrameDeltaMain *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The inputs attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    tmp = arg;
    self->inputSize = PyList_Size(tmp);
    Py_INCREF(tmp);
	Py_XDECREF(self->input);
    self->input = tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
FrameDeltaMain_setFrameSize(FrameDeltaMain *self, PyObject *arg)
{    
    int i, j, tmp;

    if (PyInt_Check(arg)) { 
        tmp = PyLong_AsLong(arg);
        if (isPowerOfTwo(tmp)) {
            self->frameSize = tmp;
            self->hopsize = self->frameSize / self->overlaps;

            self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT));
            for(i=0; i<self->overlaps; i++) {
                self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
                for (j=0; j<self->frameSize; j++) {
                    self->frameBuffer[i][j] = 0.0;
                }
            }

            self->count = 0;
        }
    }
    else
        printf("frameSize must be a power of two!\n");
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef FrameDeltaMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameDeltaMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameDeltaMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(FrameDeltaMain, input), 0, "Phase input object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameDeltaMain_methods[] = {
    {"getServer", (PyCFunction)FrameDeltaMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameDeltaMain_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)FrameDeltaMain_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)FrameDeltaMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)FrameDeltaMain_stop, METH_NOARGS, "Stops computing."},
    {"setInput", (PyCFunction)FrameDeltaMain_setInput, METH_O, "Sets list of input streams."},
    {"setFrameSize", (PyCFunction)FrameDeltaMain_setFrameSize, METH_O, "Sets frame size."},
    {NULL}  /* Sentinel */
};

PyTypeObject FrameDeltaMainType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.FrameDeltaMain_base",         /*tp_name*/
    sizeof(FrameDeltaMain),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameDeltaMain_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameDeltaMain objects. Compute the phase difference between successive frames.",           /* tp_doc */
    (traverseproc)FrameDeltaMain_traverse,   /* tp_traverse */
    (inquiry)FrameDeltaMain_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FrameDeltaMain_methods,             /* tp_methods */
    FrameDeltaMain_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)FrameDeltaMain_init,      /* tp_init */
    0,                         /* tp_alloc */
    FrameDeltaMain_new,                 /* tp_new */
};

/************************************************************************************************/
/* FrameDelta streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FrameDeltaMain *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} FrameDelta;

static void FrameDelta_postprocessing_ii(FrameDelta *self) { POST_PROCESSING_II };
static void FrameDelta_postprocessing_ai(FrameDelta *self) { POST_PROCESSING_AI };
static void FrameDelta_postprocessing_ia(FrameDelta *self) { POST_PROCESSING_IA };
static void FrameDelta_postprocessing_aa(FrameDelta *self) { POST_PROCESSING_AA };
static void FrameDelta_postprocessing_ireva(FrameDelta *self) { POST_PROCESSING_IREVA };
static void FrameDelta_postprocessing_areva(FrameDelta *self) { POST_PROCESSING_AREVA };
static void FrameDelta_postprocessing_revai(FrameDelta *self) { POST_PROCESSING_REVAI };
static void FrameDelta_postprocessing_revaa(FrameDelta *self) { POST_PROCESSING_REVAA };
static void FrameDelta_postprocessing_revareva(FrameDelta *self) { POST_PROCESSING_REVAREVA };

static void
FrameDelta_setProcMode(FrameDelta *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = FrameDelta_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = FrameDelta_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = FrameDelta_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = FrameDelta_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = FrameDelta_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = FrameDelta_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = FrameDelta_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = FrameDelta_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = FrameDelta_postprocessing_revareva;
            break;
    }
}

static void
FrameDelta_compute_next_data_frame(FrameDelta *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FrameDeltaMain_getSamplesBuffer((FrameDeltaMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
}

static int
FrameDelta_traverse(FrameDelta *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int 
FrameDelta_clear(FrameDelta *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);    
    return 0;
}

static void
FrameDelta_dealloc(FrameDelta* self)
{
    free(self->data);
    FrameDelta_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * FrameDelta_deleteStream(FrameDelta *self) { DELETE_STREAM };

static PyObject *
FrameDelta_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    FrameDelta *self;
    self = (FrameDelta *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameDelta_compute_next_data_frame);
    self->mode_func_ptr = FrameDelta_setProcMode;
    
    return (PyObject *)self;
}

static int
FrameDelta_init(FrameDelta *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FrameDeltaMain *)maintmp;
    
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

static PyObject * FrameDelta_getServer(FrameDelta* self) { GET_SERVER };
static PyObject * FrameDelta_getStream(FrameDelta* self) { GET_STREAM };
static PyObject * FrameDelta_setMul(FrameDelta *self, PyObject *arg) { SET_MUL };	
static PyObject * FrameDelta_setAdd(FrameDelta *self, PyObject *arg) { SET_ADD };	
static PyObject * FrameDelta_setSub(FrameDelta *self, PyObject *arg) { SET_SUB };	
static PyObject * FrameDelta_setDiv(FrameDelta *self, PyObject *arg) { SET_DIV };	

static PyObject * FrameDelta_play(FrameDelta *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameDelta_out(FrameDelta *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * FrameDelta_stop(FrameDelta *self) { STOP };

static PyObject * FrameDelta_multiply(FrameDelta *self, PyObject *arg) { MULTIPLY };
static PyObject * FrameDelta_inplace_multiply(FrameDelta *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FrameDelta_add(FrameDelta *self, PyObject *arg) { ADD };
static PyObject * FrameDelta_inplace_add(FrameDelta *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FrameDelta_sub(FrameDelta *self, PyObject *arg) { SUB };
static PyObject * FrameDelta_inplace_sub(FrameDelta *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FrameDelta_div(FrameDelta *self, PyObject *arg) { DIV };
static PyObject * FrameDelta_inplace_div(FrameDelta *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FrameDelta_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameDelta, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameDelta, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(FrameDelta, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(FrameDelta, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameDelta_methods[] = {
    {"getServer", (PyCFunction)FrameDelta_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameDelta_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)FrameDelta_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)FrameDelta_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)FrameDelta_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)FrameDelta_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)FrameDelta_setMul, METH_O, "Sets FrameDelta mul factor."},
    {"setAdd", (PyCFunction)FrameDelta_setAdd, METH_O, "Sets FrameDelta add factor."},
    {"setSub", (PyCFunction)FrameDelta_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)FrameDelta_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods FrameDelta_as_number = {
    (binaryfunc)FrameDelta_add,                      /*nb_add*/
    (binaryfunc)FrameDelta_sub,                 /*nb_subtract*/
    (binaryfunc)FrameDelta_multiply,                 /*nb_multiply*/
    (binaryfunc)FrameDelta_div,                   /*nb_divide*/
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
    (binaryfunc)FrameDelta_inplace_add,              /*inplace_add*/
    (binaryfunc)FrameDelta_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)FrameDelta_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)FrameDelta_inplace_div,           /*inplace_divide*/
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

PyTypeObject FrameDeltaType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.FrameDelta_base",         /*tp_name*/
    sizeof(FrameDelta),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameDelta_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &FrameDelta_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameDelta objects. Reads one band from a FrameDeltaMain object.",           /* tp_doc */
    (traverseproc)FrameDelta_traverse,   /* tp_traverse */
    (inquiry)FrameDelta_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FrameDelta_methods,             /* tp_methods */
    FrameDelta_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)FrameDelta_init,      /* tp_init */
    0,                         /* tp_alloc */
    FrameDelta_new,                 /* tp_new */
};

typedef struct {
    pyo_audio_HEAD
    PyObject *input; 
    int inputSize;
    int modebuffer[2];
    int frameSize; 
    int overlaps;
    int hopsize;
    int count;
    MYFLT **frameBuffer;
    MYFLT *buffer_streams;
} FrameAccumMain;

static void
FrameAccumMain_generate(FrameAccumMain *self) {
    int i, j, which, where;
    MYFLT curPhase, lastPhase, diff;
    
    MYFLT ins[self->overlaps][self->bufsize];
    for (j=0; j<self->overlaps; j++) {
        MYFLT *in = Stream_getData((Stream *)PyObject_CallMethod((PyObject *)PyList_GET_ITEM(self->input, j), "_getStream", NULL));
        for (i=0; i<self->bufsize; i++) {
            ins[j][i] = in[i];
        }
    }   
    for (i=0; i<self->bufsize; i++) {
        for (j=0; j<self->overlaps; j++) {
            curPhase = ins[j][i];
            which = j - 1;
            if (which < 0)
                which = self->overlaps - 1;
            where = self->count - self->hopsize;
            if (where < 0)
                where += self->frameSize;
            lastPhase = self->frameBuffer[which][where];
            diff = curPhase + lastPhase;
            self->frameBuffer[j][self->count] = diff;            
            self->buffer_streams[i+j*self->bufsize] = diff;
        }
        self->count++;
        if (self->count >= self->frameSize)
            self->count = 0;
    }
}

MYFLT *
FrameAccumMain_getSamplesBuffer(FrameAccumMain *self)
{
    return (MYFLT *)self->buffer_streams;
}    

static void
FrameAccumMain_setProcMode(FrameAccumMain *self)
{    
    self->proc_func_ptr = FrameAccumMain_generate;
}

static void
FrameAccumMain_compute_next_data_frame(FrameAccumMain *self)
{
    (*self->proc_func_ptr)(self); 
}

static int
FrameAccumMain_traverse(FrameAccumMain *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->input);
    return 0;
}

static int 
FrameAccumMain_clear(FrameAccumMain *self)
{
    pyo_CLEAR
    Py_CLEAR(self->input);
    return 0;
}

static void
FrameAccumMain_dealloc(FrameAccumMain* self)
{
    int i;
    free(self->data);
    for (i=0; i<self->overlaps; i++) {
        free(self->frameBuffer[i]);
    }
    free(self->frameBuffer);
    free(self->buffer_streams);
    FrameAccumMain_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * FrameAccumMain_deleteStream(FrameAccumMain *self) { DELETE_STREAM };

static PyObject *
FrameAccumMain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    FrameAccumMain *self;
    self = (FrameAccumMain *)type->tp_alloc(type, 0);
    
    self->count = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameAccumMain_compute_next_data_frame);
    self->mode_func_ptr = FrameAccumMain_setProcMode;
    
    return (PyObject *)self;
}

static int
FrameAccumMain_init(FrameAccumMain *self, PyObject *args, PyObject *kwds)
{
    int i, j;
    PyObject *inputtmp;
    
    static char *kwlist[] = {"input", "framesize", "overlaps", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oii", kwlist, &inputtmp, &self->frameSize, &self->overlaps))
        return -1; 
    
    if (inputtmp) {
        PyObject_CallMethod((PyObject *)self, "setInput", "O", inputtmp);
    }
    
    Py_INCREF(self->stream);
    PyObject_CallMethod(self->server, "addStream", "O", self->stream);
    
    self->hopsize = self->frameSize / self->overlaps;
    self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT));
    for(i=0; i<self->overlaps; i++) {
        self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
        for (j=0; j<self->frameSize; j++) {
            self->frameBuffer[i][j] = 0.0;
        }
    }
    self->buffer_streams = (MYFLT *)realloc(self->buffer_streams, self->overlaps * self->bufsize * sizeof(MYFLT));
    for (i=0; i<(self->overlaps*self->bufsize); i++) {
        self->buffer_streams[i] = 0.0;
    }
    
    (*self->mode_func_ptr)(self);
    
    Py_INCREF(self);
    return 0;
}

static PyObject * FrameAccumMain_getServer(FrameAccumMain* self) { GET_SERVER };
static PyObject * FrameAccumMain_getStream(FrameAccumMain* self) { GET_STREAM };

static PyObject * FrameAccumMain_play(FrameAccumMain *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameAccumMain_stop(FrameAccumMain *self) { STOP };

static PyObject *
FrameAccumMain_setInput(FrameAccumMain *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (! PyList_Check(arg)) {
        PyErr_SetString(PyExc_TypeError, "The inputs attribute must be a list.");
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    tmp = arg;
    self->inputSize = PyList_Size(tmp);
    Py_INCREF(tmp);
	Py_XDECREF(self->input);
    self->input = tmp;
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
FrameAccumMain_setFrameSize(FrameAccumMain *self, PyObject *arg)
{    
    int i, j, tmp;
    
    if (PyInt_Check(arg)) { 
        tmp = PyLong_AsLong(arg);
        if (isPowerOfTwo(tmp)) {
            self->frameSize = tmp;
            self->hopsize = self->frameSize / self->overlaps;
            
            self->frameBuffer = (MYFLT **)realloc(self->frameBuffer, self->overlaps * sizeof(MYFLT));
            for(i=0; i<self->overlaps; i++) {
                self->frameBuffer[i] = (MYFLT *)malloc(self->frameSize * sizeof(MYFLT));
                for (j=0; j<self->frameSize; j++) {
                    self->frameBuffer[i][j] = 0.0;
                }
            }
            
            self->count = 0;
        }
    }
    else
        printf("frameSize must be a power of two!\n");
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef FrameAccumMain_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameAccumMain, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameAccumMain, stream), 0, "Stream object."},
    {"input", T_OBJECT_EX, offsetof(FrameAccumMain, input), 0, "Phase input object."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameAccumMain_methods[] = {
    {"getServer", (PyCFunction)FrameAccumMain_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameAccumMain_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)FrameAccumMain_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)FrameAccumMain_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"stop", (PyCFunction)FrameAccumMain_stop, METH_NOARGS, "Stops computing."},
    {"setInput", (PyCFunction)FrameAccumMain_setInput, METH_O, "Sets list of input streams."},
    {"setFrameSize", (PyCFunction)FrameAccumMain_setFrameSize, METH_O, "Sets frame size."},
    {NULL}  /* Sentinel */
};

PyTypeObject FrameAccumMainType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.FrameAccumMain_base",         /*tp_name*/
    sizeof(FrameAccumMain),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameAccumMain_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameAccumMain objects. Integrate the phase difference between successive frames.",           /* tp_doc */
    (traverseproc)FrameAccumMain_traverse,   /* tp_traverse */
    (inquiry)FrameAccumMain_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FrameAccumMain_methods,             /* tp_methods */
    FrameAccumMain_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)FrameAccumMain_init,      /* tp_init */
    0,                         /* tp_alloc */
    FrameAccumMain_new,                 /* tp_new */
};

/************************************************************************************************/
/* FrameAccum streamer object */
/************************************************************************************************/
typedef struct {
    pyo_audio_HEAD
    FrameAccumMain *mainSplitter;
    int modebuffer[2];
    int chnl; // panning order
} FrameAccum;

static void FrameAccum_postprocessing_ii(FrameAccum *self) { POST_PROCESSING_II };
static void FrameAccum_postprocessing_ai(FrameAccum *self) { POST_PROCESSING_AI };
static void FrameAccum_postprocessing_ia(FrameAccum *self) { POST_PROCESSING_IA };
static void FrameAccum_postprocessing_aa(FrameAccum *self) { POST_PROCESSING_AA };
static void FrameAccum_postprocessing_ireva(FrameAccum *self) { POST_PROCESSING_IREVA };
static void FrameAccum_postprocessing_areva(FrameAccum *self) { POST_PROCESSING_AREVA };
static void FrameAccum_postprocessing_revai(FrameAccum *self) { POST_PROCESSING_REVAI };
static void FrameAccum_postprocessing_revaa(FrameAccum *self) { POST_PROCESSING_REVAA };
static void FrameAccum_postprocessing_revareva(FrameAccum *self) { POST_PROCESSING_REVAREVA };

static void
FrameAccum_setProcMode(FrameAccum *self)
{
    int muladdmode;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = FrameAccum_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = FrameAccum_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = FrameAccum_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = FrameAccum_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = FrameAccum_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = FrameAccum_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = FrameAccum_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = FrameAccum_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = FrameAccum_postprocessing_revareva;
            break;
    }
}

static void
FrameAccum_compute_next_data_frame(FrameAccum *self)
{
    int i;
    MYFLT *tmp;
    int offset = self->chnl * self->bufsize;
    tmp = FrameAccumMain_getSamplesBuffer((FrameAccumMain *)self->mainSplitter);
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = tmp[i + offset];
    }    
    (*self->muladd_func_ptr)(self);
}

static int
FrameAccum_traverse(FrameAccum *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->mainSplitter);
    return 0;
}

static int 
FrameAccum_clear(FrameAccum *self)
{
    pyo_CLEAR
    Py_CLEAR(self->mainSplitter);    
    return 0;
}

static void
FrameAccum_dealloc(FrameAccum* self)
{
    free(self->data);
    FrameAccum_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * FrameAccum_deleteStream(FrameAccum *self) { DELETE_STREAM };

static PyObject *
FrameAccum_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    FrameAccum *self;
    self = (FrameAccum *)type->tp_alloc(type, 0);
    
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, FrameAccum_compute_next_data_frame);
    self->mode_func_ptr = FrameAccum_setProcMode;
    
    return (PyObject *)self;
}

static int
FrameAccum_init(FrameAccum *self, PyObject *args, PyObject *kwds)
{
    PyObject *maintmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"mainSplitter", "chnl", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oi|OO", kwlist, &maintmp, &self->chnl, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->mainSplitter);
    Py_INCREF(maintmp);
    self->mainSplitter = (FrameAccumMain *)maintmp;
    
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

static PyObject * FrameAccum_getServer(FrameAccum* self) { GET_SERVER };
static PyObject * FrameAccum_getStream(FrameAccum* self) { GET_STREAM };
static PyObject * FrameAccum_setMul(FrameAccum *self, PyObject *arg) { SET_MUL };	
static PyObject * FrameAccum_setAdd(FrameAccum *self, PyObject *arg) { SET_ADD };	
static PyObject * FrameAccum_setSub(FrameAccum *self, PyObject *arg) { SET_SUB };	
static PyObject * FrameAccum_setDiv(FrameAccum *self, PyObject *arg) { SET_DIV };	

static PyObject * FrameAccum_play(FrameAccum *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * FrameAccum_out(FrameAccum *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * FrameAccum_stop(FrameAccum *self) { STOP };

static PyObject * FrameAccum_multiply(FrameAccum *self, PyObject *arg) { MULTIPLY };
static PyObject * FrameAccum_inplace_multiply(FrameAccum *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * FrameAccum_add(FrameAccum *self, PyObject *arg) { ADD };
static PyObject * FrameAccum_inplace_add(FrameAccum *self, PyObject *arg) { INPLACE_ADD };
static PyObject * FrameAccum_sub(FrameAccum *self, PyObject *arg) { SUB };
static PyObject * FrameAccum_inplace_sub(FrameAccum *self, PyObject *arg) { INPLACE_SUB };
static PyObject * FrameAccum_div(FrameAccum *self, PyObject *arg) { DIV };
static PyObject * FrameAccum_inplace_div(FrameAccum *self, PyObject *arg) { INPLACE_DIV };

static PyMemberDef FrameAccum_members[] = {
    {"server", T_OBJECT_EX, offsetof(FrameAccum, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(FrameAccum, stream), 0, "Stream object."},
    {"mul", T_OBJECT_EX, offsetof(FrameAccum, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(FrameAccum, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef FrameAccum_methods[] = {
    {"getServer", (PyCFunction)FrameAccum_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)FrameAccum_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)FrameAccum_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)FrameAccum_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)FrameAccum_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)FrameAccum_stop, METH_NOARGS, "Stops computing."},
    {"setMul", (PyCFunction)FrameAccum_setMul, METH_O, "Sets FrameAccum mul factor."},
    {"setAdd", (PyCFunction)FrameAccum_setAdd, METH_O, "Sets FrameAccum add factor."},
    {"setSub", (PyCFunction)FrameAccum_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)FrameAccum_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods FrameAccum_as_number = {
    (binaryfunc)FrameAccum_add,                      /*nb_add*/
    (binaryfunc)FrameAccum_sub,                 /*nb_subtract*/
    (binaryfunc)FrameAccum_multiply,                 /*nb_multiply*/
    (binaryfunc)FrameAccum_div,                   /*nb_divide*/
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
    (binaryfunc)FrameAccum_inplace_add,              /*inplace_add*/
    (binaryfunc)FrameAccum_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)FrameAccum_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)FrameAccum_inplace_div,           /*inplace_divide*/
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

PyTypeObject FrameAccumType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyo.FrameAccum_base",         /*tp_name*/
    sizeof(FrameAccum),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)FrameAccum_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &FrameAccum_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES,  /*tp_flags*/
    "FrameAccum objects. Reads one band from a FrameAccumMain object.",           /* tp_doc */
    (traverseproc)FrameAccum_traverse,   /* tp_traverse */
    (inquiry)FrameAccum_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FrameAccum_methods,             /* tp_methods */
    FrameAccum_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)FrameAccum_init,      /* tp_init */
    0,                         /* tp_alloc */
    FrameAccum_new,                 /* tp_new */
};
