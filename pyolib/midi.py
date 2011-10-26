"""
Objects to retrieve Midi informations for a specific Midi port.

Objects creates and returns audio streams from the value in their 
Midi input.

The audio streams of these objects are essentially intended to be
used as controls and can't be sent to the output soundcard.

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

######################################################################
### MIDI
######################################################################                                       
class Midictl(PyoObject):
    """
    Get the current value of a Midi channel controller.
    
    Get the current value of a controller and optionally map it 
    inside a specified range.

    Parent class: PyoObject
    
    Parameters:
    
    ctlnumber : int
        Midi channel. Available at initialization time only.
    minscale : float, optional
        Low range value for mapping. Available at initialization 
        time only.
    maxscale : float, optional
        High range value for mapping. Available at initialization 
        time only.

    Notes:

    The out() method is bypassed. Midictl's signal can not be sent 
    to audio outs.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> m = Midictl(ctlnumber=[107,102], minscale=250, maxscale=1000)
    >>> p = Port(m, .02)
    >>> a = Sine(freq=p, mul=.3).out()
    >>> a1 = Sine(freq=p*1.25, mul=.3).out()
    >>> a2 = Sine(freq=p*1.5, mul=.3).out()
        
    """
    def __init__(self, ctlnumber, minscale=0, maxscale=1, mul=1, add=0):
        PyoObject.__init__(self)
        self._mul = mul
        self._add = add
        ctlnumber, minscale, maxscale, mul, add, lmax = convertArgsToLists(ctlnumber, minscale, maxscale, mul, add)
        self._base_objs = [Midictl_base(wrap(ctlnumber,i), wrap(minscale,i), wrap(maxscale,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class Notein(PyoObject):
    """
    Generates Midi note messages.
    
    From a Midi device, takes the notes in the range defined with 
    `first` and `last` parameters, and outputs up to `poly` 
    noteon - noteoff streams in the `scale` format (Midi, hertz 
    or transpo).
    
    Parent class: PyoObject

    Parameters:
    
    poly : int, optional
        Number of streams of polyphony generated. Defaults to 10.
    scale : int, optional
        Pitch output format. 0 = Midi, 1 = Hertz, 2 = transpo. 
        In the transpo mode, the central key (the key where there 
        is no transposition) is (`first` + `last`) / 2.
    first : int, optional
        Lowest Midi value. Defaults to 0.
    last : int, optional
        Highest Midi value. Defaults to 127.

    Methods:

    get(identifier, all) : Return the first sample of the current 
        buffer as a float.

    Notes:
    
    Pitch and velocity are two separated set of streams. 
    The user should call :
    
    Notein['pitch'] to retrieve pitch streams.
    Notein['velocity'] to retrieve velocity streams.    

    Velocity is automatically scaled between 0 and 1.
    
    The out() method is bypassed. Notein's signal can not be sent 
    to audio outs.
    
    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> notes = Notein(poly=10, scale=1, mul=.5)
    >>> p = Port(notes['velocity'], .001, .5)
    >>> b = Sine(freq=notes['pitch'], mul=p).out()
    >>> c = Sine(freq=notes['pitch'] * 0.997, mul=p).out()
    >>> d = Sine(freq=notes['pitch'] * 1.005, mul=p).out()
    
    """
    def __init__(self, poly=10, scale=0, first=0, last=127, mul=1, add=0):
        PyoObject.__init__(self)
        self._pitch_dummy = []
        self._velocity_dummy = []
        self._poly = poly
        self._scale = scale
        self._first = first
        self._last = last
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_handler = MidiNote_base(self._poly, self._scale, self._first, self._last)
        self._base_objs = []
        for i in range(lmax * poly):
            self._base_objs.append(Notein_base(self._base_handler, i, 0, 1, 0))
            self._base_objs.append(Notein_base(self._base_handler, i, 1, wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['mul', 'add']

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
        self._base_handler.deleteStream()
        del self._base_handler

    def __getitem__(self, str):
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
        self._base_handler.play()
        dur, delay, lmax = convertArgsToLists(dur, delay)
        self._base_objs = [obj.play(wrap(dur,i), wrap(delay,i)) for i, obj in enumerate(self._base_objs)]
        return self

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self
    
    def stop(self):
        self._base_handler.stop()
        [obj.stop() for obj in self._base_objs]
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class MidiAdsr(PyoObject):
    """
    Midi triggered ADSR envelope generator.

    Calculates the classical ADSR envelope using linear segments. 
    The envelope starts when it receives a positive value in input,
    this value is used as the peak amplitude of the envelope. The
    `sustain` parameter is a fraction of the peak value and sets
    the real sustain value. A 0 in input (note off) starts the
    release part of the envelope.

    Parent class: PyoObject

    Parameters:

    input : PyoObject
        Input signal used to trigger the envelope. A positive value
        sets the peak amplitude and starts the envelope. A 0 starts
        the release part of the envelope.
    attack : float, optional
        Duration of the attack phase in seconds. Defaults to 0.01.
    decay : float, optional
        Duration of the decay phase in seconds. Defaults to 0.05.
    sustain : float, optional
        Amplitude of the sustain phase, as a fraction of the peak
        amplitude at the start of the envelope. Defaults to 0.7.
    release : float, optional
        Duration of the release phase in seconds. Defaults to 0.1.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.
    setAttack(x) : Replace the `attack` attribute.
    setDecay(x) : Replace the `decay` attribute.
    setSustain(x) : Replace the `sustain` attribute.
    setRelease(x) : Replace the `release` attribute.

    Attributes:

    input : PyoObject. Input signal used to trigger the envelope.
    attack : float. Duration of the attack phase in seconds.
    decay : float. Duration of the decay in seconds.
    sustain : float. Amplitude of the sustain phase.
    release : float. Duration of the release in seconds.

    Notes:

    The out() method is bypassed. MidiAdsr's signal can not be sent to audio outs.

    Shape of a classical Adsr:

          -
         -  -
        -     -
       -        ------------------------
      -                                  -
     -                                     -
    -                                        -
      att - dec -        sustain       - rel

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> mid = Notein(scale=1)
    >>> env = MidiAdsr(mid['velocity'], attack=.005, decay=.1, sustain=.4, release=1)
    >>> a = SineLoop(freq=mid['pitch'], feedback=.1, mul=env).out()
    >>> b = SineLoop(freq=mid['pitch']*1.005, feedback=.1, mul=env).out(1)

    """
    def __init__(self, input, attack=0.01, decay=0.05, sustain=0.7, release=0.1, mul=1, add=0):
        PyoObject.__init__(self)
        self._input = input
        self._attack = attack
        self._decay = decay
        self._sustain = sustain
        self._release = release
        self._mul = mul
        self._add = add
        self._in_fader = InputFader(input)
        in_fader, attack, decay, sustain, release, mul, add, lmax = convertArgsToLists(self._in_fader, attack, decay, sustain, release, mul, add)
        self._base_objs = [MidiAdsr_base(wrap(in_fader,i), wrap(attack,i), wrap(decay,i), wrap(sustain,i), wrap(release,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input', 'attack', 'decay', 'sustain', 'release', 'mul', 'add']

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setInput(self, x, fadetime=0.05):
        """
        Replace the `input` attribute.

        Parameters:

        x : PyoObject
            New signal used to trigger the envelope.
        fadetime : float, optional
            Crossfade time between old and new input. Defaults to 0.05.

        """
        self._input = x
        self._in_fader.setInput(x, fadetime)

    def setAttack(self, x):
        """
        Replace the `attack` attribute.

        Parameters:

        x : float
            new `attack` attribute.

        """
        self._attack = x
        x, lmax = convertArgsToLists(x)
        [obj.setAttack(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setDecay(self, x):
        """
        Replace the `decay` attribute.

        Parameters:

        x : float
            new `decay` attribute.

        """
        self._decay = x
        x, lmax = convertArgsToLists(x)
        [obj.setDecay(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSustain(self, x):
        """
        Replace the `sustain` attribute.

        Parameters:

        x : float
            new `sustain` attribute.

        """
        self._sustain = x
        x, lmax = convertArgsToLists(x)
        [obj.setSustain(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setRelease(self, x):
        """
        Replace the `sustain` attribute.

        Parameters:

        x : float
            new `sustain` attribute.

        """
        self._release = x
        x, lmax = convertArgsToLists(x)
        [obj.setRelease(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def attack(self):
        """float. Duration of the attack phase in seconds.""" 
        return self._attack
    @attack.setter
    def attack(self, x): self.setAttack(x)

    @property
    def decay(self):
        """float. Duration of the decay phase in seconds.""" 
        return self._decay
    @decay.setter
    def decay(self, x): self.setDecay(x)

    @property
    def sustain(self):
        """float. Amplitude of the sustain phase, as fraction of the peak amplitude.""" 
        return self._sustain
    @sustain.setter
    def sustain(self, x): self.setSustain(x)

    @property
    def release(self):
        """float. Duration of the release phase in seconds.""" 
        return self._release
    @release.setter
    def release(self, x): self.setRelease(x)
