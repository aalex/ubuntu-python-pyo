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
#include "tablemodule.h"
#include "interpolation.h"

typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *env;
    PyObject *pitch;
    Stream *pitch_stream;
    PyObject *pos;
    Stream *pos_stream;
    PyObject *dur;
    Stream *dur_stream;
    int ngrains;
    MYFLT basedur;
    MYFLT pointerPos;
    MYFLT *startPos;
    MYFLT *gsize;
    MYFLT *gphase;
    int modebuffer[5];
} Granulator;

static void
Granulator_transform_iii(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT pit = PyFloat_AS_DOUBLE(self->pitch);
    MYFLT pos = PyFloat_AS_DOUBLE(self->pos);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur);
    
    inc = pit * (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_aii(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *pit = Stream_getData((Stream *)self->pitch_stream);
    MYFLT pos = PyFloat_AS_DOUBLE(self->pos);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_iai(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;

    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);

    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);

    MYFLT pit = PyFloat_AS_DOUBLE(self->pitch);
    MYFLT *pos = Stream_getData((Stream *)self->pos_stream);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur);

    inc = pit * (1.0 / self->basedur) / self->sr;
    
    MYFLT gsize = dur * self->sr;
    
    for (j=0; j<self->ngrains; j++) {
        self->gsize[j] = gsize;
    }

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;

            if (amp < 0.0001)
                self->startPos[j] = pos[i];

            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;

            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_aai(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *pit = Stream_getData((Stream *)self->pitch_stream);
    MYFLT *pos = Stream_getData((Stream *)self->pos_stream);
    MYFLT dur = PyFloat_AS_DOUBLE(self->dur);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos[i];
                self->gsize[j] = dur * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }
} 

static void
Granulator_transform_iia(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT pit = PyFloat_AS_DOUBLE(self->pitch);
    MYFLT pos = PyFloat_AS_DOUBLE(self->pos);
    MYFLT *dur = Stream_getData((Stream *)self->dur_stream);
    
    inc = pit * (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }     
}

static void
Granulator_transform_aia(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *pit = Stream_getData((Stream *)self->pitch_stream);
    MYFLT pos = PyFloat_AS_DOUBLE(self->pos);
    MYFLT *dur = Stream_getData((Stream *)self->dur_stream);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos;
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    } 
}

static void
Granulator_transform_iaa(Granulator *self) {
    MYFLT val, x, x1, inc, index, fpart, amp, ppos;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT pit = PyFloat_AS_DOUBLE(self->pitch);
    MYFLT *pos = Stream_getData((Stream *)self->pos_stream);
    MYFLT *dur = Stream_getData((Stream *)self->dur_stream);
    
    inc = pit * (1.0 / self->basedur) / self->sr;

    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos[i];
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    }    
}

static void
Granulator_transform_aaa(Granulator *self) { 
    MYFLT val, x, x1, inc, index, fpart, amp, ppos, frtosamps;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    
    MYFLT *envlist = TableStream_getData(self->env);
    int envsize = TableStream_getSize(self->env);
    
    MYFLT *pit = Stream_getData((Stream *)self->pitch_stream);
    MYFLT *pos = Stream_getData((Stream *)self->pos_stream);
    MYFLT *dur = Stream_getData((Stream *)self->dur_stream);
    
    frtosamps = (1.0 / self->basedur) / self->sr;
    
    for (i=0; i<self->bufsize; i++) {
        self->data[i] = 0.0;
        inc = pit[i] * frtosamps;
        self->pointerPos += inc;
        
        for (j=0; j<self->ngrains; j++) {
            ppos = self->pointerPos + self->gphase[j];
            if (ppos >= 1.0) {
                ppos -= 1.0;
            }
            // compute envelope
            index = ppos * envsize;
            ipart = (int)index;
            fpart = index - ipart;
            x = envlist[ipart];
            x1 = envlist[ipart+1];
            amp = x + (x1 - x) * fpart;
            
            if (amp < 0.0001) {
                self->startPos[j] = pos[i];
                self->gsize[j] = dur[i] * self->sr;
            }
            
            // compute sampling
            index = ppos * self->gsize[j] + self->startPos[j];
            if (index >= 0 && index < size) {
                ipart = (int)index;
                fpart = index - ipart;
                x = tablelist[ipart];
                x1 = tablelist[ipart+1];
                val = x + (x1 - x) * fpart;
            }
            else
                val = 0.0;
            
            self->data[i] += (val * amp);
        }
        
        if (self->pointerPos < 0)
            self->pointerPos += 1.0;
        else if (self->pointerPos >= 1)
            self->pointerPos -= 1.0;
    } 
}

static void Granulator_postprocessing_ii(Granulator *self) { POST_PROCESSING_II };
static void Granulator_postprocessing_ai(Granulator *self) { POST_PROCESSING_AI };
static void Granulator_postprocessing_ia(Granulator *self) { POST_PROCESSING_IA };
static void Granulator_postprocessing_aa(Granulator *self) { POST_PROCESSING_AA };
static void Granulator_postprocessing_ireva(Granulator *self) { POST_PROCESSING_IREVA };
static void Granulator_postprocessing_areva(Granulator *self) { POST_PROCESSING_AREVA };
static void Granulator_postprocessing_revai(Granulator *self) { POST_PROCESSING_REVAI };
static void Granulator_postprocessing_revaa(Granulator *self) { POST_PROCESSING_REVAA };
static void Granulator_postprocessing_revareva(Granulator *self) { POST_PROCESSING_REVAREVA };

static void
Granulator_setProcMode(Granulator *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2] + self->modebuffer[3] * 10 + self->modebuffer[4] * 100;
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;

	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Granulator_transform_iii;
            break;
        case 1:    
            self->proc_func_ptr = Granulator_transform_aii;
            break;
        case 10:        
            self->proc_func_ptr = Granulator_transform_iai;
            break;
        case 11:    
            self->proc_func_ptr = Granulator_transform_aai;
            break;
        case 100:        
            self->proc_func_ptr = Granulator_transform_iia;
            break;
        case 101:    
            self->proc_func_ptr = Granulator_transform_aia;
            break;
        case 110:        
            self->proc_func_ptr = Granulator_transform_iaa;
            break;
        case 111:    
            self->proc_func_ptr = Granulator_transform_aaa;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Granulator_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Granulator_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Granulator_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Granulator_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Granulator_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Granulator_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Granulator_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Granulator_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Granulator_postprocessing_revareva;
            break;
    }   
}

static void
Granulator_compute_next_data_frame(Granulator *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Granulator_traverse(Granulator *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->pitch);    
    Py_VISIT(self->pitch_stream);    
    Py_VISIT(self->pos);    
    Py_VISIT(self->pos_stream);    
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    return 0;
}

static int 
Granulator_clear(Granulator *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->pitch);    
    Py_CLEAR(self->pitch_stream);    
    Py_CLEAR(self->pos);    
    Py_CLEAR(self->pos_stream);    
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    return 0;
}

static void
Granulator_dealloc(Granulator* self)
{
    free(self->data);   
    free(self->startPos);   
    free(self->gphase);
    free(self->gsize);
    Granulator_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Granulator_deleteStream(Granulator *self) { DELETE_STREAM };

static PyObject *
Granulator_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Granulator *self;
    self = (Granulator *)type->tp_alloc(type, 0);

    self->pitch = PyFloat_FromDouble(1);
    self->pos = PyFloat_FromDouble(0.0);
    self->dur = PyFloat_FromDouble(0.1);
    self->ngrains = 8;
    self->basedur = 0.1;
    self->pointerPos = 1.0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;

    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Granulator_compute_next_data_frame);
    self->mode_func_ptr = Granulator_setProcMode;
    
    return (PyObject *)self;
}

static int
Granulator_init(Granulator *self, PyObject *args, PyObject *kwds)
{
    int i;
    MYFLT phase;
    PyObject *tabletmp, *envtmp, *pitchtmp=NULL, *postmp=NULL, *durtmp=NULL, *multmp=NULL, *addtmp=NULL;

    static char *kwlist[] = {"table", "env", "pitch", "pos", "dur", "grains", "basedur", "mul", "add", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_OO_OOOIFOO, kwlist, &tabletmp, &envtmp, &pitchtmp, &postmp, &durtmp, &self->ngrains, &self->basedur, &multmp, &addtmp))
        return -1; 

    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    Py_XDECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)envtmp, "getTableStream", "");
    
    if (pitchtmp) {
        PyObject_CallMethod((PyObject *)self, "setPitch", "O", pitchtmp);
    }

    if (postmp) {
        PyObject_CallMethod((PyObject *)self, "setPos", "O", postmp);
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

    self->startPos = (MYFLT *)realloc(self->startPos, self->ngrains * sizeof(MYFLT));
    self->gsize = (MYFLT *)realloc(self->gsize, self->ngrains * sizeof(MYFLT));
    self->gphase = (MYFLT *)realloc(self->gphase, self->ngrains * sizeof(MYFLT));

    srand((unsigned)(time(0)));
    for (i=0; i<self->ngrains; i++) {
        phase = ((MYFLT)i/self->ngrains) * (1.0 + ((rand()/((MYFLT)(RAND_MAX)+1)*2.0-1.0) * 0.015));
        if (phase < 0.0)
            phase = 0.0;
        self->gphase[i] = phase;
        self->startPos[i] = self->gsize[i] = 0.0;
    }
    
    (*self->mode_func_ptr)(self);

    Py_INCREF(self);
    return 0;
}

static PyObject * Granulator_getServer(Granulator* self) { GET_SERVER };
static PyObject * Granulator_getStream(Granulator* self) { GET_STREAM };
static PyObject * Granulator_setMul(Granulator *self, PyObject *arg) { SET_MUL };	
static PyObject * Granulator_setAdd(Granulator *self, PyObject *arg) { SET_ADD };	
static PyObject * Granulator_setSub(Granulator *self, PyObject *arg) { SET_SUB };	
static PyObject * Granulator_setDiv(Granulator *self, PyObject *arg) { SET_DIV };	

