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

#include "Python.h"
#include <math.h>

#ifndef __MYFLT_DEF
#define __MYFLT_DEF

#ifndef USE_DOUBLE
#define LIB_BASE_NAME "_pyo"
#define MYFLT float
#define FLOAT_VALUE f
#define TYPE_F "f"
#define TYPE_F_I "f|i"
#define TYPE__IF "|if"
#define TYPE_F_II "f|ii"
#define TYPE__FFF "|fff"
#define TYPE_O_F "O|f"
#define TYPE_O_FO "O|fO"
#define TYPE__OF "|Of"
#define TYPE_O_FOO "O|fOO"
#define TYPE_I_FFOO "i|ffOO"
#define TYPE_O_IF "O|if"
#define TYPE_S_IFF "s|iff"
#define TYPE_S__OIFI "s|Oifi"
#define TYPE__FFFOO "|fffOO"
#define TYPE__FFFFFOO "|fffffOO"
#define TYPE_O_FFFFOO "O|ffffOO"
#define TYPE_OO_F "OO|f"
#define TYPE_OO_FI "OO|fi"
#define TYPE_OO_IF "OO|if"
#define TYPE_OOO_F "OOO|f"
#define TYPE_F_O "f|O"
#define TYPE_F_OF "f|Of"
#define TYPE__OFFI "|Offi"
#define TYPE__OFII "|Ofii"
#define TYPE_O_OFOO "O|OfOO"
#define TYPE_O_IFFO "O|iffO"
#define TYPE_OO_FFOO "OO|ffOO"
#define TYPE_O_IFIOO "O|ifiOO"
#define TYPE_O_OFOOOO "O|OfOOOO"
#define TYPE_O_OOFOO "O|OOfOO"
#define TYPE_O_OOFFOO "O|OOffOO"
#define TYPE_OO_OOOIFOO "OO|OOOifOO"
//#define TYPE_O_OOOOIIFIOO "O|OOOOiifiOO"

#define SF_WRITE sf_write_float
#define SF_READ sf_read_float

#define MYSQRT sqrtf
#define MYLOG logf
#define MYLOG2 log2f
#define MYLOG10 log10f
#define MYCOS cosf
#define MYSIN sinf
#define MYTAN tanf
#define MYPOW powf
#define MYFABS fabsf
#define MYFMOD fmodf
#define MYFLOOR floorf
#define MYTANH tanhf
#define MYATAN atanf
#define MYATAN2 atan2f
#define MYEXP expf
#define MYROUND roundf

#else
#define LIB_BASE_NAME "_pyo64"
#define MYFLT double
#define FLOAT_VALUE d
#define TYPE_F "d"
#define TYPE_F_I "d|i"
#define TYPE__IF "|id"
#define TYPE_F_II "d|ii"
#define TYPE__FFF "|ddd"
#define TYPE_O_F "O|d"
#define TYPE_O_FO "O|dO"
#define TYPE__OF "|Od"
#define TYPE_O_FOO "O|dOO"
#define TYPE_I_FFOO "i|ddOO"
#define TYPE_O_IF "O|id"
#define TYPE_S_IFF "s|idd"
#define TYPE_S__OIFI "s|Oidi"
#define TYPE__FFFOO "|dddOO"
#define TYPE__FFFFFOO "|dddddOO"
#define TYPE_O_FFFFOO "O|ddddOO"
#define TYPE_OO_F "OO|d"
#define TYPE_OO_FI "OO|di"
#define TYPE_OO_IF "OO|id"
#define TYPE_OOO_F "OOO|d"
#define TYPE_F_O "d|O"
#define TYPE_F_OF "d|Od"
#define TYPE__OFFI "|Oddi"
#define TYPE__OFII "|Odii"
#define TYPE_O_OFOO "O|OdOO"
#define TYPE_O_IFFO "O|iddO"
#define TYPE_OO_FFOO "OO|ddOO"
#define TYPE_O_IFIOO "O|idiOO"
#define TYPE_O_OFOOOO "O|OdOOOO"
#define TYPE_O_OOFOO "O|OOdOO"
#define TYPE_O_OOFFOO "O|OOddOO"
#define TYPE_OO_OOOIFOO "OO|OOOidOO"
//#define TYPE_O_OOOOIIFIOO "O|OOOOiidiOO"

