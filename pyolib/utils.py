"""
Miscellaneous objects.

"""

"""
Copyright 2010 Olivier Belanger

This file is part of pyo, a python module to help digital signal
processing script creation.

pyo is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pyo is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pyo.  If not, see <http://www.gnu.org/licenses/>.
"""
from _core import *
from _maps import *
from types import SliceType
import threading, time

class Clean_objects(threading.Thread):
    """
    Stops and deletes PyoObjects after a given time.
    
    Parameters:
    
    time : float
        Time, in seconds, to wait before calling stop on the given 
        objects and deleting them.
    *args : PyoObject(s)
        Objects to delete.
        
    Methods:
    
    start() : Starts the thread. The timer begins on this call.    

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(mul=.5).out()
    >>> b = Fader(fadein=.1, fadeout=1, dur=5).play()
    >>> c = Biquad(a, freq=1000, q=2, mul=b).out()
    >>> dump = Clean_objects(time=6, a, b, c)
    >>> dump.start()
    
    """
    def __init__(self, time, *args):
        threading.Thread.__init__(self)
        self.t = time
        self.args = args
        
    def run(self):
        time.sleep(self.t)
        for arg in self.args:
            try: arg.stop()
            except: pass
        for arg in self.args:
            del arg 

class Print(PyoObject):
    """
    Print PyoObject's current value.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal to filter.
    method : int {0, 1}, optional
        There is two methods to set when a value is printed (Defaults to 0):
        0 : at a periodic interval.
        1 : everytime the value changed.
    interval : float, optional
        Interval, in seconds, between each print. Used by method 0.
        Defaults to 0.25.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setMethod(x) : Replace the `method` attribute.
    setInterval(x) : Replace the `interval` attribute.

    Attributes:

    input : PyoObject. Input signal.
    method : int. Controls when a value is printed.
    interval : float. For method 0, interval, in seconds, between each print.

    Notes:

    The out() method is bypassed. Print's signal can not be sent to 
    audio outs.
    
    Print has no `mul` and `add` attributes.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + '/transparent.aif', loop=True, mul=.5).out()
    >>> b = Follower(a)
    >>> p = Print(b, method=0, interval=.1)

    """
    def __init__(self, input, method=0, interval=0.25):
        PyoObject.__init__(self)
        self._input = input
        self._method = method
        self._interval = interval
        self._in_fader = InputFader(input)
        in_fader, method, interval, lmax = convertArgsToLists(self._in_fader, method, interval)
        self._base_objs = [Print_base(wrap(in_fader,i), wrap(method,i), wrap(interval,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'method', 'interval']

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setMethod(self, x):
        """
        Replace the `method` attribute.
        
        Parameters:

        x : int {0, 1}
            New `method` attribute.

        """
        self._method = x
        x, lmax = convertArgsToLists(x)
        [obj.setMethod(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInterval(self, x):
        """
        Replace the `interval` attribute.

        Parameters:

        x : float
            New `interval` attribute.

        """
        self._interval = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterval(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def method(self):
        """int. Controls when a value is printed.""" 
        return self._method
    @method.setter
    def method(self, x): self.setMethod(x)

    @property
    def interval(self):
        """float. For method 0, interval, in seconds, between each print.""" 
        return self._interval
    @interval.setter
    def interval(self, x): self.setInterval(x)

class Snap(PyoObject):
    """
    Snap input values on a user's defined midi scale.
    
    Snap takes an audio input of floating-point values from 0
    to 127 and output the nearest value in the `choice` parameter. 
    `choice` must only defined the first octave (0 <= x < 12) 
    and the object will take care of the input octave range. 
    According to `scale` parameter, output can be in midi notes, 
    hertz or transposition factor (centralkey = 60).
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Incoming Midi notes as an audio stream.
    choice : list of floats
        Possible values, as midi notes, for output.
    scale : int {0, 1, 2}, optional
        Pitch output format. 0 = MIDI, 1 = Hertz, 2 = transpo. 
        In the transpo mode, the central key (the key where there 
        is no transposition) is 60. Defaults to 0.
 
    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setChoice(x) : Replace the `choice` attribute.
    setScale(x) : Replace the `scale` attribute.

    Attributes:
    
    input : PyoObject. Audio signal to transform.
    choice : list of floats. Possible values.
    scale : int. Output format.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> wav = SquareTable()
    >>> env = CosTable([(0,0), (100,1), (500,.3), (8191,0)])
    >>> met = Metro(.125, 8).play()
    >>> amp = TrigEnv(met, table=env, mul=.1)
    >>> pit = TrigXnoiseMidi(met, dist=4, x1=20, mrange=(48,84))
    >>> hertz = Snap(pit, choice=[0,2,3,5,7,8,10], scale=1)
    >>> a = Osc(table=wav, freq=hertz, phase=0, mul=amp).out()

    """
    def __init__(self, input, choice, scale=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._choice = choice
        self._scale = scale
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, scale, mul, add, lmax = convertArgsToLists(self._in_fader, scale, mul, add)
        self._base_objs = [Snap_base(wrap(in_fader,i), choice, wrap(scale,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'choice', 'scale', 'mul', 'add']

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)
        
    def setChoice(self, x):
        """
        Replace the `choice` attribute.
        
        Parameters:

        x : list of floats
            new `choice` attribute.
        
        """
        self._choice = x
        [obj.setChoice(x) for i, obj in enumerate(self._base_objs)]

    def setScale(self, x):
        """
        Replace the `scale` attribute.
        
        Possible values are: 
            0 -> Midi notes
            1 -> Hertz
            2 -> transposition factor 
                 (centralkey is (`minrange` + `maxrange`) / 2

        Parameters:

        x : int {0, 1, 2}
            new `scale` attribute.

        """
        self._scale = x
        x, lmax = convertArgsToLists(x)
        [obj.setScale(wrap(x,i)) for i, obj in enumerate(self._base_objs)]


    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self): return self._input
    @input.setter
    def input(self, x): self.setInput(x)
    @property
    def choice(self): return self._choice
    @choice.setter
    def choice(self, x): self.setChoice(x)
    @property
    def scale(self): return self._scale
    @scale.setter
    def scale(self, x): self.setScale(x)

class Interp(PyoObject):
    """
    Interpolates between two signals.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        First input signal.
    input2 : PyoObject
        Second input signal.
    interp : float or PyoObject, optional
        Averaging value. 0 means only first signal, 1 means only second
        signal. Default to 0.5.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setInput2(x, fadetime) : Replace the `input2` attribute.
    setInterp(x) : Replace the `interp` attribute.

    Attributes:

    input : PyoObject. First input signal.
    input2 : PyoObject. Second input signal.
    interp : float or PyoObject. Averaging value.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sf = SfPlayer(SNDS_PATH + '/accord.aif', speed=1, loop=True, mul=.5)
    >>> sf2 = SfPlayer(SNDS_PATH + '/transparent.aif', speed=1, loop=True, mul=.5)
    >>> lfo = Osc(table=SquareTable(20), freq=5, mul=.5, add=.5)
    >>> a = Interp(sf, sf2, lfo).out()

    """
    def __init__(self, input, input2, interp=0.5, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._input2 = input2
        self._interp = interp
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(input2)
        in_fader, in_fader2, interp, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, interp, mul, add)
        self._base_objs = [Interp_base(wrap(in_fader,i), wrap(in_fader2,i), wrap(interp,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'input2', 'interp', 'mul', 'add']
        
    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setInput2(self, x, fadetime=0.05):
        """
        Replace the `input2` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input2 = x
        self._in_fader2.setInput(x, fadetime)
        
    def setInterp(self, x):
        """
        Replace the `interp` attribute.
        
        Parameters:

        x : float or PyoObject
            New `interp` attribute.

        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., "lin", "interp", self._interp), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. First input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def input2(self):
        """PyoObject. Second input signal.""" 
        return self._input2
    @input2.setter
    def input2(self, x): self.setInput2(x)

    @property
    def interp(self):
        """float or PyoObject. Averaging value.""" 
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

class SampHold(PyoObject):
    """
    Performs a sample-and-hold operation on its input. 
 
    SampHold performs a sample-and-hold operation on its input according 
    to the value of `controlsig`. If `controlsig` equals `value`, the input 
    is sampled and holded until next sampling.
    
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal.
    controlsig : PyoObject
        Controls when to sample the signal.
    value : float or PyoObject, optional
        Sampling targeted value. Default to 0.0.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setControlsig(x, fadetime) : Replace the `controlsig` attribute.
    setValue(x) : Replace the `value` attribute.

    Attributes:

    input : PyoObject. Input signal.
    controlsig : PyoObject. Controls when to sample the signal.
    value : float or PyoObject. Targeted value.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise(500,1000)
    >>> b = Sine(4)
    >>> c = SampHold(input=a, controlsig=b, value=0)
    >>> d = Sine(c, mul=.3).out()

    """
    def __init__(self, input, controlsig, value=0.0, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._controlsig = controlsig
        self._value = value
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        self._in_fader2 = InputFader(controlsig)
        in_fader, in_fader2, value, mul, add, lmax = convertArgsToLists(self._in_fader, self._in_fader2, value, mul, add)
        self._base_objs = [SampHold_base(wrap(in_fader,i), wrap(in_fader2,i), wrap(value,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'controlsig', 'value', 'mul', 'add']
        
    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setControlsig(self, x, fadetime=0.05):
        """
        Replace the `controlsig` attribute.
        
        Parameters:

        x : PyoObject
            New control signal.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._controlsig = x
        self._in_fader2.setInput(x, fadetime)
        
    def setValue(self, x):
        """
        Replace the `value` attribute.
        
        Parameters:

        x : float or PyoObject
            New `value` attribute.

        """
        self._value = x
        x, lmax = convertArgsToLists(x)
        [obj.setValue(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def controlsig(self):
        """PyoObject. Control signal.""" 
        return self._controlsig
    @controlsig.setter
    def controlsig(self, x): self.setControlsig(x)

    @property
    def value(self):
        """float or PyoObject. Targeted value.""" 
        return self._value
    @value.setter
    def value(self, x): self.setValue(x)

class Compare(PyoObject):
    """
    Comparison object.
    
    Compare evaluates a comparison between a PyoObject and a number or
    between two PyoObjects and outputs 1.0, as audio stream, if the
    comparison is true, otherwise outputs 0.0.
 
    Parent class: PyoObject
   
    Parameters:
    
    input : PyoObject
        Input signal.
    comp : float or PyoObject
        comparison signal.
    mode : string, optional
        Comparison operator as a string. Allowed operator are "<", "<=",
        ">", ">=", "==", "!=". Default to "<".

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setComp(x, fadetime) : Replace the `comp` attribute.
    setMode(x) : Replace the `mode` attribute.

    Attributes:

    input : PyoObject. Input signal.
    comp : float or PyoObject. Comparison signal.
    mode : string. Comparison operator.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SineLoop(freq=200, feedback=.1)
    >>> b = SineLoop(freq=150, feedback=.1)
    >>> ph = Phasor(freq=1)
    >>> ch = Compare(input=ph, comp=0.5, mode="<=")
    >>> out = Selector(inputs=[a,b], voice=Port(ch), mul=.5).out()
    
    """
    def __init__(self, input, comp, mode="<", mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._comp = comp
        self._mode = mode
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        self.comp_dict = {"<": 0, "<=": 1, ">": 2, ">=": 3, "==": 4, "!=": 5}
        in_fader, comp, mode, mul, add, lmax = convertArgsToLists(self._in_fader, comp, mode, mul, add)
        self._base_objs = [Compare_base(wrap(in_fader,i), wrap(comp,i), self.comp_dict[wrap(mode,i)], wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'comp', 'mode', 'mul', 'add']

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.
        
        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setComp(self, x):
        """
        Replace the `comp` attribute.
        
        Parameters:

        x : PyoObject
            New comparison signal.

        """
        self._comp = x
        x, lmax = convertArgsToLists(x)
        [obj.setComp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setMode(self, x):
        """
        Replace the `mode` attribute. 
        
        Allowed operator are "<", "<=", ">", ">=", "==", "!=".
        
        Parameters:

        x : string
            New `mode` attribute.

        """
        self._mode = x
        x, lmax = convertArgsToLists(x)
        [obj.setMode(self.comp_dict[wrap(x,i)]) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
      
    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def comp(self):
        """PyoObject. Comparison signal.""" 
        return self._comp
    @comp.setter
    def comp(self, x): self.setComp(x)

    @property
    def mode(self):
        """string. Comparison operator.""" 
        return self._mode
    @mode.setter
    def mode(self, x): self.setMode(x)

class Record(PyoObject):
    """
    Writes input sound in an audio file on the disk.

    `input` parameter must be a valid PyoObject or an addition of 
    PyoObjects, parameters can't be in list format.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to record.
    filename : string
        Full path of the file to create.
    chnls : int, optional
        Number of channels in the audio file. Defaults to 2.
    fileformat : int, optional
        Format type of the audio file. Record will first try to
        set the format from the filename extension. If it's not possible,
        it uses the fileformat parameter. Defaults to 0. 
        Supported formats are:
            0 : WAV - Microsoft WAV format (little endian) {.wav, .wave}
            1 : AIFF - Apple/SGI AIFF format (big endian) {.aif, .aiff}
    sampletype : int, optional
        Bit depth encoding of the audio file. Defaults to 0.
        Supported types are:
            0 : 16 bits int
            1 : 24 bits int
            2 : 32 bits int
            3 : 32 bits float
            4 : 64 bits float
    buffering : int, optional
        Number of bufferSize to wait before writing samples to disk.
        High buffering uses more memory but improves performance.
        Defaults to 4.
        
    Notes:
    
    All parameters can only be set at intialization time.    

    The `stop` method must be called on the object to close the file 
    properly.
    
    The out() method is bypassed. Record's signal can not be sent to 
    audio outs.
    
    Record has no `mul` and `add` attributes.
        
    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> from random import uniform
    >>> import os
    >>> t = HarmTable([1, 0, 0, .2, 0, 0, 0, .1, 0, 0, .05])
    >>> amp = Fader(fadein=.05, fadeout=2, dur=4, mul=.05).play()
    >>> osc = Osc(t, freq=[uniform(350,360) for i in range(10)], mul=amp).out()
    >>> home = os.path.expanduser('~')
    >>> rec = Record(osc, filename=home+"/example_synth.aif", fileformat=1, sampletype=1)
    >>> clean = Clean_objects(4.5, rec)
    >>> clean.start()

    """
    def __init__(self, input, filename, chnls=2, fileformat=0, sampletype=0, buffering=4):
        PyoObject.__init__(self)
        self._input = input
        FORMATS = {'wav': 0, 'wave': 0, 'aif': 1, 'aiff': 1}
        ext = filename.rsplit('.')
        if len(ext) >= 2:
            ext = ext[-1].lower()
            if FORMATS.has_key(ext):
                fileformat = FORMATS[ext]
            else:
                print 'Warning: Unknown file extension. Using fileformat value.'
        else:
            print 'Warning: Filename has no extension. Using fileformat value.'
        self._base_objs = [Record_base(self._input.getBaseObjects(), filename, chnls, fileformat, sampletype, buffering)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def __dir__(self):
        return []

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class Denorm(PyoObject):
    """
    Mixes low level noise to an input signal.

    Mixes low level (~1e-24 for floats, and ~1e-60 for doubles) noise to a an input signal. 
    Can be used before IIR filters and reverbs to avoid denormalized numbers which may 
    otherwise result in significantly increased CPU usage. 

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Attributes:

    input : PyoObject. Input signal to process.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> amp = Linseg([(0,0),(2,1),(4,0)]).play()
    >>> a = Sine(freq=1000, mul=0.01*amp)
    >>> den = Denorm(a)
    >>> rev = Freeverb(den, size=.9).out()

    """
    def __init__(self, input, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, mul, add, lmax = convertArgsToLists(self._in_fader, mul, add)
        self._base_objs = [Denorm_base(wrap(in_fader,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'mul', 'add']

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Parameters:

        x : PyoObject
            New signal to process.
        fadetime : float, optional
            Crossfade time between old and new input. Default to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to filter.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

class ControlRec(PyoObject):
    """
    Records control values and writes them in a text file.

    `input` parameter must be a valid PyoObject managing any number
    of streams, other parameters can't be in list format. The user
    must call the `write` method to create text files on the disk.

    Each line in the text files contains two values, the absolute time
    in seconds and the sampled value.

    The play() method starts the recording and is not called at the 
    object creation time.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to sample.
    filename : string
        Full path (without extension) used to create the files. 
        "_000" will be added to file's names with increasing digits
        according to the number of streams in input. The same 
        filename can be passed to a ControlRead object to read all
        related files.
    rate : int, optional
        Rate at which the input values are sampled. Defaults to 1000.
    dur : float, optional
        Duration of the recording, in seconds. If 0.0, the recording
        won't stop until the end of the performance. If greater than
        0.0, the `stop` method is automatically called at the end of
        the recording.
        
    Methods:
    
    write() : Writes values in a text file on the disk.

    Notes:

    All parameters can only be set at intialization time.    

    The `write` method must be called on the object to write the files 
    on the disk.

    The out() method is bypassed. ControlRec's signal can not be sent to 
    audio outs.

    ControlRec has no `mul` and `add` attributes.

    See also: ControlRead

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> rnds = Randi(freq=[1,2])
    >>> home = os.path.expanduser('~')
    >>> rec = ControlRec(rnds, home+"/test", rate=100, dur=4).play()
    >>> # call rec.write() to save "test_000" and "test_001" in the home directory.

    """
    def __init__(self, input, filename, rate=1000, dur=0.0):
        PyoObject.__init__(self)
        self._input = input
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._rate = rate
        self._dur = dur
        self._in_fader = InputFader(input)
        in_fader, lmax = convertArgsToLists(self._in_fader)
        self._base_objs = [ControlRec_base(wrap(in_fader,i), rate, dur) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def __dir__(self):
        return []

    def write(self):
        """
        Writes recorded values in text files on the disk.
        
        """
        for i, obj in enumerate(self._base_objs):
            f = open(os.path.join(self._path, "%s_%03d" % (self._name, i)), "w")
            [f.write("%f %f\n" % p) for p in obj.getData()]
            f.close()

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class ControlRead(PyoObject):
    """
    Reads control values previously stored in text files.

    Read sampled sound from a table, with optional looping mode.

    Parent class: PyoObject

    Parameters:

    filename : string
        Full path (without extension) used to create the files. Usually
        the same filename as the one given to a ControlRec object to 
        record automation. The directory will be scaned and all files
        named "filename_xxx" will add a new stream in the object.
    rate : int, optional
        Rate at which the values are sampled. Defaults to 1000.
    loop : boolean, optional
        Looping mode, False means off, True means on. 
        Defaults to False.
    interp : int, optional
        Choice of the interpolation method. Defaults to 2.
            1 : no interpolation
            2 : linear
            3 : cosinus
            4 : cubic

    Methods:

    setRate(x) : Replace the `rate` attribute.
    setLoop(x) : Replace the `loop` attribute.
    setInterp(x) : Replace the `interp` attribute.

    Attributes:

    rate : int, Sampling frequency in cycles per second.
    loop : boolean, Looping mode.
    interp : int {1, 2, 3, 4}, Interpolation method.

    Notes:

    ControlRead will sends a trigger signal at the end of the playback if 
    loop is off or any time it wraps around if loop is on. User can 
    retreive the trigger streams by calling obj['trig']:

    >>> rnds = ControlRead(home+"/freq_auto", loop=True)
    >>> t = SndTable(SNDS_PATH+"/transparent.aif")
    >>> loop = TrigEnv(rnds["trig"], t, dur=[.2,.3,.4,.5], mul=.5).out()

    The out() method is bypassed. ControlRead's signal can not be sent to 
    audio outs.

    See also: ControlRec

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> home = os.path.expanduser('~')
    >>> # assuming "test_xxx" exists in the user directory
    >>> rnds = ControlRead(home+"/test", rate=100, loop=True)
    >>> sines = SineLoop(freq=rnds, feedback=.05, mul=.2).out()

    """
    def __init__(self, filename, rate=1000, loop=False, interp=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._rate = rate
        self._loop = loop
        self._interp = interp
        self._mul = mul
        self._add = add
        files = sorted([f for f in os.listdir(self._path) if self._name+"_" in f])
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = []
        for i in range(len(files)):
            path = os.path.join(self._path, files[i])
            f = open(path, "r")
            values = [float(l.split()[1]) for l in f.readlines()]
            f.close()
            self._base_objs.append(ControlRead_base(values, rate, loop, interp, wrap(mul,i), wrap(add,i)))
        self._trig_objs = [ControlReadTrig_base(obj) for obj in self._base_objs]

    def __dir__(self):
        return ['rate', 'loop', 'interp', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._trig_objs:
            obj.deleteStream()
            del obj

    def __getitem__(self, i):
        if i == 'trig':
            return self._trig_objs

        if type(i) == SliceType:
            return self._base_objs[i]
        if i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        self._trig_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._trig_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def stop(self):
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self

    def setRate(self, x):
        """
        Replace the `rate` attribute.

        Parameters:

        x : int
            new `rate` attribute.

        """
        self._rate = x
        x, lmax = convertArgsToLists(x)
        [obj.setRate(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setLoop(self, x):
        """
        Replace the `loop` attribute.

        Parameters:

        x : boolean
            new `loop` attribute.

        """
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInterp(self, x):
        """
        Replace the `interp` attribute.

        Parameters:

        x : int {1, 2, 3, 4}
            new `interp` attribute.

        """
        self._interp = x
        x, lmax = convertArgsToLists(x)
        [obj.setInterp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def rate(self):
        """int. Sampling frequency in cycles per second.""" 
        return self._rate
    @rate.setter
    def rate(self, x): self.setRate(x)

    @property
    def loop(self): 
        """boolean. Looping mode.""" 
        return self._loop
    @loop.setter
    def loop(self, x): self.setLoop(x)

    @property
    def interp(self): 
        """int {1, 2, 3, 4}. Interpolation method."""
        return self._interp
    @interp.setter
    def interp(self, x): self.setInterp(x)

class NoteinRec(PyoObject):
    """
    Records Notein inputs and writes them in a text file.

    `input` parameter must be a Notein object managing any number
    of streams, other parameters can't be in list format. The user
    must call the `write` method to create text files on the disk.

    Each line in the text files contains three values, the absolute time
    in seconds, the Midi pitch and the normalized velocity.

    The play() method starts the recording and is not called at the 
    object creation time.

    Parent class: PyoObject

    Parameters:

    input : Notein
        Notein signal to sample.
    filename : string
        Full path (without extension) used to create the files. 
        "_000" will be added to file's names with increasing digits
        according to the number of streams in input. The same 
        filename can be passed to a NoteinRead object to read all
        related files.

    Methods:

    write() : Writes values in a text file on the disk.

    Notes:

    All parameters can only be set at intialization time.    

    The `write` method must be called on the object to write the files 
    on the disk.

    The out() method is bypassed. NoteinRec's signal can not be sent to 
    audio outs.

    NoteinRec has no `mul` and `add` attributes.

    See also: NoteinRead

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=2)
    >>> home = os.path.expanduser('~')
    >>> rec = NoteinRec(notes, home+"/test").play()
    >>> # call rec.write() to save "test_000" and "test_001" in the home directory.

    """
    def __init__(self, input, filename):
        PyoObject.__init__(self)
        self._input = input
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._in_pitch = self._input["pitch"]
        self.in_velocity = self._input["velocity"]
        in_pitch, in_velocity, lmax = convertArgsToLists(self._in_pitch, self.in_velocity)
        self._base_objs = [NoteinRec_base(wrap(in_pitch,i), wrap(in_velocity,i)) for i in range(lmax)]

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def __dir__(self):
        return []

    def write(self):
        """
        Writes recorded values in text files on the disk.

        """
        for i, obj in enumerate(self._base_objs):
            f = open(os.path.join(self._path, "%s_%03d" % (self._name, i)), "w")
            [f.write("%f %f %f\n" % p) for p in obj.getData()]
            f.close()

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
    
class NoteinRead(PyoObject):
    """
    Reads Notein values previously stored in text files.

    Parent class: PyoObject

    Parameters:

    filename : string
        Full path (without extension) used to create the files. Usually
        the same filename as the one given to a NoteinRec object to 
        record automation. The directory will be scaned and all files
        named "filename_xxx" will add a new stream in the object.
    loop : boolean, optional
        Looping mode, False means off, True means on. 
        Defaults to False.

    Methods:

    setLoop(x) : Replace the `loop` attribute.
    get(identifier, all) : Return the first sample of the current buffer as a float.

    Attributes:

    loop : boolean, Looping mode.

    Notes:

    NoteinRead will sends a trigger signal at the end of the playback if 
    loop is off or any time it wraps around if loop is on. User can 
    retreive the trigger streams by calling obj['trig']:

    >>> notes = NoteinRead(home+"/notes_rec", loop=True)
    >>> t = SndTable(SNDS_PATH+"/transparent.aif")
    >>> loop = TrigEnv(notes["trig"], t, dur=[.2,.3,.4,.5], mul=.25).out()

    The out() method is bypassed. NoteinRead's signal can not be sent to 
    audio outs.

    See also: NoteinRec

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> home = os.path.expanduser('~')
    >>> # assuming "test_xxx" exists in the user directory
    >>> notes = NoteinRead(home+"/test", loop=True)
    >>> amps = Port(notes['velocity'], 0.001, 0.5, mul=.2)
    >>> sines = SineLoop(freq=notes['pitch'], feedback=.05, mul=amps).out()

    """
    def __init__(self, filename, loop=False, mul=1, add=0):
        PyoObject.__init__(self)
        self._pitch_dummy = []
        self._velocity_dummy = []
        self._filename = filename
        self._path, self._name = os.path.split(filename)
        self._loop = loop
        self._mul = mul
        self._add = add
        files = sorted([f for f in os.listdir(self._path) if self._name+"_" in f])
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = []
        self._trig_objs = []
        self._poly = len(files)
        for i in range(self._poly):
            path = os.path.join(self._path, files[i])
            f = open(path, "r")
            vals = [l.split() for l in f.readlines()]
            timestamps = [float(v[0]) for v in vals]
            pitches = [float(v[1]) for v in vals]
            amps = [float(v[2]) for v in vals]
            f.close()
            self._base_objs.append(NoteinRead_base(pitches, timestamps, loop))
            self._base_objs.append(NoteinRead_base(amps, timestamps, loop, wrap(mul,i), wrap(add,i)))
            self._trig_objs.append(NoteinReadTrig_base(self._base_objs[-1]))

    def __dir__(self):
        return ['loop', 'mul', 'add']

    def __del__(self):
        if self._pitch_dummy:
            [obj.deleteStream() for obj in self._pitch_dummy]
        if self._velocity_dummy:
            [obj.deleteStream() for obj in self._velocity_dummy]
        self._pitch_dummy = []
        self._velocity_dummy = []
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._trig_objs:
            obj.deleteStream()
            del obj

    def __getitem__(self, str):
        if str == 'trig':
            return self._trig_objs
        if str == 'pitch':
            self._pitch_dummy.append(Dummy([self._base_objs[i*2] for i in range(self._poly)]))
            return self._pitch_dummy[-1]
        if str == 'velocity':
            self._velocity_dummy.append(Dummy([self._base_objs[i*2+1] for i in range(self._poly)]))
            return self._velocity_dummy[-1]

    def get(self, identifier="pitch", all=False):
        """
        Return the first sample of the current buffer as a float.
        
        Can be used to convert audio stream to usable Python data.
        
        "pitch" or "velocity" must be given to `identifier` to specify
        which stream to get value from.
        
        Parameters:

            identifier : string {"pitch", "velocity"}
                Address string parameter identifying audio stream.
                Defaults to "pitch".
            all : boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.
                 
        """
        if not all:
            return self.__getitem__(identifier)[0]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self.__getitem__(identifier).getBaseObjects()]
 
    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        self._trig_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._trig_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def stop(self):
        [obj.stop() for obj in self._base_objs]
        [obj.stop() for obj in self._trig_objs]
        return self

    def setLoop(self, x):
        """
        Replace the `loop` attribute.

        Parameters:

        x : boolean
            new `loop` attribute.

        """
        self._loop = x
        x, lmax = convertArgsToLists(x)
        [obj.setLoop(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def loop(self): 
        """boolean. Looping mode.""" 
        return self._loop
    @loop.setter
    def loop(self, x): self.setLoop(x)