static PyObject * Granulator_play(Granulator *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Granulator_out(Granulator *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Granulator_stop(Granulator *self) { STOP };

static PyObject * Granulator_multiply(Granulator *self, PyObject *arg) { MULTIPLY };
static PyObject * Granulator_inplace_multiply(Granulator *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Granulator_add(Granulator *self, PyObject *arg) { ADD };
static PyObject * Granulator_inplace_add(Granulator *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Granulator_sub(Granulator *self, PyObject *arg) { SUB };
static PyObject * Granulator_inplace_sub(Granulator *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Granulator_div(Granulator *self, PyObject *arg) { DIV };
static PyObject * Granulator_inplace_div(Granulator *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Granulator_setPitch(Granulator *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pitch);
	if (isNumber == 1) {
		self->pitch = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->pitch = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pitch, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pitch_stream);
        self->pitch_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setPos(Granulator *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pos);
	if (isNumber == 1) {
		self->pos = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->pos = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pos, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pos_stream);
        self->pos_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setDur(Granulator *self, PyObject *arg)
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
        self->modebuffer[4] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Granulator_getTable(Granulator* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Granulator_setTable(Granulator *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_getEnv(Granulator* self)
{
    Py_INCREF(self->env);
    return self->env;
};

static PyObject *
Granulator_setEnv(Granulator *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->env);
    self->env = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setBaseDur(Granulator *self, PyObject *arg)
{	
	if (arg != NULL)
        self->basedur = PyFloat_AS_DOUBLE(PyNumber_Float(arg));
        
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Granulator_setGrains(Granulator *self, PyObject *arg)
{	
    int i;
    MYFLT phase;
	if (PyLong_Check(arg) || PyInt_Check(arg)) {
        self->ngrains = PyLong_AsLong(arg);
        self->startPos = (MYFLT *)realloc(self->startPos, self->ngrains * sizeof(MYFLT));
        self->gsize = (MYFLT *)realloc(self->gsize, self->ngrains * sizeof(MYFLT));
        self->gphase = (MYFLT *)realloc(self->gphase, self->ngrains * sizeof(MYFLT));
        
        srand((unsigned)(time(0)));
        for (i=0; i<self->ngrains; i++) {
            phase = ((MYFLT)i/self->ngrains) * (1.0 + ((rand()/((MYFLT)(RAND_MAX)+1)*2.0-1.0) * 0.015));
            if (phase < 0.0)
                phase = 0.0;
            self->gphase[i] = phase;
        }        
    }    
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyMemberDef Granulator_members[] = {
    {"server", T_OBJECT_EX, offsetof(Granulator, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Granulator, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(Granulator, table), 0, "Sound table."},
    {"env", T_OBJECT_EX, offsetof(Granulator, env), 0, "Envelope table."},
    {"pitch", T_OBJECT_EX, offsetof(Granulator, pitch), 0, "Speed of the reading pointer."},
    {"pos", T_OBJECT_EX, offsetof(Granulator, pos), 0, "Position in the sound table."},
    {"dur", T_OBJECT_EX, offsetof(Granulator, dur), 0, "Duration of each grains."},
    {"mul", T_OBJECT_EX, offsetof(Granulator, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Granulator, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Granulator_methods[] = {
    {"getTable", (PyCFunction)Granulator_getTable, METH_NOARGS, "Returns sound table object."},
    {"setTable", (PyCFunction)Granulator_setTable, METH_O, "Sets sound table."},
    {"getEnv", (PyCFunction)Granulator_getEnv, METH_NOARGS, "Returns envelope table object."},
    {"setEnv", (PyCFunction)Granulator_setEnv, METH_O, "Sets envelope table."},
    {"getServer", (PyCFunction)Granulator_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Granulator_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Granulator_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Granulator_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Granulator_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Granulator_stop, METH_NOARGS, "Stops computing."},
	{"setPitch", (PyCFunction)Granulator_setPitch, METH_O, "Sets global pitch factor."},
    {"setPos", (PyCFunction)Granulator_setPos, METH_O, "Sets position in the sound table."},
    {"setDur", (PyCFunction)Granulator_setDur, METH_O, "Sets the grain duration."},
    {"setBaseDur", (PyCFunction)Granulator_setBaseDur, METH_O, "Sets the grain base duration."},
    {"setGrains", (PyCFunction)Granulator_setGrains, METH_O, "Sets the number of grains."},
	{"setMul", (PyCFunction)Granulator_setMul, METH_O, "Sets granulator mul factor."},
	{"setAdd", (PyCFunction)Granulator_setAdd, METH_O, "Sets granulator add factor."},
    {"setSub", (PyCFunction)Granulator_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Granulator_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Granulator_as_number = {
    (binaryfunc)Granulator_add,                      /*nb_add*/
    (binaryfunc)Granulator_sub,                 /*nb_subtract*/
    (binaryfunc)Granulator_multiply,                 /*nb_multiply*/
    (binaryfunc)Granulator_div,                   /*nb_divide*/
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
    (binaryfunc)Granulator_inplace_add,              /*inplace_add*/
    (binaryfunc)Granulator_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Granulator_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Granulator_inplace_div,           /*inplace_divide*/
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

PyTypeObject GranulatorType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_pitch*/
    "_pyo.Granulator_base",         /*tp_name*/
    sizeof(Granulator),         /*tp_basicpitch*/
    0,                         /*tp_itempitch*/
    (destructor)Granulator_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Granulator_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Granulator objects. Accumulation of multiples grains of sound.",           /* tp_doc */
    (traverseproc)Granulator_traverse,   /* tp_traverse */
    (inquiry)Granulator_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Granulator_methods,             /* tp_methods */
    Granulator_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Granulator_init,      /* tp_init */
    0,                         /* tp_alloc */
    Granulator_new,                 /* tp_new */
};

MYFLT LOOPER_LINEAR_FADE[513] = {0.0, 0.001953125, 0.00390625, 0.005859375, 0.0078125, 0.009765625, 0.01171875, 0.013671875, 0.015625, 0.017578125, 0.01953125, 0.021484375, 0.0234375, 0.025390625, 0.02734375, 0.029296875, 0.03125, 0.033203125, 0.03515625, 0.037109375, 0.0390625, 0.041015625, 0.04296875, 0.044921875, 0.046875, 0.048828125, 0.05078125, 0.052734375, 0.0546875, 0.056640625, 0.05859375, 0.060546875, 0.0625, 0.064453125, 0.06640625, 0.068359375, 0.0703125, 0.072265625, 0.07421875, 0.076171875, 0.078125, 0.080078125, 0.08203125, 0.083984375, 0.0859375, 0.087890625, 0.08984375, 0.091796875, 0.09375, 0.095703125, 0.09765625, 0.099609375, 0.1015625, 0.103515625, 0.10546875, 0.107421875, 0.109375, 0.111328125, 0.11328125, 0.115234375, 0.1171875, 0.119140625, 0.12109375, 0.123046875, 0.125, 0.126953125, 0.12890625, 0.130859375, 0.1328125, 0.134765625, 0.13671875, 0.138671875, 0.140625, 0.142578125, 0.14453125, 0.146484375, 0.1484375, 0.150390625, 0.15234375, 0.154296875, 0.15625, 0.158203125, 0.16015625, 0.162109375, 0.1640625, 0.166015625, 0.16796875, 0.169921875, 0.171875, 0.173828125, 0.17578125, 0.177734375, 0.1796875, 0.181640625, 0.18359375, 0.185546875, 0.1875, 0.189453125, 0.19140625, 0.193359375, 0.1953125, 0.197265625, 0.19921875, 0.201171875, 0.203125, 0.205078125, 0.20703125, 0.208984375, 0.2109375, 0.212890625, 0.21484375, 0.216796875, 0.21875, 0.220703125, 0.22265625, 0.224609375, 0.2265625, 0.228515625, 0.23046875, 0.232421875, 0.234375, 0.236328125, 0.23828125, 0.240234375, 0.2421875, 0.244140625, 0.24609375, 0.248046875, 0.25, 0.251953125, 0.25390625, 0.255859375, 0.2578125, 0.259765625, 0.26171875, 0.263671875, 0.265625, 0.267578125, 0.26953125, 0.271484375, 0.2734375, 0.275390625, 0.27734375, 0.279296875, 0.28125, 0.283203125, 0.28515625, 0.287109375, 0.2890625, 0.291015625, 0.29296875, 0.294921875, 0.296875, 0.298828125, 0.30078125, 0.302734375, 0.3046875, 0.306640625, 0.30859375, 0.310546875, 0.3125, 0.314453125, 0.31640625, 0.318359375, 0.3203125, 0.322265625, 0.32421875, 0.326171875, 0.328125, 0.330078125, 0.33203125, 0.333984375, 0.3359375, 0.337890625, 0.33984375, 0.341796875, 0.34375, 0.345703125, 0.34765625, 0.349609375, 0.3515625, 0.353515625, 0.35546875, 0.357421875, 0.359375, 0.361328125, 0.36328125, 0.365234375, 0.3671875, 0.369140625, 0.37109375, 0.373046875, 0.375, 0.376953125, 0.37890625, 0.380859375, 0.3828125, 0.384765625, 0.38671875, 0.388671875, 0.390625, 0.392578125, 0.39453125, 0.396484375, 0.3984375, 0.400390625, 0.40234375, 0.404296875, 0.40625, 0.408203125, 0.41015625, 0.412109375, 0.4140625, 0.416015625, 0.41796875, 0.419921875, 0.421875, 0.423828125, 0.42578125, 0.427734375, 0.4296875, 0.431640625, 0.43359375, 0.435546875, 0.4375, 0.439453125, 0.44140625, 0.443359375, 0.4453125, 0.447265625, 0.44921875, 0.451171875, 0.453125, 0.455078125, 0.45703125, 0.458984375, 0.4609375, 0.462890625, 0.46484375, 0.466796875, 0.46875, 0.470703125, 0.47265625, 0.474609375, 0.4765625, 0.478515625, 0.48046875, 0.482421875, 0.484375, 0.486328125, 0.48828125, 0.490234375, 0.4921875, 0.494140625, 0.49609375, 0.498046875, 0.5, 0.501953125, 0.50390625, 0.505859375, 0.5078125, 0.509765625, 0.51171875, 0.513671875, 0.515625, 0.517578125, 0.51953125, 0.521484375, 0.5234375, 0.525390625, 0.52734375, 0.529296875, 0.53125, 0.533203125, 0.53515625, 0.537109375, 0.5390625, 0.541015625, 0.54296875, 0.544921875, 0.546875, 0.548828125, 0.55078125, 0.552734375, 0.5546875, 0.556640625, 0.55859375, 0.560546875, 0.5625, 0.564453125, 0.56640625, 0.568359375, 0.5703125, 0.572265625, 0.57421875, 0.576171875, 0.578125, 0.580078125, 0.58203125, 0.583984375, 0.5859375, 0.587890625, 0.58984375, 0.591796875, 0.59375, 0.595703125, 0.59765625, 0.599609375, 0.6015625, 0.603515625, 0.60546875, 0.607421875, 0.609375, 0.611328125, 0.61328125, 0.615234375, 0.6171875, 0.619140625, 0.62109375, 0.623046875, 0.625, 0.626953125, 0.62890625, 0.630859375, 0.6328125, 0.634765625, 0.63671875, 0.638671875, 0.640625, 0.642578125, 0.64453125, 0.646484375, 0.6484375, 0.650390625, 0.65234375, 0.654296875, 0.65625, 0.658203125, 0.66015625, 0.662109375, 0.6640625, 0.666015625, 0.66796875, 0.669921875, 0.671875, 0.673828125, 0.67578125, 0.677734375, 0.6796875, 0.681640625, 0.68359375, 0.685546875, 0.6875, 0.689453125, 0.69140625, 0.693359375, 0.6953125, 0.697265625, 0.69921875, 0.701171875, 0.703125, 0.705078125, 0.70703125, 0.708984375, 0.7109375, 0.712890625, 0.71484375, 0.716796875, 0.71875, 0.720703125, 0.72265625, 0.724609375, 0.7265625, 0.728515625, 0.73046875, 0.732421875, 0.734375, 0.736328125, 0.73828125, 0.740234375, 0.7421875, 0.744140625, 0.74609375, 0.748046875, 0.75, 0.751953125, 0.75390625, 0.755859375, 0.7578125, 0.759765625, 0.76171875, 0.763671875, 0.765625, 0.767578125, 0.76953125, 0.771484375, 0.7734375, 0.775390625, 0.77734375, 0.779296875, 0.78125, 0.783203125, 0.78515625, 0.787109375, 0.7890625, 0.791015625, 0.79296875, 0.794921875, 0.796875, 0.798828125, 0.80078125, 0.802734375, 0.8046875, 0.806640625, 0.80859375, 0.810546875, 0.8125, 0.814453125, 0.81640625, 0.818359375, 0.8203125, 0.822265625, 0.82421875, 0.826171875, 0.828125, 0.830078125, 0.83203125, 0.833984375, 0.8359375, 0.837890625, 0.83984375, 0.841796875, 0.84375, 0.845703125, 0.84765625, 0.849609375, 0.8515625, 0.853515625, 0.85546875, 0.857421875, 0.859375, 0.861328125, 0.86328125, 0.865234375, 0.8671875, 0.869140625, 0.87109375, 0.873046875, 0.875, 0.876953125, 0.87890625, 0.880859375, 0.8828125, 0.884765625, 0.88671875, 0.888671875, 0.890625, 0.892578125, 0.89453125, 0.896484375, 0.8984375, 0.900390625, 0.90234375, 0.904296875, 0.90625, 0.908203125, 0.91015625, 0.912109375, 0.9140625, 0.916015625, 0.91796875, 0.919921875, 0.921875, 0.923828125, 0.92578125, 0.927734375, 0.9296875, 0.931640625, 0.93359375, 0.935546875, 0.9375, 0.939453125, 0.94140625, 0.943359375, 0.9453125, 0.947265625, 0.94921875, 0.951171875, 0.953125, 0.955078125, 0.95703125, 0.958984375, 0.9609375, 0.962890625, 0.96484375, 0.966796875, 0.96875, 0.970703125, 0.97265625, 0.974609375, 0.9765625, 0.978515625, 0.98046875, 0.982421875, 0.984375, 0.986328125, 0.98828125, 0.990234375, 0.9921875, 0.994140625, 0.99609375, 0.998046875, 1.0};
MYFLT LOOPER_POWER_FADE[513] = {0.0, 0.0030679567629659761, 0.0061358846491544753, 0.0092037547820598194, 0.012271538285719925, 0.0153392062849881, 0.01840672990580482, 0.021474080275469508, 0.024541228522912288, 0.02760814577896574, 0.030674803176636626, 0.03374117185137758, 0.036807222941358832, 0.039872927587739811, 0.04293825693494082, 0.046003182130914623, 0.049067674327418015, 0.052131704680283324, 0.055195244349689941, 0.058258264500435752, 0.061320736302208578, 0.064382630929857465, 0.067443919563664051, 0.070504573389613856, 0.073564563599667426, 0.076623861392031492, 0.079682437971430126, 0.082740264549375692, 0.085797312344439894, 0.0888535525825246, 0.091908956497132724, 0.094963495329638992, 0.098017140329560604, 0.10106986275482782, 0.10412163387205459, 0.10717242495680884, 0.11022220729388306, 0.11327095217756435, 0.11631863091190475, 0.11936521481099135, 0.1224106751992162, 0.12545498341154623, 0.12849811079379317, 0.13154002870288312, 0.13458070850712617, 0.13762012158648604, 0.14065823933284921, 0.14369503315029447, 0.14673047445536175, 0.14976453467732151, 0.15279718525844344, 0.15582839765426523, 0.15885814333386145, 0.16188639378011183, 0.16491312048996992, 0.16793829497473117, 0.17096188876030122, 0.17398387338746382, 0.17700422041214875, 0.18002290140569951, 0.18303988795514095, 0.18605515166344663, 0.18906866414980619, 0.19208039704989244, 0.19509032201612825, 0.19809841071795356, 0.2011046348420919, 0.20410896609281687, 0.20711137619221856, 0.21011183688046961, 0.21311031991609136, 0.21610679707621952, 0.2191012401568698, 0.22209362097320351, 0.22508391135979283, 0.22807208317088573, 0.23105810828067111, 0.23404195858354343, 0.2370236059943672, 0.2400030224487415, 0.24298017990326387, 0.24595505033579459, 0.24892760574572015, 0.25189781815421697, 0.25486565960451457, 0.25783110216215899, 0.26079411791527551, 0.26375467897483135, 0.26671275747489837, 0.26966832557291509, 0.27262135544994898, 0.27557181931095814, 0.27851968938505306, 0.28146493792575794, 0.28440753721127188, 0.28734745954472951, 0.29028467725446233, 0.29321916269425863, 0.29615088824362379, 0.29907982630804048, 0.30200594931922808, 0.30492922973540237, 0.30784964004153487, 0.31076715274961147, 0.31368174039889152, 0.31659337555616585, 0.31950203081601569, 0.32240767880106985, 0.32531029216226293, 0.3282098435790925, 0.33110630575987643, 0.33399965144200938, 0.33688985339222005, 0.33977688440682685, 0.34266071731199438, 0.34554132496398909, 0.34841868024943456, 0.35129275608556709, 0.35416352542049034, 0.35703096123342998, 0.35989503653498811, 0.36275572436739723, 0.36561299780477385, 0.36846682995337232, 0.37131719395183754, 0.37416406297145793, 0.37700741021641826, 0.37984720892405116, 0.38268343236508978, 0.38551605384391885, 0.38834504669882625, 0.39117038430225387, 0.3939920400610481, 0.39680998741671031, 0.39962419984564679, 0.40243465085941843, 0.40524131400498986, 0.40804416286497869, 0.41084317105790391, 0.41363831223843456, 0.41642956009763715, 0.41921688836322391, 0.42200027079979968, 0.42477968120910881, 0.42755509343028208, 0.43032648134008261, 0.43309381885315196, 0.43585707992225547, 0.43861623853852766, 0.44137126873171667, 0.4441221445704292, 0.44686884016237416, 0.44961132965460654, 0.45234958723377089, 0.45508358712634384, 0.45781330359887723, 0.46053871095824001, 0.46325978355186015, 0.46597649576796618, 0.46868882203582796, 0.47139673682599764, 0.47410021465054997, 0.47679923006332209, 0.47949375766015301, 0.48218377207912272, 0.48486924800079106, 0.487550160148436, 0.49022648328829116, 0.49289819222978404, 0.49556526182577254, 0.49822766697278187, 0.50088538261124071, 0.50353838372571758, 0.50618664534515523, 0.50883014254310699, 0.5114688504379703, 0.51410274419322166, 0.51673179901764987, 0.51935599016558964, 0.52197529293715439, 0.52458968267846895, 0.52719913478190139, 0.52980362468629461, 0.5324031278771979, 0.53499761988709715, 0.53758707629564539, 0.54017147272989285, 0.54275078486451589, 0.54532498842204646, 0.54789405917310019, 0.55045797293660481, 0.55301670558002747, 0.55557023301960218, 0.5581185312205561, 0.56066157619733603, 0.56319934401383409, 0.56573181078361312, 0.56825895267013149, 0.57078074588696726, 0.5732971666980422, 0.57580819141784534, 0.57831379641165559, 0.58081395809576453, 0.58330865293769829, 0.58579785745643886, 0.58828154822264522, 0.59075970185887416, 0.5932322950397998, 0.59569930449243336, 0.59816070699634238, 0.60061647938386897, 0.60306659854034816, 0.60551104140432555, 0.60794978496777363, 0.61038280627630948, 0.61281008242940971, 0.61523159058062682, 0.61764730793780387, 0.6200572117632891, 0.62246127937414997, 0.62485948814238634, 0.62725181549514408, 0.62963823891492698, 0.63201873593980906, 0.63439328416364549, 0.6367618612362842, 0.63912444486377573, 0.64148101280858316, 0.64383154288979139, 0.64617601298331628, 0.64851440102211244, 0.65084668499638099, 0.65317284295377676, 0.65549285299961535, 0.65780669329707864, 0.66011434206742048, 0.66241577759017178, 0.66471097820334479, 0.66699992230363747, 0.66928258834663601, 0.67155895484701833, 0.67382900037875604, 0.67609270357531592, 0.67835004312986147, 0.68060099779545302, 0.68284554638524808, 0.68508366777270036, 0.68731534089175905, 0.68954054473706683, 0.69175925836415775, 0.69397146088965389, 0.69617713149146299, 0.69837624940897292, 0.70056879394324834, 0.7027547444572253, 0.70493408037590488, 0.70710678118654746, 0.70927282643886558, 0.71143219574521632, 0.71358486878079352, 0.71573082528381859, 0.7178700450557316, 0.72000250796138165, 0.72212819392921523, 0.72424708295146689, 0.7263591550843459, 0.7284643904482252, 0.73056276922782759, 0.7326542716724127, 0.73473887809596339, 0.73681656887736979, 0.73888732446061511, 0.74095112535495899, 0.74300795213512161, 0.74505778544146595, 0.74710060598018013, 0.74913639452345926, 0.75116513190968637, 0.75318679904361241, 0.75520137689653644, 0.75720884650648446, 0.75920918897838796, 0.76120238548426178, 0.76318841726338127, 0.76516726562245885, 0.76713891193582029, 0.76910333764557959, 0.77106052426181371, 0.77301045336273688, 0.77495310659487382, 0.77688846567323244, 0.77881651238147587, 0.78073722857209438, 0.78265059616657562, 0.78455659715557524, 0.78645521359908577, 0.78834642762660623, 0.79023022143731003, 0.79210657730021228, 0.79397547755433706, 0.79583690460888346, 0.79769084094339104, 0.79953726910790501, 0.80137617172314013, 0.80320753148064483, 0.80503133114296355, 0.80684755354379922, 0.80865618158817498, 0.81045719825259477, 0.81225058658520388, 0.8140363297059483, 0.81581441080673378, 0.81758481315158371, 0.8193475200767969, 0.82110251499110465, 0.82284978137582632, 0.82458930278502529, 0.82632106284566342, 0.82804504525775569, 0.82976123379452305, 0.83146961230254512, 0.83317016470191319, 0.83486287498638001, 0.83654772722351189, 0.83822470555483797, 0.83989379419599941, 0.84155497743689833, 0.84320823964184544, 0.84485356524970701, 0.84649093877405202, 0.84812034480329712, 0.84974176800085244, 0.8513551931052652, 0.85296060493036363, 0.85455798836540053, 0.85614732837519436, 0.85772861000027212, 0.85930181835700825, 0.8608669386377672, 0.8624239561110405, 0.8639728561215867, 0.86551362409056898, 0.86704624551569265, 0.8685707059713409, 0.87008699110871135, 0.87159508665595098, 0.87309497841828998, 0.87458665227817611, 0.87607009419540649, 0.87754529020726124, 0.87901222642863341, 0.88047088905216075, 0.88192126434835494, 0.88336333866573158, 0.88479709843093779, 0.88622253014888064, 0.88763962040285393, 0.88904835585466446, 0.89044872324475788, 0.89184070939234272, 0.89322430119551532, 0.89459948563138258, 0.89596624975618511, 0.89732458070541832, 0.89867446569395382, 0.90001589201616028, 0.90134884704602203, 0.90267331823725883, 0.90398929312344334, 0.90529675931811882, 0.90659570451491533, 0.90788611648766615, 0.90916798309052238, 0.91044129225806714, 0.91170603200542988, 0.9129621904283981, 0.91420975570353069, 0.91544871608826783, 0.9166790599210427, 0.91790077562139039, 0.91911385169005777, 0.92031827670911048, 0.9215140393420419, 0.92270112833387852, 0.92387953251128674, 0.92504924078267758, 0.92621024213831138, 0.92736252565040111, 0.92850608047321548, 0.92964089584318121, 0.93076696107898371, 0.93188426558166815, 0.93299279883473885, 0.93409255040425887, 0.9351835099389475, 0.93626566717027826, 0.93733901191257496, 0.93840353406310806, 0.93945922360218992, 0.9405060705932683, 0.94154406518302081, 0.94257319760144687, 0.94359345816196039, 0.94460483726148026, 0.94560732538052128, 0.94660091308328353, 0.94758559101774109, 0.94856134991573027, 0.94952818059303667, 0.9504860739494817, 0.95143502096900834, 0.95237501271976588, 0.95330604035419375, 0.95422809510910567, 0.95514116830577067, 0.95604525134999641, 0.95694033573220894, 0.95782641302753291, 0.9587034748958716, 0.95957151308198452, 0.96043051941556579, 0.96128048581132064, 0.96212140426904158, 0.96295326687368388, 0.96377606579543984, 0.96458979328981265, 0.9653944416976894, 0.96619000344541262, 0.96697647104485207, 0.96775383709347551, 0.96852209427441727, 0.96928123535654853, 0.97003125319454397, 0.97077214072895035, 0.97150389098625178, 0.97222649707893627, 0.97293995220556007, 0.97364424965081187, 0.97433938278557586, 0.97502534506699412, 0.97570213003852857, 0.97636973133002114, 0.97702814265775439, 0.97767735782450993, 0.97831737071962765, 0.9789481753190622, 0.97956976568544052, 0.98018213596811732, 0.98078528040323043, 0.98137919331375456, 0.98196386910955524, 0.98253930228744124, 0.98310548743121629, 0.98366241921173025, 0.98421009238692903, 0.98474850180190421, 0.98527764238894122, 0.98579750916756737, 0.98630809724459867, 0.98680940181418542, 0.98730141815785843, 0.98778414164457218, 0.98825756773074946, 0.98872169196032378, 0.98917650996478101, 0.98962201746320078, 0.99005821026229712, 0.99048508425645698, 0.99090263542778001, 0.99131085984611544, 0.99170975366909953, 0.9920993131421918, 0.99247953459870997, 0.9928504144598651, 0.9932119492347945, 0.9935641355205953, 0.99390697000235606, 0.9942404494531879, 0.99456457073425542, 0.99487933079480562, 0.99518472667219682, 0.99548075549192694, 0.99576741446765982, 0.99604470090125197, 0.996312612182778, 0.99657114579055484, 0.99682029929116567, 0.99706007033948296, 0.99729045667869021, 0.99751145614030345, 0.99772306664419164, 0.997925286198596, 0.99811811290014918, 0.99830154493389289, 0.99847558057329477, 0.99864021818026527, 0.99879545620517241, 0.99894129318685687, 0.99907772775264536, 0.99920475861836389, 0.99932238458834954, 0.99943060455546173, 0.99952941750109314, 0.99961882249517864, 0.99969881869620425, 0.99976940535121528, 0.9998305817958234, 0.99988234745421256, 0.9999247018391445, 0.9999576445519639, 0.99998117528260111, 0.99999529380957619, 1.0};
MYFLT LOOPER_SIGMOID_FADE[513] = {0.0, 9.4123586994454556e-06, 3.7649080427748505e-05, 8.4709102088298405e-05, 0.00015059065189787502, 0.00023529124945342872, 0.00033880770582522812, 0.00046113612367731927, 0.0006022718974137975, 0.00076220971335261289, 0.00094094354992541041, 0.0011384666779041819, 0.0013547716606548965, 0.0015898503544171105, 0.0018436939086109994, 0.0021162927661700914, 0.0024076366639015356, 0.0027177146328722368, 0.0030465149988219697, 0.0033940253826026945, 0.0037602327006450165, 0.0041451231654502374, 0.0045486822861099951, 0.0049708948688514387, 0.0054117450176094928, 0.0058712161346252678, 0.0063492909210707826, 0.0068459513777006653, 0.0073611788055293337, 0.0078949538065354319, 0.008447256284391802, 0.0090180654452223785, 0.0096073597983847847, 0.010215117157279741, 0.010841314640186173, 0.011485928671122803, 0.012148934980735715, 0.012830308607212071, 0.013530023897219912, 0.014248054506874053, 0.014984373402728013, 0.015738952862791311, 0.016511764477573909, 0.017302779151155301, 0.018111967102280024, 0.01893929786547921, 0.019784740292217051, 0.0206482625520642, 0.021529832133895532, 0.022429415847114664, 0.023346979822903013, 0.024282489515495831, 0.025235909703481607, 0.026207204491129399, 0.02719633730973936, 0.028203270919019807, 0.029227967408489597, 0.030270388198904985, 0.03133049404371252, 0.032408245030526195, 0.033503600582630522, 0.034616519460508088, 0.035746959763392205, 0.036894878930844255, 0.038060233744356631, 0.039242980328979049, 0.040443074154971115, 0.041660470039478648, 0.042895122148234655, 0.044146983997285061, 0.045416008454738754, 0.046702147742542333, 0.048005353438278275, 0.049325576476988986, 0.050662767153022981, 0.052016875121907391, 0.053387849402242338, 0.054775638377621005, 0.056180189798573033, 0.05760145078453105, 0.059039367825822475, 0.060493886785683182, 0.061964952902296699, 0.06345251079085501, 0.06495650444564427, 0.066476877242153565, 0.068013571939206596, 0.069566530681116401, 0.071135694999863941, 0.072721005817299733, 0.074322403447367402, 0.075939827598351384, 0.077573217375146386, 0.079222511281550778, 0.080887647222581016, 0.082568562506809939, 0.084265193848727271, 0.085977477371122046, 0.087705348607487355, 0.08944874250444762, 0.091207593424208144, 0.092981835147025738, 0.094771400873702616, 0.096576223228100277, 0.098396234259677529, 0.10023136544604749, 0.10208154769555822, 0.10394671134989369, 0.10582678618669683, 0.10772170142221238, 0.1096313857139527, 0.11155576716338378, 0.11349477331863145, 0.11544833117721015, 0.11741636718877047, 0.11939880725786906, 0.12139557674675777, 0.12340660047819374, 0.1254318027382702, 0.12747110727926697, 0.1295244373225205, 0.13159171556131499, 0.13367286416379359, 0.13576780477588735, 0.1378764585242665, 0.13999874601930906, 0.14213458735809065, 0.14428390212739184, 0.14644660940672616, 0.14862262777138718, 0.15081187529551349, 0.153014269555173, 0.15522972763146642, 0.15745816611364977, 0.15969950110227338, 0.16195364821234198, 0.16422052257649067, 0.16650003884818121, 0.16879211120491411, 0.17109665335146057, 0.17341357852311146, 0.17574279948894383, 0.17808422855510425, 0.18043777756811202, 0.18280335791817703, 0.18518088054253651, 0.18757025592880677, 0.18997139411835529, 0.19238420470968631, 0.19480859686184526, 0.19724447929783717, 0.19969176030806535, 0.20215034775378338, 0.20462014907056286, 0.20710107127178046, 0.20959302095211751, 0.21209590429107739, 0.21460962705651632, 0.21713409460819322, 0.21966921190133171, 0.22221488349019891, 0.22477101353169748, 0.2273375057889766, 0.22991426363505363, 0.23250119005645137, 0.23509818765685253, 0.23770515866076569, 0.24032200491720523, 0.24294862790338906, 0.24558492872844634, 0.24823080813714093, 0.25088616651360907, 0.25355090388510787, 0.25622491992578178, 0.25890811396043867, 0.26160038496833893, 0.26430163158700104, 0.26701175211601708, 0.26973064452088003, 0.27245820643682805, 0.27519433517269654, 0.27793892771478512, 0.2806918807307362, 0.28345309057342394, 0.28622245328485874, 0.28899986460010024, 0.2917852199511814, 0.29457841447104793, 0.29737934299750524, 0.30018790007717666, 0.30300397996947592, 0.30582747665058668, 0.30865828381745486, 0.31149629489179087, 0.31434140302408115, 0.31719350109761285, 0.320052481732506, 0.32291823728975477, 0.32579065987528255, 0.32866964134400301, 0.33155507330389, 0.33444684712006173, 0.33734485391886837, 0.34024898459199188, 0.3431591298005543, 0.34607517997923243, 0.34899702534038579, 0.35192455587818816, 0.35485766137276881, 0.35779623139436395, 0.36074015530747361, 0.36368932227502559, 0.36664362126255073, 0.36960294104236202, 0.37256717019774244, 0.37553619712713993, 0.37850991004836793, 0.38148819700281617, 0.38447094585966451, 0.38745804432010356, 0.39044937992156492, 0.39344484004195446, 0.39644431190389073, 0.39944768257895397, 0.40245483899193568, 0.40546566792509658, 0.40848005602242954, 0.41149788979392549, 0.41451905561984914, 0.41754343975501512, 0.42057092833306925, 0.42360140737077812, 0.42663476277231926, 0.42967088033357542, 0.43270964574643683, 0.43575094460310321, 0.43879466240039156, 0.44184068454404762, 0.44488889635305834, 0.44793918306397246, 0.45099142983521978, 0.45404552175143359, 0.45710134382777989, 0.46015878101428509, 0.46321771820016633, 0.46627804021816788, 0.46933963184889549, 0.47240237782515471, 0.47546616283629101, 0.47853087153252949, 0.48159638852932035, 0.48466259841168174, 0.4877293857385438, 0.49079663504709742, 0.49386423085714021, 0.49693205767542281, 0.49999999999999989, 0.50306794232457708, 0.50613576914285963, 0.50920336495290242, 0.51227061426145604, 0.51533740158831809, 0.51840361147067948, 0.5214691284674704, 0.52453383716370883, 0.52759762217484507, 0.53066036815110429, 0.5337219597818319, 0.53678228179983345, 0.53984121898571469, 0.54289865617221988, 0.54595447824856624, 0.54900857016478, 0.55206081693602738, 0.55511110364694149, 0.55815931545595221, 0.56120533759960822, 0.56424905539689663, 0.567290354253563, 0.57032911966642441, 0.57336523722768051, 0.57639859262922166, 0.57942907166693058, 0.58245656024498471, 0.58548094438015064, 0.58850211020607435, 0.59151994397757024, 0.59453433207490325, 0.5975451610080641, 0.60055231742104587, 0.60355568809610904, 0.60655515995804532, 0.60955062007843486, 0.61254195567989622, 0.61552905414033532, 0.61851180299718367, 0.62149008995163191, 0.62446380287285996, 0.62743282980225745, 0.63039705895763776, 0.6333563787374491, 0.63631067772497429, 0.63925984469252617, 0.64220376860563588, 0.64514233862723103, 0.64807544412181162, 0.6510029746596141, 0.65392482002076735, 0.65684087019944559, 0.65975101540800796, 0.66265514608113152, 0.6655531528799381, 0.66844492669610978, 0.67133035865599688, 0.67420934012471723, 0.67708176271024501, 0.67994751826749389, 0.68280649890238698, 0.68565859697591869, 0.68850370510820902, 0.69134171618254503, 0.69417252334941315, 0.69699602003052397, 0.69981209992282323, 0.70262065700249465, 0.70542158552895196, 0.70821478004881844, 0.71100013539989959, 0.71377754671514104, 0.71654690942657595, 0.71930811926926363, 0.72206107228521477, 0.72480566482730335, 0.72754179356317183, 0.7302693554791202, 0.73298824788398276, 0.73569836841299874, 0.73839961503166096, 0.74109188603956111, 0.743775080074218, 0.74644909611489196, 0.74911383348639071, 0.75176919186285884, 0.75441507127155349, 0.75705137209661078, 0.75967799508279499, 0.76229484133923409, 0.76490181234314725, 0.76749880994354847, 0.77008573636494626, 0.77266249421102318, 0.77522898646830229, 0.77778511650980087, 0.78033078809866807, 0.78286590539180656, 0.78539037294348346, 0.78790409570892272, 0.79040697904788193, 0.79289892872821932, 0.79537985092943697, 0.7978496522462164, 0.80030823969193454, 0.80275552070216261, 0.80519140313815452, 0.80761579529031347, 0.8100286058816446, 0.81242974407119306, 0.81481911945746366, 0.81719664208182241, 0.81956222243188781, 0.82191577144489558, 0.824257200511056, 0.82658642147688832, 0.82890334664853926, 0.83120788879508578, 0.83349996115181879, 0.83577947742350922, 0.83804635178765785, 0.84030049889772662, 0.84254183388634996, 0.84477027236853341, 0.84698573044482683, 0.84918812470448612, 0.85137737222861265, 0.85355339059327373, 0.85571609787260805, 0.85786541264190941, 0.86000125398069083, 0.86212354147573333, 0.86423219522411276, 0.86632713583620613, 0.8684082844386849, 0.87047556267747939, 0.87252889272073275, 0.87456819726172963, 0.87659339952180615, 0.87860442325324217, 0.880601192742131, 0.88258363281122942, 0.88455166882278968, 0.88650522668136855, 0.888444232836616, 0.89036861428604719, 0.89227829857778751, 0.89417321381330295, 0.89605328865010625, 0.89791845230444167, 0.8997686345539524, 0.90160376574032253, 0.90342377677189956, 0.90522859912629738, 0.90701816485297426, 0.90879240657579163, 0.91055125749555232, 0.91229465139251253, 0.91402252262887773, 0.91573480615127267, 0.91743143749319001, 0.91911235277741887, 0.92077748871844922, 0.92242678262485356, 0.92406017240164862, 0.92567759655263271, 0.92727899418269999, 0.92886430500013595, 0.9304334693188836, 0.93198642806079324, 0.93352312275784632, 0.93504349555435562, 0.93654748920914499, 0.93803504709770336, 0.93950611321431676, 0.94096063217417747, 0.94239854921546895, 0.9438198102014268, 0.94522436162237888, 0.94661215059775761, 0.94798312487809244, 0.94933723284697691, 0.9506744235230109, 0.9519946465617215, 0.95329785225745778, 0.95458399154526119, 0.95585301600271488, 0.95710487785176546, 0.95833952996052119, 0.95955692584502883, 0.9607570196710209, 0.96193976625564326, 0.96310512106915569, 0.96425304023660774, 0.96538348053949186, 0.96649639941736942, 0.96759175496947369, 0.96866950595628742, 0.96972961180109496, 0.97077203259151024, 0.97179672908098014, 0.97280366269026053, 0.97379279550887043, 0.97476409029651834, 0.97571751048450417, 0.97665302017709688, 0.97757058415288545, 0.97847016786610441, 0.97935173744793569, 0.98021525970778289, 0.98106070213452068, 0.98188803289771998, 0.9826972208488447, 0.98348823552242592, 0.98426104713720863, 0.98501562659727204, 0.98575194549312584, 0.98646997610278009, 0.98716969139278787, 0.98785106501926423, 0.98851407132887714, 0.98915868535981377, 0.98978488284272026, 0.99039264020161522, 0.99098193455477757, 0.9915527437156082, 0.99210504619346451, 0.99263882119447056, 0.99315404862229939, 0.99365070907892916, 0.99412878386537473, 0.99458825498239056, 0.99502910513114851, 0.99545131771388995, 0.99585487683454976, 0.99623976729935493, 0.99660597461739719, 0.99695348500117809, 0.99728228536712771, 0.99759236333609846, 0.99788370723382991, 0.99815630609138895, 0.99841014964558283, 0.99864522833934499, 0.99886153332209582, 0.99905905645007453, 0.99923779028664739, 0.9993977281025862, 0.99953886387632274, 0.99966119229417472, 0.99976470875054657, 0.99984940934810207, 0.99991529089791165, 0.99996235091957231, 0.99999058764130055, 1.0};

typedef struct {
    pyo_audio_HEAD
    PyObject *table;
    PyObject *pitch;
    Stream *pitch_stream;
    PyObject *start;
    Stream *start_stream;
    PyObject *dur;
    Stream *dur_stream;
    PyObject *xfade;
    Stream *xfade_stream;
    int xfadeshape;
    int startfromloop;
    int init;
    int mode; /* 0 = no loop, 1 = forward, 2 = backward, 3 = back-and-forth */
    int tmpmode;
    MYFLT pointerPos[2];
    int active[2];
    long loopstart[2];
    long loopend[2];
    long crossfadedur[2];
    MYFLT crossfadescaling[2];
    long minfadepoint[2];
    long maxfadepoint[2];
    MYFLT *fader;
    int interp; /* 0 = default to 2, 1 = nointerp, 2 = linear, 3 = cos, 4 = cubic */
    MYFLT (*interp_func_ptr)(MYFLT *, int, MYFLT, int);
    int modebuffer[6];
    int autosmooth;
    MYFLT lastpitch;
    // sample memories
    MYFLT y1;
    MYFLT y2;
    // variables
    MYFLT c1;
    MYFLT c2;
    
} Looper;

static void
Looper_reset(Looper *self, int x, int which, int init) {
    MYFLT start, dur, xfade;

    int size = TableStream_getSize(self->table) - 1;
    double tableSr = TableStream_getSamplingRate(self->table);

    if (self->modebuffer[3] == 0)
        start = PyFloat_AS_DOUBLE(self->start);
    else
        start = Stream_getData((Stream *)self->start_stream)[x];
    if (self->modebuffer[4] == 0)
        dur = PyFloat_AS_DOUBLE(self->dur);
    else
        dur = Stream_getData((Stream *)self->dur_stream)[x];
    if (self->modebuffer[5] == 0)
        xfade = PyFloat_AS_DOUBLE(self->xfade);
    else
        xfade = Stream_getData((Stream *)self->xfade_stream)[x];
    
    if (start < 0.0)
        start = 0.0;
    else if (start > (size/tableSr))
        start = (MYFLT)(size/tableSr);
    if (dur < 0.001)
        dur = 0.001;
    if (xfade < 0.0)
        xfade = 0.0;
    else if (xfade > 50.0)
        xfade = 50.0;

    if (self->xfadeshape == 0)
        self->fader = LOOPER_LINEAR_FADE;
    else if (self->xfadeshape == 1)
        self->fader = LOOPER_POWER_FADE;
    else if (self->xfadeshape == 2)
        self->fader = LOOPER_SIGMOID_FADE;
    else
        self->fader = LOOPER_LINEAR_FADE;
    
    if (self->tmpmode != self->mode) {
        self->mode = self->tmpmode;
        self->active[0] = self->active[1] = 0;
        which = 0;
    }
    
    switch (self->mode) {
        case 0:
            self->loopstart[which] = 0;
            self->loopend[which] = (long)size;
                self->crossfadedur[which] = 5;
            self->crossfadescaling[which] = 1.0 / self->crossfadedur[which] * 512.0;
            if (init == 1 && self->startfromloop == 0) {
                self->minfadepoint[which] = self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] - self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which] = 0.0;
            }
            else {
                self->minfadepoint[which] = self->loopstart[which] + self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] - self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which];        
            }            
            break;
        case 1:
            self->loopstart[which] = (long)(start * tableSr);
            self->loopend[which] = (long)((start + dur) * tableSr);
            self->crossfadedur[which] = (long)((self->loopend[which] - self->loopstart[which]) * xfade * 0.01);
            if (self->crossfadedur[which] < 5)
                self->crossfadedur[which] = 5;
            self->crossfadescaling[which] = 1.0 / self->crossfadedur[which] * 512.0;
            if (init == 1 && self->startfromloop == 0) {
                self->minfadepoint[which] = self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] - self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which] = 0.0;
            }
            else {
                self->minfadepoint[which] = self->loopstart[which] + self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] - self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which];        
            }            
            break;
        case 2:
            self->loopstart[which] = (long)(start * tableSr);
            self->loopend[which] = (long)((start - dur) * tableSr);
            self->crossfadedur[which] = (long)((self->loopstart[which] - self->loopend[which]) * xfade * 0.01);
            if (self->crossfadedur[which] < 5)
                self->crossfadedur[which] = 5;
            self->crossfadescaling[which] = 1.0 / self->crossfadedur[which] * 512.0;
            if (init == 1 && self->startfromloop == 0) {
                self->minfadepoint[which] = size - self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] + self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which] = (MYFLT)size;
            }
            else {
                self->minfadepoint[which] = self->loopstart[which] - self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] + self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which];        
            }            
            break;
        case 3:
            if (which == 0) {
                self->loopstart[which] = (long)(start * tableSr);
                self->loopend[which] = (long)((start + dur) * tableSr);
                self->crossfadedur[which] = (long)((self->loopend[which] - self->loopstart[which]) * xfade * 0.01);
                if (self->crossfadedur[which] < 5)
                    self->crossfadedur[which] = 5;
                self->crossfadescaling[which] = 1.0 / self->crossfadedur[which] * 512.0;
                if (init == 1 && self->startfromloop == 0) {
                    self->minfadepoint[which] = self->crossfadedur[which];
                    self->maxfadepoint[which] = self->loopend[which] - self->crossfadedur[which];
                    self->pointerPos[which] = self->loopstart[which] = 0.0;
                }
                else {
                    self->minfadepoint[which] = self->loopstart[which] + self->crossfadedur[which];
                    self->maxfadepoint[which] = self->loopend[which] - self->crossfadedur[which];
                    self->pointerPos[which] = self->loopstart[which];        
                }
            }
            else {
                self->loopstart[which] = (long)((start + dur) * tableSr);
                self->loopend[which] = (long)(start * tableSr);
                self->crossfadedur[which] = (long)((self->loopstart[which] - self->loopend[which]) * xfade * 0.01);
                if (self->crossfadedur[which] < 5)
                    self->crossfadedur[which] = 5;
                self->crossfadescaling[which] = 1.0 / self->crossfadedur[which] * 512.0;
                self->minfadepoint[which] = self->loopstart[which] - self->crossfadedur[which];
                self->maxfadepoint[which] = self->loopend[which] + self->crossfadedur[which];
                self->pointerPos[which] = self->loopstart[which];
            }
            break;
    }

    self->active[which] = 1;
}

static void
Looper_transform_i(Looper *self) {
    MYFLT pit, fpart, amp, fr, b;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    double tableSr = TableStream_getSamplingRate(self->table);

    MYFLT pitval = PyFloat_AS_DOUBLE(self->pitch);
    if (pitval < 0.0)
        pitval = 0.0;
    
    pit = pitval * tableSr / self->sr;
    
    if (self->active[0] == 0 && self->active[1] == 0) {
        Looper_reset(self, 0, 0, 1);
    }
    
    switch (self->mode) {
        case 0:            
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (self->active[0] == 1) {
                    if (self->pointerPos[0] > size)
                        self->data[i] += 0.0;
                    else {
                        if (self->pointerPos[0] < self->minfadepoint[0]) {
                            fpart = (self->pointerPos[0] - self->loopstart[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else if (self->pointerPos[0] > self->maxfadepoint[0]) {
                            fpart = (self->loopend[0] - self->pointerPos[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else
                            amp = 1.0;
                        ipart = (int)self->pointerPos[0];
                        fpart = self->pointerPos[0] - ipart;
                        self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                    }
                    self->pointerPos[0] += pit;
                    if (self->pointerPos[0] < 0)
                        self->pointerPos[0] = 0.0;
                    else if (self->pointerPos[0] >= self->loopend[0]) {
                        self->active[0] = 0;
                        PyObject_CallMethod((PyObject *)self, "stop", NULL);
                    }
                }
            }
            break;
        case 1:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                for (j=0; j<2; j++) {
                    if (self->active[j] == 1) {
                        if (self->pointerPos[j] > size)
                            self->data[i] += 0.0;
                        else {
                            if (self->pointerPos[j] < self->minfadepoint[j]) {
                                fpart = (self->pointerPos[j] - self->loopstart[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else if (self->pointerPos[j] > self->maxfadepoint[j]) {
                                fpart = (self->loopend[j] - self->pointerPos[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else
                                amp = 1.0;
                            ipart = (int)self->pointerPos[j];
                            fpart = self->pointerPos[j] - ipart;
                            self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                        }
                        self->pointerPos[j] += pit;
                        if (self->pointerPos[j] < 0)
                            self->pointerPos[j] = 0.0;
                        else if (self->pointerPos[j] > self->maxfadepoint[j] && self->active[1-j] == 0)
                            Looper_reset(self, i, 1-j, 0);                    
                        else if (self->pointerPos[j] >= self->loopend[j])
                            self->active[j] = 0;
                    }
                }
            }
            break;
        case 2:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                for (j=0; j<2; j++) {
                    if (self->active[j] == 1) {
                        if (self->pointerPos[j] < 0.0)
                            self->data[i] += 0.0;
                        else {
                            if (self->pointerPos[j] > self->minfadepoint[j]) {
                                fpart = (self->loopstart[j] - self->pointerPos[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else if (self->pointerPos[j] < self->maxfadepoint[j]) {
                                fpart = (self->pointerPos[j] - self->loopend[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else
                                amp = 1.0;
                            ipart = (int)self->pointerPos[j];
                            fpart = self->pointerPos[j] - ipart;
                            self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                        }
                        self->pointerPos[j] -= pit;
                        if (self->pointerPos[j] >= size)
                            self->pointerPos[j] = size-1;
                        else if (self->pointerPos[j] < self->maxfadepoint[j] && self->active[1-j] == 0)
                            Looper_reset(self, i, 1-j, 0);                    
                        else if (self->pointerPos[j] <= self->loopend[j])
                            self->active[j] = 0;
                    }
                }
            }
            break;
        case 3:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                if (self->active[0] == 1) {
                    if (self->pointerPos[0] > size)
                        self->data[i] += 0.0;
                    else {
                        if (self->pointerPos[0] < self->minfadepoint[0]) {
                            fpart = (self->pointerPos[0] - self->loopstart[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else if (self->pointerPos[0] > self->maxfadepoint[0]) {
                            fpart = (self->loopend[0] - self->pointerPos[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else
                            amp = 1.0;
                        ipart = (int)self->pointerPos[0];
                        fpart = self->pointerPos[0] - ipart;
                        self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                    }
                    self->pointerPos[0] += pit;
                    if (self->pointerPos[0] < 0)
                        self->pointerPos[0] = 0.0;
                    else if (self->pointerPos[0] > self->maxfadepoint[0] && self->active[1] == 0)
                        Looper_reset(self, i, 1, 0);                    
                    else if (self->pointerPos[0] >= self->loopend[0])
                        self->active[0] = 0;
                } 
                if (self->active[1] == 1) {
                    if (self->pointerPos[1] < 0.0)
                        self->data[i] += 0.0;
                    else {
                        if (self->pointerPos[1] > self->minfadepoint[1]) {
                            fpart = (self->loopstart[1] - self->pointerPos[1]) * self->crossfadescaling[1];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else if (self->pointerPos[1] < self->maxfadepoint[1]) {
                            fpart = (self->pointerPos[1] - self->loopend[1]) * self->crossfadescaling[1];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else
                            amp = 1.0;
                        ipart = (int)self->pointerPos[1];
                        fpart = self->pointerPos[1] - ipart;
                        self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                    }
                    self->pointerPos[1] -= pit;
                    if (self->pointerPos[1] >= size)
                        self->pointerPos[1] = size-1;
                    else if (self->pointerPos[1] < self->maxfadepoint[1] && self->active[0] == 0)
                        Looper_reset(self, i, 0, 0);                    
                    else if (self->pointerPos[1] <= self->loopend[1])
                        self->active[1] = 0;
                }
            }
            break;
    }

    /* Automatic smoothering of low transposition */
    if (self->autosmooth == 1 && pitval < 1.0 && pit > 0.0) {
        if (self->lastpitch != pitval) {
            self->lastpitch = pitval;
            fr = pitval * tableSr * 0.45;
            b = 2.0 - MYCOS(TWOPI * fr / self->sr);
            self->c2 = (b - MYSQRT(b * b - 1.0));
            self->c1 = 1.0 - self->c2;
        }
        for (i=0; i<self->bufsize; i++) {
            self->y1 = self->c1 * self->data[i] + self->c2 * self->y1;
            self->y2 = self->c1 * self->y1 + self->c2 * self->y2;
            self->data[i] = self->y2;
        }
    }
}

static void
Looper_transform_a(Looper *self) {
    MYFLT fpart, amp, pit, fr, b;
    int i, j, ipart;
    
    MYFLT *tablelist = TableStream_getData(self->table);
    int size = TableStream_getSize(self->table);
    double tableSr = TableStream_getSamplingRate(self->table);
    double ratio = tableSr / self->sr;
    
    MYFLT *pitch = Stream_getData((Stream *)self->pitch_stream);
    
    if (self->active[0] == 0 && self->active[1] == 0) {
        Looper_reset(self, 0, 0, 1);
    }
    
    switch (self->mode) {
        case 0:            
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                pit = pitch[i];
                if (pit < 0.0)
                    pit = 0.0;    
                pit = pit * ratio;
                if (self->active[0] == 1) {
                    if (self->pointerPos[0] > size)
                        self->data[i] += 0.0;
                    else {
                        if (self->pointerPos[0] < self->minfadepoint[0]) {
                            fpart = (self->pointerPos[0] - self->loopstart[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else if (self->pointerPos[0] > self->maxfadepoint[0]) {
                            fpart = (self->loopend[0] - self->pointerPos[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else
                            amp = 1.0;
                        ipart = (int)self->pointerPos[0];
                        fpart = self->pointerPos[0] - ipart;
                        self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                    }
                    self->pointerPos[0] += pit;
                    if (self->pointerPos[0] < 0)
                        self->pointerPos[0] = 0.0;
                    else if (self->pointerPos[0] >= self->loopend[0]) {
                        self->active[0] = 0;
                        PyObject_CallMethod((PyObject *)self, "stop", NULL);
                    }
                }
            }
            break;
        case 1:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                pit = pitch[i];
                if (pit < 0.0)
                    pit = 0.0;    
                pit = pit * ratio;
                for (j=0; j<2; j++) {
                    if (self->active[j] == 1) {
                        if (self->pointerPos[j] > size)
                            self->data[i] += 0.0;
                        else {
                            if (self->pointerPos[j] < self->minfadepoint[j]) {
                                fpart = (self->pointerPos[j] - self->loopstart[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else if (self->pointerPos[j] > self->maxfadepoint[j]) {
                                fpart = (self->loopend[j] - self->pointerPos[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else
                                amp = 1.0;
                            ipart = (int)self->pointerPos[j];
                            fpart = self->pointerPos[j] - ipart;
                            self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                        }
                        self->pointerPos[j] += pit;
                        if (self->pointerPos[j] < 0)
                            self->pointerPos[j] = 0.0;
                        else if (self->pointerPos[j] > self->maxfadepoint[j] && self->active[1-j] == 0)
                            Looper_reset(self, i, 1-j, 0);                    
                        else if (self->pointerPos[j] >= self->loopend[j])
                            self->active[j] = 0;
                    }
                }
            }
            break;
        case 2:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                pit = pitch[i];
                if (pit < 0.0)
                    pit = 0.0;    
                pit = pit * ratio;
                for (j=0; j<2; j++) {
                    if (self->active[j] == 1) {
                        if (self->pointerPos[j] < 0.0)
                            self->data[i] += 0.0;
                        else {
                            if (self->pointerPos[j] > self->minfadepoint[j]) {
                                fpart = (self->loopstart[j] - self->pointerPos[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else if (self->pointerPos[j] < self->maxfadepoint[j]) {
                                fpart = (self->pointerPos[j] - self->loopend[j]) * self->crossfadescaling[j];
                                ipart = (int)fpart;
                                fpart = fpart - ipart;
                                amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                            }
                            else
                                amp = 1.0;
                            ipart = (int)self->pointerPos[j];
                            fpart = self->pointerPos[j] - ipart;
                            self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                        }
                        self->pointerPos[j] -= pit;
                        if (self->pointerPos[j] >= size)
                            self->pointerPos[j] = size-1;
                        else if (self->pointerPos[j] < self->maxfadepoint[j] && self->active[1-j] == 0)
                            Looper_reset(self, i, 1-j, 0);                    
                        else if (self->pointerPos[j] <= self->loopend[j])
                            self->active[j] = 0;
                    }
                }
            }
            break;
        case 3:
            for (i=0; i<self->bufsize; i++) {
                self->data[i] = 0.0;
                pit = pitch[i];
                if (pit < 0.0)
                    pit = 0.0;    
                pit = pit * ratio;
                if (self->active[0] == 1) {
                    if (self->pointerPos[0] > size)
                        self->data[i] += 0.0;
                    else {
                        if (self->pointerPos[0] < self->minfadepoint[0]) {
                            fpart = (self->pointerPos[0] - self->loopstart[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else if (self->pointerPos[0] > self->maxfadepoint[0]) {
                            fpart = (self->loopend[0] - self->pointerPos[0]) * self->crossfadescaling[0];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else
                            amp = 1.0;
                        ipart = (int)self->pointerPos[0];
                        fpart = self->pointerPos[0] - ipart;
                        self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                    }
                    self->pointerPos[0] += pit;
                    if (self->pointerPos[0] < 0)
                        self->pointerPos[0] = 0.0;
                    else if (self->pointerPos[0] > self->maxfadepoint[0] && self->active[1] == 0)
                        Looper_reset(self, i, 1, 0);                    
                    else if (self->pointerPos[0] >= self->loopend[0])
                        self->active[0] = 0;
                } 
                if (self->active[1] == 1) {
                    if (self->pointerPos[1] < 0.0)
                        self->data[i] += 0.0;
                    else {
                        if (self->pointerPos[1] > self->minfadepoint[1]) {
                            fpart = (self->loopstart[1] - self->pointerPos[1]) * self->crossfadescaling[1];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else if (self->pointerPos[1] < self->maxfadepoint[1]) {
                            fpart = (self->pointerPos[1] - self->loopend[1]) * self->crossfadescaling[1];
                            ipart = (int)fpart;
                            fpart = fpart - ipart;
                            amp = self->fader[ipart] + (self->fader[ipart+1] - self->fader[ipart]) * fpart;
                        }
                        else
                            amp = 1.0;
                        ipart = (int)self->pointerPos[1];
                        fpart = self->pointerPos[1] - ipart;
                        self->data[i] += (*self->interp_func_ptr)(tablelist, ipart, fpart, size) * amp;
                    }
                    self->pointerPos[1] -= pit;
                    if (self->pointerPos[1] >= size)
                        self->pointerPos[1] = size-1;
                    else if (self->pointerPos[1] < self->maxfadepoint[1] && self->active[0] == 0)
                        Looper_reset(self, i, 0, 0);                    
                    else if (self->pointerPos[1] <= self->loopend[1])
                        self->active[1] = 0;
                }
            }
            break;
    }
    /* Automatic smoothering of low transposition */
    if (self->autosmooth == 1) {
        for (i=0; i<self->bufsize; i++) {
            pit = pitch[i];
            if (pit < 0.0)
                pit = 0.0;    
            if (pit < 1.0 && pit > 0.0) {
                if (self->lastpitch != pit) {
                    self->lastpitch = pit;
                    fr = pit * tableSr * 0.45;
                    b = 2.0 - MYCOS(TWOPI * fr / self->sr);
                    self->c2 = (b - MYSQRT(b * b - 1.0));
                    self->c1 = 1.0 - self->c2;
                }
                for (i=0; i<self->bufsize; i++) {
                    self->y1 = self->c1 * self->data[i] + self->c2 * self->y1;
                    self->y2 = self->c1 * self->y1 + self->c2 * self->y2;
                    self->data[i] = self->y2;
                }
            }
        }
    }
}

static void Looper_postprocessing_ii(Looper *self) { POST_PROCESSING_II };
static void Looper_postprocessing_ai(Looper *self) { POST_PROCESSING_AI };
static void Looper_postprocessing_ia(Looper *self) { POST_PROCESSING_IA };
static void Looper_postprocessing_aa(Looper *self) { POST_PROCESSING_AA };
static void Looper_postprocessing_ireva(Looper *self) { POST_PROCESSING_IREVA };
static void Looper_postprocessing_areva(Looper *self) { POST_PROCESSING_AREVA };
static void Looper_postprocessing_revai(Looper *self) { POST_PROCESSING_REVAI };
static void Looper_postprocessing_revaa(Looper *self) { POST_PROCESSING_REVAA };
static void Looper_postprocessing_revareva(Looper *self) { POST_PROCESSING_REVAREVA };

static void
Looper_setProcMode(Looper *self)
{
    int procmode, muladdmode;
    procmode = self->modebuffer[2];
    muladdmode = self->modebuffer[0] + self->modebuffer[1] * 10;
    
	switch (procmode) {
        case 0:    
            self->proc_func_ptr = Looper_transform_i;
            break;
        case 1:    
            self->proc_func_ptr = Looper_transform_a;
            break;
    } 
	switch (muladdmode) {
        case 0:        
            self->muladd_func_ptr = Looper_postprocessing_ii;
            break;
        case 1:    
            self->muladd_func_ptr = Looper_postprocessing_ai;
            break;
        case 2:    
            self->muladd_func_ptr = Looper_postprocessing_revai;
            break;
        case 10:        
            self->muladd_func_ptr = Looper_postprocessing_ia;
            break;
        case 11:    
            self->muladd_func_ptr = Looper_postprocessing_aa;
            break;
        case 12:    
            self->muladd_func_ptr = Looper_postprocessing_revaa;
            break;
        case 20:        
            self->muladd_func_ptr = Looper_postprocessing_ireva;
            break;
        case 21:    
            self->muladd_func_ptr = Looper_postprocessing_areva;
            break;
        case 22:    
            self->muladd_func_ptr = Looper_postprocessing_revareva;
            break;
    }   
}

static void
Looper_compute_next_data_frame(Looper *self)
{
    (*self->proc_func_ptr)(self); 
    (*self->muladd_func_ptr)(self);
}

static int
Looper_traverse(Looper *self, visitproc visit, void *arg)
{
    pyo_VISIT
    Py_VISIT(self->table);
    Py_VISIT(self->pitch);    
    Py_VISIT(self->pitch_stream);    
    Py_VISIT(self->start);    
    Py_VISIT(self->start_stream);    
    Py_VISIT(self->dur);    
    Py_VISIT(self->dur_stream);    
    Py_VISIT(self->xfade);    
    Py_VISIT(self->xfade_stream);    
    return 0;
}

static int 
Looper_clear(Looper *self)
{
    pyo_CLEAR
    Py_CLEAR(self->table);
    Py_CLEAR(self->pitch);    
    Py_CLEAR(self->pitch_stream);    
    Py_CLEAR(self->start);    
    Py_CLEAR(self->start_stream);    
    Py_CLEAR(self->dur);    
    Py_CLEAR(self->dur_stream);    
    Py_CLEAR(self->xfade);    
    Py_CLEAR(self->xfade_stream);    
    return 0;
}

static void
Looper_dealloc(Looper* self)
{
    free(self->data);   
    Looper_clear(self);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject * Looper_deleteStream(Looper *self) { DELETE_STREAM };

static PyObject *
Looper_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int i;
    Looper *self;
    self = (Looper *)type->tp_alloc(type, 0);
    
    self->pitch = PyFloat_FromDouble(1.0);
    self->start = PyFloat_FromDouble(0.0);
    self->dur = PyFloat_FromDouble(1.0);
    self->xfade = PyFloat_FromDouble(20.0);
    self->lastpitch = -1.0;
    self->autosmooth = 0;
    self->y1 = self->y2 = 0.0;
    self->xfadeshape = 0;
    self->startfromloop = 0;
    self->interp = 2;
    self->init = 1;
    self->mode = self->tmpmode = 1;
    self->pointerPos[0] = self->pointerPos[1] = 0.0;
    self->active[0] = self->active[1] = 0;
	self->modebuffer[0] = 0;
	self->modebuffer[1] = 0;
	self->modebuffer[2] = 0;
	self->modebuffer[3] = 0;
	self->modebuffer[4] = 0;
    
    INIT_OBJECT_COMMON
    Stream_setFunctionPtr(self->stream, Looper_compute_next_data_frame);
    self->mode_func_ptr = Looper_setProcMode;
    
    return (PyObject *)self;
}

static int
Looper_init(Looper *self, PyObject *args, PyObject *kwds)
{
    PyObject *tabletmp, *pitchtmp=NULL, *starttmp=NULL, *durtmp=NULL, *xfadetmp=NULL, *multmp=NULL, *addtmp=NULL;
    
    static char *kwlist[] = {"table", "pitch", "start", "dur", "xfade", "mode", "xfadeshape", "startfromloop", "interp", "autosmooth", "mul", "add", NULL};
    
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOiiiiiOO", kwlist, &tabletmp, &pitchtmp, &starttmp, &durtmp, &xfadetmp, &self->tmpmode, &self->xfadeshape, &self->startfromloop, &self->interp, &self->autosmooth, &multmp, &addtmp))
        return -1; 
    
    Py_XDECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tabletmp, "getTableStream", "");

    if (pitchtmp) {
        PyObject_CallMethod((PyObject *)self, "setPitch", "O", pitchtmp);
    }
    
    if (starttmp) {
        PyObject_CallMethod((PyObject *)self, "setStart", "O", starttmp);
    }
    
    if (durtmp) {
        PyObject_CallMethod((PyObject *)self, "setDur", "O", durtmp);
    }

    if (xfadetmp) {
        PyObject_CallMethod((PyObject *)self, "setXfade", "O", xfadetmp);
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
    
    if (self->tmpmode >= 0 && self->tmpmode < 4)
        self->mode = self->tmpmode;
    else
        self->mode = self->tmpmode = 1;

    SET_INTERP_POINTER

    Py_INCREF(self);
    return 0;
}

static PyObject * Looper_getServer(Looper* self) { GET_SERVER };
static PyObject * Looper_getStream(Looper* self) { GET_STREAM };
static PyObject * Looper_setMul(Looper *self, PyObject *arg) { SET_MUL };	
static PyObject * Looper_setAdd(Looper *self, PyObject *arg) { SET_ADD };	
static PyObject * Looper_setSub(Looper *self, PyObject *arg) { SET_SUB };	
static PyObject * Looper_setDiv(Looper *self, PyObject *arg) { SET_DIV };	

static PyObject * Looper_play(Looper *self, PyObject *args, PyObject *kwds) { PLAY };
static PyObject * Looper_out(Looper *self, PyObject *args, PyObject *kwds) { OUT };
static PyObject * Looper_stop(Looper *self) { STOP };

static PyObject * Looper_multiply(Looper *self, PyObject *arg) { MULTIPLY };
static PyObject * Looper_inplace_multiply(Looper *self, PyObject *arg) { INPLACE_MULTIPLY };
static PyObject * Looper_add(Looper *self, PyObject *arg) { ADD };
static PyObject * Looper_inplace_add(Looper *self, PyObject *arg) { INPLACE_ADD };
static PyObject * Looper_sub(Looper *self, PyObject *arg) { SUB };
static PyObject * Looper_inplace_sub(Looper *self, PyObject *arg) { INPLACE_SUB };
static PyObject * Looper_div(Looper *self, PyObject *arg) { DIV };
static PyObject * Looper_inplace_div(Looper *self, PyObject *arg) { INPLACE_DIV };

static PyObject *
Looper_setPitch(Looper *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->pitch);
	if (isNumber == 1) {
		self->pitch = PyNumber_Float(tmp);
        self->modebuffer[2] = 0;
	}
	else {
		self->pitch = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->pitch, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->pitch_stream);
        self->pitch_stream = (Stream *)streamtmp;
		self->modebuffer[2] = 1;
	}
    
    (*self->mode_func_ptr)(self);
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Looper_setStart(Looper *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->start);
	if (isNumber == 1) {
		self->start = PyNumber_Float(tmp);
        self->modebuffer[3] = 0;
	}
	else {
		self->start = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->start, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->start_stream);
        self->start_stream = (Stream *)streamtmp;
		self->modebuffer[3] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Looper_setDur(Looper *self, PyObject *arg)
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
        self->modebuffer[4] = 0;
	}
	else {
		self->dur = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->dur, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->dur_stream);
        self->dur_stream = (Stream *)streamtmp;
		self->modebuffer[4] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Looper_setXfade(Looper *self, PyObject *arg)
{
	PyObject *tmp, *streamtmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	int isNumber = PyNumber_Check(arg);
	
	tmp = arg;
	Py_INCREF(tmp);
	Py_DECREF(self->xfade);
	if (isNumber == 1) {
		self->xfade = PyNumber_Float(tmp);
        self->modebuffer[5] = 0;
	}
	else {
		self->xfade = tmp;
        streamtmp = PyObject_CallMethod((PyObject *)self->xfade, "_getStream", NULL);
        Py_INCREF(streamtmp);
        Py_XDECREF(self->xfade_stream);
        self->xfade_stream = (Stream *)streamtmp;
		self->modebuffer[5] = 1;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
Looper_getTable(Looper* self)
{
    Py_INCREF(self->table);
    return self->table;
};

static PyObject *
Looper_setTable(Looper *self, PyObject *arg)
{
	PyObject *tmp;
	
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	tmp = arg;
	Py_DECREF(self->table);
    self->table = PyObject_CallMethod((PyObject *)tmp, "getTableStream", "");
    
	Py_INCREF(Py_None);
	return Py_None;
}	

static PyObject *
Looper_setStartFromLoop(Looper *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->startfromloop = PyInt_AsLong(arg);
    }  
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Looper_setXfadeShape(Looper *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->xfadeshape = PyInt_AsLong(arg);
    }  

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Looper_setMode(Looper *self, PyObject *arg)
{
    int tmp;
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		tmp = PyInt_AsLong(arg);
        if (tmp >= 0 && tmp < 4)
            self->tmpmode = tmp;
    }  
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Looper_setInterp(Looper *self, PyObject *arg)
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

static PyObject *
Looper_setAutoSmooth(Looper *self, PyObject *arg)
{
	if (arg == NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
    int isInt = PyInt_Check(arg);
    
	if (isInt == 1) {
		self->autosmooth = PyInt_AsLong(arg);
    }  
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMemberDef Looper_members[] = {
    {"server", T_OBJECT_EX, offsetof(Looper, server), 0, "Pyo server."},
    {"stream", T_OBJECT_EX, offsetof(Looper, stream), 0, "Stream object."},
    {"table", T_OBJECT_EX, offsetof(Looper, table), 0, "Sound table."},
    {"pitch", T_OBJECT_EX, offsetof(Looper, pitch), 0, "Speed of the reading pointer."},
    {"start", T_OBJECT_EX, offsetof(Looper, start), 0, "Position in the sound table."},
    {"dur", T_OBJECT_EX, offsetof(Looper, dur), 0, "Duration of each grains."},
    {"mul", T_OBJECT_EX, offsetof(Looper, mul), 0, "Mul factor."},
    {"add", T_OBJECT_EX, offsetof(Looper, add), 0, "Add factor."},
    {NULL}  /* Sentinel */
};

static PyMethodDef Looper_methods[] = {
    {"getTable", (PyCFunction)Looper_getTable, METH_NOARGS, "Returns sound table object."},
    {"setTable", (PyCFunction)Looper_setTable, METH_O, "Sets sound table."},
    {"getServer", (PyCFunction)Looper_getServer, METH_NOARGS, "Returns server object."},
    {"_getStream", (PyCFunction)Looper_getStream, METH_NOARGS, "Returns stream object."},
    {"deleteStream", (PyCFunction)Looper_deleteStream, METH_NOARGS, "Remove stream from server and delete the object."},
    {"play", (PyCFunction)Looper_play, METH_VARARGS|METH_KEYWORDS, "Starts computing without sending sound to soundcard."},
    {"out", (PyCFunction)Looper_out, METH_VARARGS|METH_KEYWORDS, "Starts computing and sends sound to soundcard channel speficied by argument."},
    {"stop", (PyCFunction)Looper_stop, METH_NOARGS, "Stops computing."},
	{"setPitch", (PyCFunction)Looper_setPitch, METH_O, "Sets global pitch factor."},
    {"setStart", (PyCFunction)Looper_setStart, METH_O, "Sets position in the sound table."},
    {"setDur", (PyCFunction)Looper_setDur, METH_O, "Sets the grain duration."},
    {"setXfade", (PyCFunction)Looper_setXfade, METH_O, "Sets crossfade length in percent."},
    {"setXfadeShape", (PyCFunction)Looper_setXfadeShape, METH_O, "Sets crossfade shape."},
    {"setMode", (PyCFunction)Looper_setMode, METH_O, "Sets looping mode (0 = no loop, 1 = forward, 2 = backward, 3 = back-and-forth)."},
    {"setStartFromLoop", (PyCFunction)Looper_setStartFromLoop, METH_O, "Sets init pointer position."},
    {"setInterp", (PyCFunction)Looper_setInterp, METH_O, "Sets oscillator interpolation mode."},
    {"setAutoSmooth", (PyCFunction)Looper_setAutoSmooth, METH_O, "Activate lowpass filter for transposition below 1."},
	{"setMul", (PyCFunction)Looper_setMul, METH_O, "Sets granulator mul factor."},
	{"setAdd", (PyCFunction)Looper_setAdd, METH_O, "Sets granulator add factor."},
    {"setSub", (PyCFunction)Looper_setSub, METH_O, "Sets inverse add factor."},
    {"setDiv", (PyCFunction)Looper_setDiv, METH_O, "Sets inverse mul factor."},
    {NULL}  /* Sentinel */
};

static PyNumberMethods Looper_as_number = {
    (binaryfunc)Looper_add,                      /*nb_add*/
    (binaryfunc)Looper_sub,                 /*nb_subtract*/
    (binaryfunc)Looper_multiply,                 /*nb_multiply*/
    (binaryfunc)Looper_div,                   /*nb_divide*/
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
    (binaryfunc)Looper_inplace_add,              /*inplace_add*/
    (binaryfunc)Looper_inplace_sub,         /*inplace_subtract*/
    (binaryfunc)Looper_inplace_multiply,         /*inplace_multiply*/
    (binaryfunc)Looper_inplace_div,           /*inplace_divide*/
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

PyTypeObject LooperType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_pitch*/
    "_pyo.Looper_base",         /*tp_name*/
    sizeof(Looper),         /*tp_basicpitch*/
    0,                         /*tp_itempitch*/
    (destructor)Looper_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    &Looper_as_number,             /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Looper objects. Sound looper with crossfade.",           /* tp_doc */
    (traverseproc)Looper_traverse,   /* tp_traverse */
    (inquiry)Looper_clear,           /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Looper_methods,             /* tp_methods */
    Looper_members,             /* tp_members */
    0,                      /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Looper_init,      /* tp_init */
    0,                         /* tp_alloc */
    Looper_new,                 /* tp_new */
};