#define SF_WRITE sf_write_double
#define SF_READ sf_read_double

#define MYSQRT sqrt
#define MYLOG log
#define MYLOG2 log2
#define MYLOG10 log10
#define MYCOS cos
#define MYSIN sin
#define MYTAN tan
#define MYPOW pow
#define MYFABS fabs
#define MYFMOD fmod
#define MYFLOOR floor
#define MYTANH tanh
#define MYATAN atan
#define MYATAN2 atan2
#define MYEXP exp
#define MYROUND round


#endif
#endif

extern PyTypeObject SineType;
extern PyTypeObject SineLoopType;
extern PyTypeObject FmType;
extern PyTypeObject CrossFmType;
extern PyTypeObject LFOType;
extern PyTypeObject BlitType;
extern PyTypeObject RosslerType;
extern PyTypeObject RosslerAltType;
extern PyTypeObject LorenzType;
extern PyTypeObject LorenzAltType;
extern PyTypeObject PhasorType;
extern PyTypeObject PointerType;
extern PyTypeObject TableIndexType;
extern PyTypeObject LookupType;
extern PyTypeObject TableReadType;
extern PyTypeObject TableReadTrigType;
extern PyTypeObject OscType;
extern PyTypeObject OscLoopType;
extern PyTypeObject OscBankType;
extern PyTypeObject PulsarType;
extern PyTypeObject NoiseType;
extern PyTypeObject PinkNoiseType;
extern PyTypeObject BrownNoiseType;
extern PyTypeObject InputType;
extern PyTypeObject SfPlayerType;
extern PyTypeObject SfPlayType;
extern PyTypeObject SfPlayTrigType;
extern PyTypeObject SfMarkerShufflerType;
extern PyTypeObject SfMarkerShuffleType;
extern PyTypeObject SfMarkerLooperType;
extern PyTypeObject SfMarkerLoopType;

extern PyTypeObject TrigType;
extern PyTypeObject MetroType;
extern PyTypeObject SeqerType;
extern PyTypeObject SeqType;
extern PyTypeObject ClouderType;
extern PyTypeObject CloudType;
extern PyTypeObject BeaterType;
extern PyTypeObject BeatType;
extern PyTypeObject BeatTapStreamType;
extern PyTypeObject BeatAmpStreamType;
extern PyTypeObject BeatDurStreamType;
extern PyTypeObject BeatEndStreamType;
extern PyTypeObject CounterType;
extern PyTypeObject SelectType;
extern PyTypeObject ChangeType;
extern PyTypeObject ThreshType;
extern PyTypeObject PercentType;

extern PyTypeObject ScoreType;

extern PyTypeObject FaderType;
extern PyTypeObject AdsrType;
extern PyTypeObject LinsegType;
extern PyTypeObject ExpsegType;

extern PyTypeObject RandiType;
extern PyTypeObject RandhType;
extern PyTypeObject ChoiceType;
extern PyTypeObject RandIntType;
extern PyTypeObject XnoiseType;
extern PyTypeObject XnoiseMidiType;

extern PyTypeObject BiquadType;
extern PyTypeObject BiquadxType;
extern PyTypeObject EQType;
extern PyTypeObject ToneType;
extern PyTypeObject DCBlockType;
extern PyTypeObject PortType;
extern PyTypeObject AllpassType;
extern PyTypeObject Allpass2Type;
extern PyTypeObject PhaserType;
extern PyTypeObject DenormType;
extern PyTypeObject DistoType;
extern PyTypeObject ClipType;
extern PyTypeObject MirrorType;
extern PyTypeObject WrapType;
extern PyTypeObject BetweenType;
extern PyTypeObject DegradeType;
extern PyTypeObject CompressType;
extern PyTypeObject GateType;
extern PyTypeObject DelayType;
extern PyTypeObject SDelayType;
extern PyTypeObject WaveguideType;
extern PyTypeObject AllpassWGType;
extern PyTypeObject FreeverbType;
extern PyTypeObject WGVerbType;
extern PyTypeObject ChorusType;
extern PyTypeObject ConvolveType;
extern PyTypeObject IRWinSincType;
extern PyTypeObject IRPulseType;
extern PyTypeObject IRAverageType;
extern PyTypeObject IRFMType;

