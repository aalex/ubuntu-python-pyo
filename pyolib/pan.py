"""
Objects to place the sound on an arbitrary set of speakers.
 
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

class Pan(PyoObject):
    """
    Cosinus panner with control on the spread factor.

    Parent class: PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal to process.
    outs : int, optional
        Number of channels on the panning circle. Defaults to 2.
    pan : float or PyoObject
        Position of the sound on the panning circle, between 0 and 1. 
        Defaults to 0.5.
    spread : float or PyoObject
        Amount of sound leaking to the surrounding channels, 
        between 0 and 1. Defaults to 0.5.
 
    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    setPan(x) : Replace the `pan` attribute.
    setSpread(x) : Replace the `spread` attribute.

    Attributes:
    
    input : PyoObject. Input signal to process.
    pan : float or PyoObject. Position of the sound on the panning circle.
    spread : float or PyoObject. Amount of sound leaking to the 
        surrounding channels. 

    Examples:
    
    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = Noise(mul=.5)
    >>> lfo = Sine(freq=1, mul=.5, add=.5)
    >>> p = Pan(a, outs=2, pan=lfo).out()
    
    """ 
    def __init__(self, input, outs=2, pan=0.5, spread=0.5, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._pan = pan
        self._outs = outs
        self._spread = spread
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, pan, spread, mul, add, lmax = convertArgsToLists(self._in_fader, pan, spread, mul, add)
        self._base_players = [Panner_base(wrap(in_fader,i), outs, wrap(pan,i), wrap(spread,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(outs):
                self._base_objs.append(Pan_base(wrap(self._base_players,i), j, wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['input', 'pan', 'spread', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj
     
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

    def setPan(self, x):
        """
        Replace the `pan` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pan` attribute.
        
        """
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def setSpread(self, x):
        """
        Replace the `spread` attribute.
        
        Parameters:

        x : float or PyoObject
            new `spread` attribute.
        
        """
        self._spread = x
        x, lmax = convertArgsToLists(x)
        [obj.setSpread(wrap(x,i)) for i, obj in enumerate(self._base_players)]
                     
    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapPan(self._pan),
                        SLMap(0., 1., 'lin', 'spread', self._spread),
                        SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal to process."""
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
 
    @property
    def pan(self):
        """float or PyoObject. Position of the sound on the panning circle."""
        return self._pan
    @pan.setter
    def pan(self, x): self.setPan(x) 

    @property
    def spread(self):
        """float or PyoObject. Amount of sound leaking to the surrounding channels.""" 
        return self._spread
    @spread.setter
    def spread(self, x): self.setSpread(x)

class SPan(PyoObject):
    """
    Simple equal power panner.
    
    Parent class: PyoObject
     
    Parameters:
    
    input : PyoObject
        Input signal to process.
    outs : int, optional
        Number of channels on the panning circle. Defaults to 2.
    pan : float or PyoObject
        Position of the sound on the panning circle, between 0 and 1. 
        Defaults to 0.5.

    Methods:
    
    setInput(x, fadetime) : Replace the `input` attribute.
    setPan(x) : Replace the `pan` attribute.

    Attributes:
    
    input : PyoObject. Input signal to process.
    pan : float or PyoObject. Position of the sound on the panning circle.

    Examples:
    
    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = Noise(mul=.5)
    >>> lfo = Sine(freq=1, mul=.5, add=.5)
    >>> p = SPan(a, outs=2, pan=lfo).out()

    """
    def __init__(self, input, outs=2, pan=0.5, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._outs = outs
        self._pan = pan
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, pan, mul, add, lmax = convertArgsToLists(self._in_fader, pan, mul, add)
        self._base_players = [SPanner_base(wrap(in_fader,i), outs, wrap(pan,i)) for i in range(lmax)]
        self._base_objs = []
        for i in range(lmax):
            for j in range(outs):
                self._base_objs.append(SPan_base(wrap(self._base_players,i), j, wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['input', 'pan', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj
     
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

    def setPan(self, x):
        """
        Replace the `pan` attribute.
        
        Parameters:

        x : float or PyoObject
            new `pan` attribute.
        
        """
        self._pan = x
        x, lmax = convertArgsToLists(x)
        [obj.setPan(wrap(x,i)) for i, obj in enumerate(self._base_players)]
                     
    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:    
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:   
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self
    
    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapPan(self._pan), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self): 
        """PyoObject. Input signal to process."""
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def pan(self): 
        """float or PyoObject. Position of the sound on the panning circle."""
        return self._pan
    @pan.setter
    def pan(self, x): self.setPan(x)

class Switch(PyoObject):
    """
    Audio switcher.

    Switch takes an audio input and interpolates between multiple outputs.
    
    User can retrieve the different streams by calling the output number
    between brackets. obj[0] retrieve the first stream, obj[outs-1] the
    last one.
    
    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal to process.
    outs : int, optional
        Number of outputs. Defaults to 2.
    voice : float or PyoObject
        Voice position pointer, between 0 and outs-1. 
        Defaults to 0.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setVoice(x) : Replace the `voice` attribute.

    Attributes:

    input : PyoObject. Input signal to process.
    voice : float or PyoObject. Voice position pointer.

    Examples:

    >>> s = Server(nchnls=2).boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> lf = Sine(freq=.25, mul=1.5, add=1.5)
    >>> b = Switch(a, outs=3, voice=lf)
    >>> c = WGVerb(b[0], feedback=.8).out()
    >>> d = Disto(b[1], mul=.1).out()
    >>> e = Delay(b[2], delay=.2, feedback=.6).out()

    """
    def __init__(self, input, outs=2, voice=0., mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._outs = outs
        self._voice = voice
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, voice, mul, add, lmax = convertArgsToLists(self._in_fader, voice, mul, add)
        self._base_players = [Switcher_base(wrap(in_fader,i), outs, wrap(voice,i)) for i in range(lmax)]
        self._base_objs = []
        for j in range(outs):
            for i in range(lmax):
                self._base_objs.append(Switch_base(wrap(self._base_players,i), j, wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['input', 'voice', 'mul', 'add']

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

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

    def setVoice(self, x):
        """
        Replace the `voice` attribute.

        Parameters:

        x : float or PyoObject
            new `voice` attribute.

        """
        self._voice = x
        x, lmax = convertArgsToLists(x)
        [obj.setVoice(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, self._outs-1, "lin", "voice", self._voice), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self): 
        """PyoObject. Input signal to process."""
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)

    @property
    def voice(self): 
        """float or PyoObject. Voice position pointer."""
        return self._voice
    @voice.setter
    def voice(self, x): self.setVoice(x)

class Selector(PyoObject):
    """
    Audio selector.
    
    Selector takes multiple PyoObjects in input and interpolates between 
    them to generate a single output.
        
    Parent class: PyoObject

    Parameters:

    inputs : list of PyoObject
        Audio objects to interpolate from.
    voice : float or PyoObject, optional
        Voice position pointer, between 0 and len(inputs)-1. 
        Defaults to 0.
    
    Methods:

    setInputs(x) : Replace the `inputs` attribute.
    setVoice(x) : Replace the `voice` attribute.

    Attributes:
    
    inputs : list of PyoObject. Audio objects to interpolate from.
    voice : float or PyoObject. Voice position pointer.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH + "/transparent.aif", loop=True)
    >>> b = Noise(mul=.1)
    >>> c = SfPlayer(SNDS_PATH + "/accord.aif", loop=True)
    >>> lf = Sine(freq=.1, add=1)
    >>> d = Selector(inputs=[a,b,c], voice=lf).out()
    
    """
    def __init__(self, inputs, voice=0., mul=1, add=0):
        PyoObject.__init__(self)
        self._inputs = inputs
        self._voice = voice
        self._mul = mul
        self._add = add
        voice, mul, add, self._lmax = convertArgsToLists(voice, mul, add)
        self._length = 1
        for obj in self._inputs:
            try:
                if len(obj) > self._length: self._length = len(obj)
            except:
                pass    
        self._base_objs = []        
        for i in range(self._lmax):
            for j in range(self._length):
                choice = []
                for obj in self._inputs:
                    try:
                        choice.append(obj[j%len(obj)])
                    except:
                        choice.append(obj)            
                self._base_objs.append(Selector_base(choice, wrap(voice,i), wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['inputs', 'voice', 'mul', 'add']

    def setInputs(self, x):
        """
        Replace the `inputs` attribute.
        
        Parameters:

        x : list of PyoObject
            new `inputs` attribute.
        
        """
        self._inputs = x
        for i in range(self._lmax):           
            for j in range(self._length):
                choice = []
                for obj in self._inputs:
                    try:
                        choice.append(obj[j%len(obj)])
                    except:
                        choice.append(obj) 
                self._base_objs[i+j*self._lmax].setInputs(choice)

    def setVoice(self, x):
        """
        Replace the `voice` attribute.
        
        Parameters:

        x : float or PyoObject
            new `voice` attribute.
        
        """
        self._voice = x
        x, lmax = convertArgsToLists(x)
        for i, obj in enumerate(self._base_objs):
            obj.setVoice(wrap(x, i/self._length))

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0, len(self._inputs)-1, "lin", "voice", self._voice), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def inputs(self): return self._inputs
    @inputs.setter
    def inputs(self, x): self.setInputs(x)
    @property
    def voice(self): return self._voice
    @voice.setter
    def voice(self, x): self.setVoice(x)

class Mixer(PyoObject):
    """
    Audio mixer.

    Mixer mixes multiple inputs to an arbitrary number of outputs 
    with independant amplitude values per mixing channel and a 
    user defined portamento applied on amplitude changes.

    Parent class: PyoObject

    Parameters:

    outs : int, optional
        Number of outputs of the mixer. Available at initialization
        time only. Defaults to 2.
    chnls : int, optional
        Number of channels per output. Available at initialization
        time only. Defaults to 1.
    time : float, optional
        Duration, in seconds, of a portamento applied on
        a new amplitude value for a mixing channel.
        Defaults to 0.025.

    Methods:

    setTime(x) : Sets the portamento duration in seconds.
    addInput(voice, input) : Adds a new input to the mixer.
    delInput(voice) : Removes an input from the mixer.
    setAmp(vin, vout, amp) : Sets the amplitude of a mixing channel.

    Attributes:

    time : float. Portamento applied on amplitude changes.

    Notes:

    User can retrieve each of the output channels by calling the Mixer
    object with the desired channel between square brackets (see example).

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = SfPlayer(SNDS_PATH+"/transparent.aif", loop=True, mul=.2)
    >>> b = FM(carrier=200, ratio=[.5013,.4998], index=6, mul=.2)
    >>> mm = Mixer(outs=3, chnls=2, time=.025)
    >>> fx1 = Disto(mm[0], drive=.9, slope=.9, mul=.2).out()
    >>> fx2 = Freeverb(mm[1], size=.8, damp=.8).out()
    >>> fx3 = Harmonizer(mm[2], transpo=1, feedback=.75).out()
    >>> mm.addInput(0, a)
    >>> mm.addInput(1, b)
    >>> mm.setAmp(0,0,.5)
    >>> mm.setAmp(0,1,.5)
    >>> mm.setAmp(1,2,.5)
    >>> mm.setAmp(1,1,.5)

    """
    def __init__(self, outs=2, chnls=1, time=0.025, mul=1, add=0):
        PyoObject.__init__(self)
        self._outs = outs
        self._chnls = chnls
        self._time = time
        self._mul = mul
        self._add = add
        self._inputs = {}
        time, mul, add, lmax = convertArgsToLists(time, mul, add)
        self._base_players = [Mixer_base(outs, wrap(time,i)) for i in range(chnls)]
        self._base_objs = [MixerVoice_base(self._base_players[j], i, wrap(mul,i), wrap(add,i)) for i in range(outs) for j in range(chnls)]
        
    def __dir__(self):
        return ["time", "mul", "add"]

    def __del__(self):
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        for obj in self._base_players:
            obj.deleteStream()
            del obj

    def __getitem__(self, x):
        if type(x) == SliceType:
            return [self._base_objs[j*self._chnls+i] for j in range(x.start or 0, x.stop or sys.maxint, x.step or 1) for i in range(self._chnls)]
        elif x < len(self._base_objs):
            return [self._base_objs[x*self._chnls+i] for i in range(self._chnls)]
        else:
            print "'x' too large!"         

    def play(self, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_players = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_players)]
        if type(chnl) == ListType:
            self._base_objs = [obj.out(wrap(chnl,i), wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        else:
            if chnl < 0:
                self._base_objs = [obj.out(i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(random.sample(self._base_objs, len(self._base_objs)))]
            else:
                self._base_objs = [obj.out(chnl+i*inc, wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def stop(self):
        [obj.stop() for obj in self._base_players]
        [obj.stop() for obj in self._base_objs]
        return self

    def setTime(self, x):
        """
        Sets the portamento duration in seconds.

        Parameters:

        x : float
            New portamento duration.

        """
        self._time = x
        x, lmax = convertArgsToLists(x)
        [obj.setTime(wrap(x,i)) for i, obj in enumerate(self._base_players)]

    def addInput(self, voice, input):
        """
        Adds an audio object in the mixer's inputs.

        Parameters:

        voice : int or string
            Key in the mixer dictionary for this input.
        input : PyoObject
            Audio object to add to the mixer.

        """
        self._inputs[voice] = input
        [obj.addInput(str(voice), wrap(input,i)) for i, obj in enumerate(self._base_players)]

    def delInput(self, voice):
        """
        Removes an audio object from the mixer's inputs.

        Parameters:

        voice : int or string
            Key in the mixer dictionary assigned to the input to remove.

        """
        if self._inputs.has_key(voice):
            del self._inputs[voice]
            [obj.delInput(str(voice)) for i, obj in enumerate(self._base_players)]

    def setAmp(self, vin, vout, amp):
        """
        Sets the amplitude of a mixing channel.

        Parameters:

        vin : int or string
            Key in the mixer dictionary of the desired input.
        vout : int
            Ouput channel where to send the signal.
        amp : float
            Amplitude value for this mixing channel.

        """
        if self._inputs.has_key(vin) and vout < self._outs:
            [obj.setAmp(str(vin), vout, amp) for i, obj in enumerate(self._base_players)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def time(self):
        """float. Portamento.""" 
        return self._time
    @time.setter
    def time(self, x): self.setTime(x)