extern PyTypeObject GranulatorType;
extern PyTypeObject LooperType;
extern PyTypeObject HarmonizerType;

extern PyTypeObject MidictlType;
extern PyTypeObject MidiNoteType;
extern PyTypeObject NoteinType;
extern PyTypeObject MidiAdsrType;

extern PyTypeObject DummyType;
extern PyTypeObject RecordType;
extern PyTypeObject ControlRecType;
extern PyTypeObject ControlReadType;
extern PyTypeObject ControlReadTrigType;
extern PyTypeObject NoteinRecType;
extern PyTypeObject NoteinReadType;
extern PyTypeObject NoteinReadTrigType;
extern PyTypeObject CompareType;
extern PyTypeObject MixType;
extern PyTypeObject SigType;
extern PyTypeObject SigToType;
extern PyTypeObject VarPortType;
extern PyTypeObject InputFaderType;

extern PyTypeObject HarmTableType;
extern PyTypeObject ChebyTableType;
extern PyTypeObject HannTableType;
extern PyTypeObject WinTableType;
extern PyTypeObject ParaTableType;
extern PyTypeObject LinTableType;
extern PyTypeObject CosTableType;
extern PyTypeObject CurveTableType;
extern PyTypeObject ExpTableType;
extern PyTypeObject SndTableType;
extern PyTypeObject DataTableType;
extern PyTypeObject NewTableType;
extern PyTypeObject TableRecType;
extern PyTypeObject TableRecTrigType;
extern PyTypeObject TableMorphType;
extern PyTypeObject TrigTableRecType;
extern PyTypeObject TrigTableRecTrigType;

extern PyTypeObject NewMatrixType;
extern PyTypeObject MatrixPointerType;
extern PyTypeObject MatrixRecType;
extern PyTypeObject MatrixRecTrigType;
extern PyTypeObject MatrixMorphType;

extern PyTypeObject OscSendType;
extern PyTypeObject OscReceiveType;
extern PyTypeObject OscReceiverType;
extern PyTypeObject OscDataSendType;
extern PyTypeObject OscDataReceiveType;

extern PyTypeObject TrigRandIntType;
extern PyTypeObject TrigRandType;
extern PyTypeObject TrigChoiceType;
extern PyTypeObject TrigEnvType;
extern PyTypeObject TrigEnvTrigType;
extern PyTypeObject TrigLinsegType;
extern PyTypeObject TrigLinsegTrigType;
extern PyTypeObject TrigExpsegType;
extern PyTypeObject TrigExpsegTrigType;
extern PyTypeObject TrigFuncType;
extern PyTypeObject TrigXnoiseType;
extern PyTypeObject TrigXnoiseMidiType;

extern PyTypeObject PatternType;
extern PyTypeObject CallAfterType;

extern PyTypeObject BandSplitterType;
extern PyTypeObject BandSplitType;
extern PyTypeObject FourBandMainType;
extern PyTypeObject FourBandType;

extern PyTypeObject HilbertMainType;
extern PyTypeObject HilbertType;

extern PyTypeObject FollowerType;
extern PyTypeObject Follower2Type;
extern PyTypeObject ZCrossType;

extern PyTypeObject SPannerType;
extern PyTypeObject SPanType;
extern PyTypeObject PannerType;
extern PyTypeObject PanType;
extern PyTypeObject SwitcherType;
extern PyTypeObject SwitchType;
extern PyTypeObject SelectorType;
extern PyTypeObject MixerType;
extern PyTypeObject MixerVoiceType;

extern PyTypeObject PrintType;
extern PyTypeObject SnapType;
extern PyTypeObject InterpType;
extern PyTypeObject SampHoldType;

extern PyTypeObject M_SinType;
extern PyTypeObject M_CosType;
extern PyTypeObject M_TanType;
extern PyTypeObject M_AbsType;
extern PyTypeObject M_SqrtType;
extern PyTypeObject M_LogType;
extern PyTypeObject M_Log2Type;
extern PyTypeObject M_Log10Type;
extern PyTypeObject M_PowType;
extern PyTypeObject M_Atan2Type;
extern PyTypeObject M_FloorType;
extern PyTypeObject M_RoundType;

extern PyTypeObject FFTMainType;
extern PyTypeObject FFTType;
extern PyTypeObject IFFTType;
extern PyTypeObject CarToPolType;
extern PyTypeObject PolToCarType;
extern PyTypeObject FrameDeltaMainType;
extern PyTypeObject FrameDeltaType;
extern PyTypeObject FrameAccumMainType;
extern PyTypeObject FrameAccumType;

/* Constants */
#define E M_E
#define PI M_PI
#define TWOPI (2 * M_PI)

/* random uniform (0.0 -> 1.0) */
#define RANDOM_UNIFORM rand()/((MYFLT)(RAND_MAX)+1)

/* object headers */
#define pyo_audio_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    Stream *stream; \
    void (*mode_func_ptr)(); \
    void (*proc_func_ptr)(); \
    void (*muladd_func_ptr)(); \
    PyObject *mul; \
    Stream *mul_stream; \
    PyObject *add; \
    Stream *add_stream; \
    int bufsize; \
    int nchnls; \
    double sr; \
    MYFLT *data; 

#define pyo_table_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    TableStream *tablestream; \
    int size; \
    MYFLT *data;

#define pyo_matrix_HEAD \
    PyObject_HEAD \
    PyObject *server; \
    MatrixStream *matrixstream; \
    int width; \
    int height; \
    MYFLT **data;

/* VISIT & CLEAR */
#define pyo_VISIT \
    Py_VISIT(self->stream); \
    Py_VISIT(self->server); \
    Py_VISIT(self->mul); \
    Py_VISIT(self->mul_stream); \
    Py_VISIT(self->add); \
    Py_VISIT(self->add_stream);    

#define pyo_CLEAR \
    Py_CLEAR(self->stream); \
    Py_CLEAR(self->server); \
    Py_CLEAR(self->mul); \
    Py_CLEAR(self->mul_stream); \
    Py_CLEAR(self->add); \
    Py_CLEAR(self->add_stream);    

#define DELETE_STREAM \
    Server_removeStream((Server *)self->server, Stream_getStreamId(self->stream)); \
    Py_INCREF(Py_None); \
    return Py_None;

/* INIT INPUT STREAM */
#define INIT_INPUT_STREAM \
    Py_XDECREF(self->input); \
    self->input = inputtmp; \
    input_streamtmp = PyObject_CallMethod((PyObject *)self->input, "_getStream", NULL); \
    Py_INCREF(input_streamtmp); \
    Py_XDECREF(self->input_stream); \
    self->input_stream = (Stream *)input_streamtmp;


/* Set data */
#define SET_TABLE_DATA \
    int i; \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of floats."); \
        return PyInt_FromLong(-1); \
    } \
    self->size = PyList_Size(arg)-1; \
    self->data = (MYFLT *)realloc(self->data, (self->size+1) * sizeof(MYFLT)); \
    TableStream_setSize(self->tablestream, self->size+1); \
 \
    for (i=0; i<(self->size+1); i++) { \
        self->data[i] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(arg, i))); \
    } \
    TableStream_setData(self->tablestream, self->data); \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define SET_MATRIX_DATA \
    int i, j; \
    PyObject *innerlist; \
 \
    if (! PyList_Check(arg)) { \
        PyErr_SetString(PyExc_TypeError, "The data must be a list of list of floats."); \
        return PyInt_FromLong(-1); \
    } \
    self->height = PyList_Size(arg); \
    self->width = PyList_Size(PyList_GetItem(arg, 0)); \
    self->data = (MYFLT **)realloc(self->data, (self->height + 1) * sizeof(MYFLT)); \
    for (i=0; i<(self->height+1); i++) { \
        self->data[i] = (MYFLT *)realloc(self->data[i], (self->width + 1) * sizeof(MYFLT)); \
    } \
    MatrixStream_setWidth(self->matrixstream, self->width); \
    MatrixStream_setHeight(self->matrixstream, self->height); \
 \
    for(i=0; i<self->height; i++) { \
        innerlist = PyList_GetItem(arg, i); \
        for (j=0; j<self->width; j++) { \
            self->data[i][j] = PyFloat_AS_DOUBLE(PyNumber_Float(PyList_GET_ITEM(innerlist, j))); \
        } \
    } \
 \
    MatrixStream_setData(self->matrixstream, self->data); \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define GET_TABLE \
    int i; \
    PyObject *samples; \
 \
    samples = PyList_New(self->size); \
    for(i=0; i<self->size; i++) { \
        PyList_SetItem(samples, i, PyFloat_FromDouble(self->data[i])); \
    } \
 \
    return samples;

#define GET_VIEW_TABLE \
    int i, y; \
    int w = 500; \
    int h = 200; \
    int h2 = h/2; \
    int amp = h2 - 2; \
    float step = (float)self->size / (float)(w - 1); \
    PyObject *samples; \
 \
    samples = PyList_New(w*4); \
    for(i=0; i<w; i++) { \
        y = self->data[(int)(i*step)-1] * amp + amp + 1; \
        PyList_SetItem(samples, i*4, PyInt_FromLong(i)); \
        PyList_SetItem(samples, i*4+1, PyInt_FromLong(h-y)); \
        PyList_SetItem(samples, i*4+2, PyInt_FromLong(i)); \
        PyList_SetItem(samples, i*4+3, PyInt_FromLong(h-y)); \
    } \
 \
    return samples;

/* Normalize */
#define NORMALIZE \
	int i; \
	MYFLT mi, ma, max, ratio; \
	mi = ma = *self->data; \
	for (i=1; i<self->size; i++) { \
		if (mi > *(self->data+i)) \
			mi = *(self->data+i); \
		if (ma < *(self->data+i)) \
			ma = *(self->data+i); \
	} \
	if ((mi*mi) > (ma*ma)) \
		max = MYFABS(mi); \
	else \
		max = MYFABS(ma); \
 \
	if (max > 0.0) { \
		ratio = 0.99 / max; \
		for (i=0; i<self->size+1; i++) { \
			self->data[i] *= ratio; \
		} \
	} \
	Py_INCREF(Py_None); \
	return Py_None; \

#define NORMALIZE_MATRIX \
    int i, j; \
    MYFLT mi, ma, max, ratio; \
    mi = ma = self->data[0][0]; \
    for (i=1; i<self->height; i++) { \
        for (j=1; j<self->width; j++) { \
            if (mi > self->data[i][j]) \
                mi = self->data[i][j]; \
            if (ma < self->data[i][j]) \
                ma = self->data[i][j]; \
        } \
    } \
    if ((mi*mi) > (ma*ma)) \
        max = MYFABS(mi); \
    else \
        max = MYFABS(ma); \
 \
    if (max > 0.0) { \
        ratio = 0.99 / max; \
        for (i=0; i<self->height+1; i++) { \
            for (j=0; j<self->width+1; j++) { \
                self->data[i][j] *= ratio; \
            } \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \


#define TABLE_PUT \
    MYFLT val; \
    int pos = 0; \
    static char *kwlist[] = {"value", "pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_I, kwlist, &val, &pos)) \
        return PyInt_FromLong(-1); \
 \
    if (pos >= self->size) { \
        PyErr_SetString(PyExc_TypeError, "position outside of table boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    self->data[pos] = val; \
 \
    Py_RETURN_NONE;

#define TABLE_GET \
    int pos; \
    static char *kwlist[] = {"pos", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &pos)) \
        return PyInt_FromLong(-1); \
 \
    if (pos >= self->size) { \
        PyErr_SetString(PyExc_TypeError, "position outside of table boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[pos]);

/* Matrix macros */
#define MATRIX_BLUR \
    int i,j; \
    MYFLT tmp[self->height][self->width]; \
 \
    int lw = self->width - 1; \
    int lh = self->height - 1; \
    for (i=1; i<lw; i++) { \
        tmp[0][i] = (self->data[0][i-1] + self->data[0][i] + self->data[1][i] + self->data[0][i+1]) * 0.25; \
        tmp[lh][i] = (self->data[lh][i-1] + self->data[lh][i] + self->data[lh-1][i] + self->data[lh][i+1]) * 0.25; \
    } \
    for (i=1; i<lh; i++) { \
        tmp[i][0] = (self->data[i-1][0] + self->data[i][0] + self->data[i][1] + self->data[i+1][0]) * 0.25; \
        tmp[i][lw] = (self->data[i-1][lw] + self->data[i][lw] + self->data[i][lw-1] + self->data[i+1][lw]) * 0.25; \
    } \
 \
    for (i=1; i<lh; i++) { \
        for (j=1; j<lw; j++) { \
            tmp[i][j] = (self->data[i][j-1] + self->data[i][j] + self->data[i][j+1]) * 0.3333333; \
        } \
    } \
    for (j=1; j<lw; j++) { \
        for (i=1; i<lh; i++) { \
            self->data[i][j] = (tmp[i-1][j] + tmp[i][j] + tmp[i+1][j]) * 0.3333333; \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None;

#define MATRIX_BOOST \
    int i, j; \
    MYFLT min, max, boost, val; \
    min = -1.0; \
    max = 1.0; \
    boost = 0.01; \
    static char *kwlist[] = {"min", "max", "boost", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE__FFF, kwlist, &min, &max, &boost)) \
        return PyInt_FromLong(-1); \
 \
    float mid = (min + max) * 0.5; \
 \
    for (i=0; i<self->height; i++) { \
        for (j=0; j<self->width; j++) { \
            val = self->data[i][j]; \
            self->data[i][j] = NewMatrix_clip(val + (val-mid) * boost, min, max); \
        } \
    } \
    Py_INCREF(Py_None); \
    return Py_None; \

#define MATRIX_PUT \
    MYFLT val; \
    int x, y; \
    x = y = 0; \
    static char *kwlist[] = {"value", "x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, TYPE_F_II, kwlist, &val, &x, &y)) \
        return PyInt_FromLong(-1); \
 \
    if (x >= self->width) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (y >= self->height) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    self->data[y][x] = val; \
 \
    Py_INCREF(Py_None); \
    return Py_None; \

#define MATRIX_GET \
    int x, y; \
    static char *kwlist[] = {"x", "y", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &x, &y)) \
        return PyInt_FromLong(-1); \
 \
    if (x >= self->width) { \
        PyErr_SetString(PyExc_TypeError, "X position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    if (y >= self->height) { \
        PyErr_SetString(PyExc_TypeError, "Y position outside of matrix boundaries!."); \
        return PyInt_FromLong(-1); \
    } \
 \
    return PyFloat_FromDouble(self->data[y][x]); \


/* Init Server & Stream */
#define INIT_OBJECT_COMMON \
    self->server = PyServer_get_server(); \
    self->mul = PyFloat_FromDouble(1); \
    self->add = PyFloat_FromDouble(0); \
    self->bufsize = PyInt_AsLong(PyObject_CallMethod(self->server, "getBufferSize", NULL)); \
    self->sr = PyFloat_AsDouble(PyObject_CallMethod(self->server, "getSamplingRate", NULL)); \
    self->nchnls = PyInt_AsLong(PyObject_CallMethod(self->server, "getNchnls", NULL)); \
    self->data = (MYFLT *)realloc(self->data, (self->bufsize) * sizeof(MYFLT)); \
    MAKE_NEW_STREAM(self->stream, &StreamType, NULL); \
    Stream_setStreamObject(self->stream, (PyObject *)self); \
    Stream_setStreamId(self->stream, Stream_getNewStreamId()); \
    Stream_setBufferSize(self->stream, self->bufsize); \
    for (i=0; i<self->bufsize; i++) \
        self->data[i] = 0.0; \
    Stream_setData(self->stream, self->data);


#define SET_INTERP_POINTER \
    if (self->interp == 0) \
        self->interp = 2; \
    if (self->interp == 1) \
        self->interp_func_ptr = nointerp; \
    else if (self->interp == 2) \
        self->interp_func_ptr = linear; \
    else if (self->interp == 3) \
        self->interp_func_ptr = cosine; \
    else if (self->interp == 4) \
        self->interp_func_ptr = cubic; \

/* GETS & SETS */
#define GET_SERVER \
    if (self->server == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No server founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->server); \
    return self->server;

#define GET_STREAM \
    if (self->stream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->stream); \
    return (PyObject *)self->stream;

#define GET_TABLE_STREAM \
    if (self->tablestream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No table stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->tablestream); \
    return (PyObject *)self->tablestream; \

#define GET_MATRIX_STREAM \
    if (self->matrixstream == NULL) { \
        PyErr_SetString(PyExc_TypeError, "No matrix stream founded!"); \
        return PyInt_FromLong(-1); \
    } \
    Py_INCREF(self->matrixstream); \
    return (PyObject *)self->matrixstream; \

#define SET_MUL \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->mul); \
    if (isNumber == 1) { \
        self->mul = PyNumber_Float(tmp); \
        self->modebuffer[0] = 0; \
    } \
    else { \
        self->mul = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 1; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_ADD \
    PyObject *tmp, *streamtmp; \
\
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
\
    int isNumber = PyNumber_Check(arg); \
\
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Float(tmp); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 1; \
    } \
\
    (*self->mode_func_ptr)(self); \
\
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_SUB \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    Py_DECREF(self->add); \
    if (isNumber == 1) { \
        self->add = PyNumber_Multiply(PyNumber_Float(tmp), PyFloat_FromDouble(-1)); \
        self->modebuffer[1] = 0; \
    } \
    else { \
        self->add = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->add, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->add_stream); \
        self->add_stream = (Stream *)streamtmp; \
        self->modebuffer[1] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

#define SET_DIV \
    PyObject *tmp, *streamtmp; \
 \
    if (arg == NULL) { \
        Py_INCREF(Py_None); \
        return Py_None; \
    } \
 \
    int isNumber = PyNumber_Check(arg); \
 \
    tmp = arg; \
    Py_INCREF(tmp); \
    if (isNumber == 1) { \
        if (PyFloat_AsDouble(PyNumber_Float(tmp)) != 0.) { \
            Py_DECREF(self->mul); \
            self->mul = PyNumber_Divide(PyFloat_FromDouble(1.), PyNumber_Float(tmp)); \
            self->modebuffer[0] = 0; \
        } \
    } \
    else { \
        Py_DECREF(self->mul); \
        self->mul = tmp; \
        streamtmp = PyObject_CallMethod((PyObject *)self->mul, "_getStream", NULL); \
        Py_INCREF(streamtmp); \
        Py_XDECREF(self->mul_stream); \
        self->mul_stream = (Stream *)streamtmp; \
        self->modebuffer[0] = 2; \
    } \
 \
    (*self->mode_func_ptr)(self); \
 \
    Py_INCREF(Py_None); \
    return Py_None; 

/* Multiply, Add, inplace_multiply & inplace_add */
#define MULTIPLY \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setMul", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_MULTIPLY \
    PyObject_CallMethod((PyObject *)self, "setMul", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define ADD \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setAdd", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_ADD \
    PyObject_CallMethod((PyObject *)self, "setAdd", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define SUB \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setSub", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_SUB \
    PyObject_CallMethod((PyObject *)self, "setSub", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

#define DIV \
    Dummy *dummy; \
    MAKE_NEW_DUMMY(dummy, &DummyType, NULL); \
    Dummy_initialize(dummy); \
    PyObject_CallMethod((PyObject *)dummy, "setDiv", "O", arg); \
    Py_INCREF(self); \
    PyObject_CallMethod((PyObject *)dummy, "setInput", "O", self); \
    Py_INCREF(dummy); \
    return (PyObject *)dummy;

#define INPLACE_DIV \
    PyObject_CallMethod((PyObject *)self, "setDiv", "O", arg); \
    Py_INCREF(self); \
    return (PyObject *)self;

/* PLAY, OUT, STOP */
#define PLAY \
    float del = 0; \
    float dur = 0; \
    int nearestBuf = 0; \
    int i; \
 \
    static char *kwlist[] = {"dur", "delay", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|ff", kwlist, &dur, &del)) \
        return PyInt_FromLong(-1); \
 \
    Stream_setStreamToDac(self->stream, 0); \
    if (del == 0) { \
        Stream_setBufferCountWait(self->stream, 0); \
        Stream_setStreamActive(self->stream, 1); \
    } \
    else { \
        Stream_setStreamActive(self->stream, 0); \
        for (i=0; i<self->bufsize; i++) \
            self->data[i] = 0.0; \
        nearestBuf = (int)roundf((del * self->sr) / self->bufsize); \
        Stream_setBufferCountWait(self->stream, nearestBuf); \
    } \
    if (dur == 0) \
        Stream_setDuration(self->stream, 0); \
    else { \
        nearestBuf = (int)roundf((dur * self->sr) / self->bufsize); \
        Stream_setDuration(self->stream, nearestBuf); \
    } \
    Py_INCREF(self); \
    return (PyObject *)self;

# define OUT \
    int chnltmp = 0; \
    float del = 0; \
    float dur = 0; \
    int nearestBuf = 0; \
    int i; \
\
    static char *kwlist[] = {"chnl", "dur", "delay", NULL}; \
 \
    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iff", kwlist, &chnltmp, &dur, &del)) \
        return PyInt_FromLong(-1); \
 \
    Stream_setStreamChnl(self->stream, chnltmp % self->nchnls); \
    Stream_setStreamToDac(self->stream, 1); \
    if (del == 0) { \
        Stream_setBufferCountWait(self->stream, 0); \
        Stream_setStreamActive(self->stream, 1); \
    } \
    else { \
        Stream_setStreamActive(self->stream, 0); \
        for (i=0; i<self->bufsize; i++) \
            self->data[i] = 0.0; \
        nearestBuf = (int)roundf((del * self->sr) / self->bufsize); \
        Stream_setBufferCountWait(self->stream, nearestBuf); \
    } \
    if (dur == 0) \
        Stream_setDuration(self->stream, 0); \
    else { \
        nearestBuf = (int)roundf((dur * self->sr) / self->bufsize); \
        Stream_setDuration(self->stream, nearestBuf); \
    } \
    Py_INCREF(self); \
    return (PyObject *)self;

#define STOP \
    int i; \
    Stream_setStreamActive(self->stream, 0); \
    Stream_setStreamChnl(self->stream, 0); \
    Stream_setStreamToDac(self->stream, 0); \
    for (i=0; i<self->bufsize; i++) { \
        self->data[i] = 0; \
    } \
    Py_INCREF(Py_None); \
    return Py_None;    

/* Post processing (mul & add) macros */
#define POST_PROCESSING_II \
    MYFLT mul, add, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    add = PyFloat_AS_DOUBLE(self->add); \
    if (mul != 1 || add != 0) { \
        for (i=0; i<self->bufsize; i++) { \
            old = self->data[i]; \
            val = mul * old + add; \
            self->data[i] = val; \
        } \
    }

#define POST_PROCESSING_AI \
    MYFLT add, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IA \
    MYFLT mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old + add[i]; \
        self->data[i] = val; \
    } 

#define POST_PROCESSING_AA \
    MYFLT old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAI \
    MYFLT tmp, add, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    add = PyFloat_AS_DOUBLE(self->add); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp + add; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAA \
    MYFLT tmp, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp + add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_IREVA \
    MYFLT mul, old, val; \
    int i; \
    mul = PyFloat_AS_DOUBLE(self->mul); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul * old - add[i]; \
        self->data[i] = val; \
    } 

#define POST_PROCESSING_AREVA \
    MYFLT old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        val = mul[i] * old - add[i]; \
        self->data[i] = val; \
    }

#define POST_PROCESSING_REVAREVA \
    MYFLT tmp, old, val; \
    int i; \
    MYFLT *mul = Stream_getData((Stream *)self->mul_stream); \
    MYFLT *add = Stream_getData((Stream *)self->add_stream); \
    for (i=0; i<self->bufsize; i++) { \
        old = self->data[i]; \
        tmp = mul[i]; \
        if (tmp < 0.00001 && tmp > -0.00001) \
            tmp = 0.00001; \
        val = old / tmp - add[i]; \
        self->data[i] = val; \
    }

