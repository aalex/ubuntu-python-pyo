"""
Synthesis generators.

Classical synthesis generators that can be used as sources of a signal
processing chain.

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
### Sources
######################################################################                                       
class Sine(PyoObject):
    """
    A simple sine wave oscillator.
    
    Parent class: PyoObject
    
    Parameters:
    
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1000.
    phase : float or PyoObject, optional
        Phase of sampling, expressed as a fraction of a cycle (0 to 1). 
        Defaults to 0.
        
    Methods:
    
    setFreq(x) : Replace the `freq` attribute.
    setPhase(x) : Replace the `phase` attribute.
    
    Attributes:
    
    freq : float or PyoObject, Frequency in cycles per second.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    
    See also: Osc, Phasor
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> sine = Sine(freq=500).out()
    
    """
    def __init__(self, freq=1000, phase=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._freq = freq
        self._phase = phase
        self._mul = mul
        self._add = add
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Sine_base(wrap(freq,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['freq', 'phase', 'mul', 'add']
        
    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            new `freq` attribute.
        
        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setPhase(self, x):
        """
        Replace the `phase` attribute.
        
        Parameters:

        x : float or PyoObject
            new `phase` attribute.
        
        """
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapPhase(self._phase), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
        
    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling.""" 
        return self._phase
    @phase.setter
    def phase(self, x): self.setPhase(x)

class SineLoop(PyoObject):
    """
    A simple sine wave oscillator with feedback.
    
    The oscillator output, multiplied by `feedback`, is added to the position
    increment and can be used to control the brightness of the oscillator.

    Parent class: PyoObject

    Parameters:

    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 1000.
    feedback : float or PyoObject, optional
        Amount of the output signal added to position increment, between 0 and 1. 
        Controls the brightness. Defaults to 0.

    Methods:

    setFreq(x) : Replace the `freq` attribute.
    setFeedback(x) : Replace the `feedback` attribute.

    Attributes:

    freq : float or PyoObject, Frequency in cycles per second.
    feedback : float or PyoObject, Brightness control.

    See also: Sine, OscLoop

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(.1, 0, .1, .1)
    >>> a = SineLoop(freq=400, feedback=lfo).out()

    """
    def __init__(self, freq=1000, feedback=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._freq = freq
        self._feedback = feedback
        self._mul = mul
        self._add = add
        freq, feedback, mul, add, lmax = convertArgsToLists(freq, feedback, mul, add)
        self._base_objs = [SineLoop_base(wrap(freq,i), wrap(feedback,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['freq', 'feedback', 'mul', 'add']

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        Parameters:

        x : float or PyoObject
            new `freq` attribute.

        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setFeedback(self, x):
        """
        Replace the `feedback` attribute.

        Parameters:

        x : float or PyoObject
            new `feedback` attribute.

        """
        self._feedback = x
        x, lmax = convertArgsToLists(x)
        [obj.setFeedback(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(0, 1, "lin", "feedback", self._feedback), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def feedback(self):
        """float or PyoObject. Brightness control.""" 
        return self._feedback
    @feedback.setter
    def feedback(self, x): self.setFeedback(x)

class Phasor(PyoObject):
    """
    A simple phase incrementor. 
    
    Output is a periodic ramp from 0 to 1.
 
    Parent class: PyoObject
   
    Parameters:
    
    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 100.
    phase : float or PyoObject, optional
        Phase of sampling, expressed as a fraction of a cycle (0 to 1). 
        Defaults to 0.
        
    Methods:
    
    setFreq(x) : Replace the `freq` attribute.
    setPhase(x) : Replace the `phase` attribute.
 
    Attributes:
    
    freq : float or PyoObject, Frequency in cycles per second.
    phase : float or PyoObject, Phase of sampling (0 -> 1).
    
    See also: Osc, Sine
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> f = Phasor(freq=1, mul=1000, add=500)
    >>> sine = Sine(freq=f).out()   
    
    """
    def __init__(self, freq=100, phase=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._freq = freq
        self._phase = phase
        self._mul = mul
        self._add = add
        freq, phase, mul, add, lmax = convertArgsToLists(freq, phase, mul, add)
        self._base_objs = [Phasor_base(wrap(freq,i), wrap(phase,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['freq', 'phase', 'mul', 'add']

    def setFreq(self, x):
        """
        Replace the `freq` attribute.
        
        Parameters:

        x : float or PyoObject
            new `freq` attribute.
        
        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setPhase(self, x):
        """
        Replace the `phase` attribute.
        
        Parameters:

        x : float or PyoObject
            new `phase` attribute.
        
        """
        self._phase = x
        x, lmax = convertArgsToLists(x)
        [obj.setPhase(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMapPhase(self._phase), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
        
    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def phase(self):
        """float or PyoObject. Phase of sampling.""" 
        return self._phase
    @phase.setter
    def phase(self, x): self.setPhase(x)

class Input(PyoObject):
    """
    Read from a numbered channel in an external audio signal.

    Parent class: PyoObject

    Parameters:
    
    chnl : int, optional
        Input channel to read from. Defaults to 0.

    Notes:
    
    Requires that the Server's duplex mode is set to 1. 
    
    Examples:
    
    >>> s = Server(duplex=1).boot()
    >>> s.start()
    >>> a = Input(chnl=0)
    >>> b = Delay(a, delay=.25, feedback=.5, mul=.5).out()   
    
    """
    def __init__(self, chnl=0, mul=1, add=0):                
        PyoObject.__init__(self)
        self._chnl = chnl
        self._mul = mul
        self._add = add
        chnl, mul, add, lmax = convertArgsToLists(chnl, mul, add)
        self._base_objs = [Input_base(wrap(chnl,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class Noise(PyoObject):
    """
    A white noise generator.
        
    Parent class: PyoObject
    
    Methods:
    
    setType(x) : Sets the generation algorithm.

    Attributes:
    
    type : int {0, 1}, Generation algorithm.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = Noise()
    >>> b = Biquad(a, freq=1000, q=5, type=0).out()    
        
    """
    def __init__(self, mul=1, add=0):                
        PyoObject.__init__(self)
        self._type = 0
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [Noise_base(wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def setType(self, x):
        """
        Sets the generation algorithm.

        Parameters:

        x : int, {0, 1}
            0 uses the system rand() method to generate number. Used as default.
            1 uses a simple linear congruential generator, cheaper than rand().

        """
        self._type = x
        x, lmax = convertArgsToLists(x)
        [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def type(self):
        """int {0, 1}. Sets the generation algorithm.""" 
        return self._type
    @type.setter
    def type(self, x): self.setType(x)

class PinkNoise(PyoObject):
    """
    A pink noise generator.

    Paul Kellet's implementation of pink noise generator.

    This is an approximation to a -10dB/decade filter using a weighted sum
    of first order filters. It is accurate to within +/-0.05dB above 9.2Hz
    (44100Hz sampling rate).
    
    Parent class: PyoObject

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = PinkNoise()
    >>> b = Biquad(a, freq=1000, q=5, type=0).out()    

    """
    def __init__(self, mul=1, add=0):                
        PyoObject.__init__(self)
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [PinkNoise_base(wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class BrownNoise(PyoObject):
    """
    A brown noise generator.

    The spectrum of a brown noise has a power density which decreases 6 dB 
    per octave with increasing frequency (density proportional to 1/f^2).
    
    Parent class: PyoObject

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = BrownNoise(.3).out()

    """
    def __init__(self, mul=1, add=0):                
        PyoObject.__init__(self)
        self._mul = mul
        self._add = add
        mul, add, lmax = convertArgsToLists(mul, add)
        self._base_objs = [BrownNoise_base(wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class FM(PyoObject):
    """
    A simple frequency modulation generator.
    
    Implements frequency modulation synthesis based on Chowning's algorithm.
    
    Parent class: PyoObject
    
    Parameters:
    
    carrier : float or PyoObject, optional
        Carrier frequency in cycles per second. Defaults to 100.
    ratio : float or PyoObject, optional
        A factor that, when multiplied by the `carrier` parameter, 
        gives the modulator frequency. Defaults to 0.5.
    index : float or PyoObject, optional
        The modulation index. This value multiplied by the modulator
        frequency gives the modulator amplitude. Defaults to 5.
        
    Methods:
    
    setCarrier(x) : Replace the `carrier` attribute.
    setRatio(x) : Replace the `ratio` attribute.
    setIndex(x) : Replace the `index` attribute.
    
    Attributes:
    
    carrier : float or PyoObject, Carrier frequency in cycles per second.
    ratio : float or PyoObject, Modulator/Carrier ratio.
    index : float or PyoObject, Modulation index.
    
    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> ind = LinTable([(0,20), (200,5), (1000,2), (8191,1)])
    >>> m = Metro(4).play()
    >>> tr = TrigEnv(m, table=ind, dur=4)
    >>> f = FM(carrier=[250.5,250], ratio=.2499, index=tr, mul=.5).out()
    
    """
    def __init__(self, carrier=100, ratio=0.5, index=5, mul=1, add=0):
        PyoObject.__init__(self)
        self._carrier = carrier
        self._ratio = ratio
        self._index = index
        self._mul = mul
        self._add = add
        carrier, ratio, index, mul, add, lmax = convertArgsToLists(carrier, ratio, index, mul, add)
        self._base_objs = [Fm_base(wrap(carrier,i), wrap(ratio,i), wrap(index,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['carrier', 'ratio', 'index', 'mul', 'add']
        
    def setCarrier(self, x):
        """
        Replace the `carrier` attribute.
        
        Parameters:

        x : float or PyoObject
            new `carrier` attribute.
        
        """
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x,i)) for i, obj in enumerate(self._base_objs)]
        
    def setRatio(self, x):
        """
        Replace the `ratio` attribute.
        
        Parameters:

        x : float or PyoObject
            new `ratio` attribute.
        
        """
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setIndex(self, x):
        """
        Replace the `index` attribute.
        
        Parameters:

        x : float or PyoObject
            new `index` attribute.
        
        """
        self._index = x
        x, lmax = convertArgsToLists(x)
        [obj.setIndex(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(10, 500, "lin", "carrier", self._carrier),
                          SLMap(.01, 10, "lin", "ratio", self._ratio),
                          SLMap(0, 20, "lin", "index", self._index),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)
        
    @property
    def carrier(self):
        """float or PyoObject. Carrier frequency in cycles per second.""" 
        return self._carrier
    @carrier.setter
    def carrier(self, x): self.setCarrier(x)

    @property
    def ratio(self):
        """float or PyoObject. Modulator/Carrier ratio.""" 
        return self._ratio
    @ratio.setter
    def ratio(self, x): self.setRatio(x)

    @property
    def index(self):
        """float or PyoObject. Modulation index.""" 
        return self._index
    @index.setter
    def index(self, x): self.setIndex(x)

class CrossFM(PyoObject):
    """
    A cross frequency modulation generator.

    Frequency modulation synthesis where the output of both oscillators
    modulates the frequency of the other one.

    Parent class: PyoObject

    Parameters:

    carrier : float or PyoObject, optional
        Carrier frequency in cycles per second. Defaults to 100.
    ratio : float or PyoObject, optional
        A factor that, when multiplied by the `carrier` parameter, 
        gives the modulator frequency. Defaults to 0.5.
    ind1 : float or PyoObject, optional
        The carrier index. This value multiplied by the carrier
        frequency gives the carrier amplitude for modulating the
        modulation oscillator frequency. 
        Defaults to 2.
    ind1 : float or PyoObject, optional
        The modulation index. This value multiplied by the modulation
        frequency gives the modulation amplitude for modulating the 
        carrier oscillator frequency. 
        Defaults to 2.

    Methods:

    setCarrier(x) : Replace the `carrier` attribute.
    setRatio(x) : Replace the `ratio` attribute.
    setInd1(x) : Replace the `ind1` attribute.
    setInd2(x) : Replace the `ind2` attribute.

    Attributes:

    carrier : float or PyoObject, Carrier frequency in cycles per second.
    ratio : float or PyoObject, Modulator/Carrier ratio.
    ind1 : float or PyoObject, Carrier index.
    ind2 : float or PyoObject, Modulation index.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> ind = LinTable([(0,20), (200,5), (1000,2), (8191,1)])
    >>> m = Metro(4).play()
    >>> tr = TrigEnv(m, table=ind, dur=4)
    >>> f = CrossFM(carrier=[250.5,250], ratio=[.2499,.2502], ind1=tr, ind2=tr, mul=.3).out()

    """
    def __init__(self, carrier=100, ratio=0.5, ind1=2, ind2=2, mul=1, add=0):
        PyoObject.__init__(self)
        self._carrier = carrier
        self._ratio = ratio
        self._ind1 = ind1
        self._ind2 = ind2
        self._mul = mul
        self._add = add
        carrier, ratio, ind1, ind2, mul, add, lmax = convertArgsToLists(carrier, ratio, ind1, ind2, mul, add)
        self._base_objs = [CrossFm_base(wrap(carrier,i), wrap(ratio,i), wrap(ind1,i), wrap(ind2,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['carrier', 'ratio', 'ind1', 'ind2', 'mul', 'add']

    def setCarrier(self, x):
        """
        Replace the `carrier` attribute.

        Parameters:

        x : float or PyoObject
            new `carrier` attribute.

        """
        self._carrier = x
        x, lmax = convertArgsToLists(x)
        [obj.setCarrier(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setRatio(self, x):
        """
        Replace the `ratio` attribute.

        Parameters:

        x : float or PyoObject
            new `ratio` attribute.

        """
        self._ratio = x
        x, lmax = convertArgsToLists(x)
        [obj.setRatio(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInd1(self, x):
        """
        Replace the `ind1` attribute.

        Parameters:

        x : float or PyoObject
            new `ind1` attribute.

        """
        self._ind1 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInd1(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setInd2(self, x):
        """
        Replace the `ind2` attribute.

        Parameters:

        x : float or PyoObject
            new `ind2` attribute.

        """
        self._ind2 = x
        x, lmax = convertArgsToLists(x)
        [obj.setInd2(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(10, 500, "lin", "carrier", self._carrier),
                          SLMap(.01, 10, "lin", "ratio", self._ratio),
                          SLMap(0, 20, "lin", "ind1", self._ind1),
                          SLMap(0, 20, "lin", "ind2", self._ind2),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def carrier(self):
        """float or PyoObject. Carrier frequency in cycles per second.""" 
        return self._carrier
    @carrier.setter
    def carrier(self, x): self.setCarrier(x)

    @property
    def ratio(self):
        """float or PyoObject. Modulator/Carrier ratio.""" 
        return self._ratio
    @ratio.setter
    def ratio(self, x): self.setRatio(x)

    @property
    def ind1(self):
        """float or PyoObject. Carrier index.""" 
        return self._ind1
    @ind1.setter
    def ind1(self, x): self.setInd1(x)

    @property
    def ind2(self):
        """float or PyoObject. Modulation index.""" 
        return self._ind2
    @ind2.setter
    def ind2(self, x): self.setInd2(x)

class Blit(PyoObject):
    """
    Band limited impulse train synthesis.

    Impulse train generator with control over the number of harmonics 
    in the spectrum, which gives oscillators with very low aliasing.

    Parent class: PyoObject

    Parameters:

    freq : float or PyoObject, optional
        Frequency in cycles per second. Defaults to 100.
    harms : float or PyoObject, optional
        Number of harmonics in the generated spectrum. Defaults to 40.

    Methods:

    setFreq(x) : Replace the `freq` attribute.
    setHarms(x) : Replace the `harms` attribute.

    Attributes:

    freq : float or PyoObject, Frequency in cycles per second.
    harms : float or PyoObject, Number of harmonics.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> lfo = Sine(freq=4, mul=.02, add=1)
    >>> a = Blit(freq=[100, 99]*lfo, harms=45, mul=.3)
    >>> lp = Tone(a, 1000).out()

    """
    def __init__(self, freq=100, harms=40, mul=1, add=0):
        PyoObject.__init__(self)
        self._freq = freq
        self._harms = harms
        self._mul = mul
        self._add = add
        freq, harms, mul, add, lmax = convertArgsToLists(freq, harms, mul, add)
        self._base_objs = [Blit_base(wrap(freq,i), wrap(harms,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['freq', 'harms', 'mul', 'add']

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        Parameters:

        x : float or PyoObject
            new `freq` attribute.

        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setHarms(self, x):
        """
        Replace the `harms` attribute.

        Parameters:

        x : float or PyoObject
            new `harms` attribute.

        """
        self._harms = x
        x, lmax = convertArgsToLists(x)
        [obj.setHarms(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(1, 5000, "log", "freq", self._freq),
                          SLMap(2, 100, "lin", "harms", self._harms),
                          SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def harms(self):
        """float or PyoObject. Number of harmonics.""" 
        return self._harms
    @harms.setter
    def harms(self, x): self.setHarms(x)

class Rossler(PyoObject):
    """
    Chaotic attractor for the Rossler system.

    The Rossler attractor is a system of three non-linear ordinary differential 
    equations. These differential equations define a continuous-time dynamical 
    system that exhibits chaotic dynamics associated with the fractal properties 
    of the attractor.
    
    Parent class: PyoObject

    Parameters:

    pitch : float or PyoObject, optional
        Controls the speed, in the range 0 -> 1, of the variations. With values 
        below 0.2, this object can be used as a low frequency oscillator (LFO) 
        and above 0.2, it will generate a broad spectrum noise with harmonic peaks. 
        Defaults to 0.25.
    chaos : float or PyoObject, optional
        Controls the chaotic behavior, in the range 0 -> 1, of the oscillator. 
        0 means nearly periodic while 1 is totally chaotic. Defaults to 0.5.
    stereo, boolean, optional
        If True, 2 streams will be generated, one with the X variable signal of 
        the algorithm and a second composed of the Y variable signal of the algorithm.
        These two signal are strongly related in their frequency spectrum but 
        the Y signal is out-of-phase by approximatly 180 degrees. Useful to create
        alternating LFOs. Available at initialization only. Defaults to False.

    Methods:

    setPitch(x) : Replace the `pitch` attribute.
    setChaos(x) : Replace the `chaos` attribute.

    Attributes:

    pitch : float or PyoObject, Speed of the variations {0. -> 1.}.
    chaos : float or PyoObject, Chaotic behavior {0. -> 1.}.

    See also: Lorenz

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Rossler(pitch=.003, stereo=True, mul=.15, add=.15)
    >>> b = Rossler(pitch=[.4,.38], mul=a).out()

    """
    def __init__(self, pitch=0.25, chaos=0.5, stereo=False, mul=1, add=0):
        PyoObject.__init__(self)
        self._pitch = pitch
        self._chaos = chaos
        self._stereo = stereo
        self._mul = mul
        self._add = add
        pitch, chaos, mul, add, lmax = convertArgsToLists(pitch, chaos, mul, add)
        self._base_objs = []
        self._alt_objs = []
        for i in range(lmax):
            self._base_objs.append(Rossler_base(wrap(pitch,i), wrap(chaos,i), wrap(mul,i), wrap(add,i)))
            if self._stereo:
                self._base_objs.append(RosslerAlt_base(self._base_objs[-1], wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['pitch', 'chaos', 'mul', 'add']

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        Parameters:

        x : float or PyoObject
            new `pitch` attribute. {0. -> 1.}

        """
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x,i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        """
        Replace the `chaos` attribute.

        Parameters:

        x : float or PyoObject
            new `chaos` attribute. {0. -> 1.}

        """
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x,i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., "lin", "pitch", self._pitch), 
                          SLMap(0., 1., "lin", "chaos", self._chaos), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def pitch(self):
        """float or PyoObject. Speed of the variations.""" 
        return self._pitch
    @pitch.setter
    def pitch(self, x): self.setPitch(x)

    @property
    def chaos(self):
        """float or PyoObject. Chaotic behavior.""" 
        return self._chaos
    @chaos.setter
    def chaos(self, x): self.setChaos(x)

class Lorenz(PyoObject):
    """
    Chaotic attractor for the Lorenz system.

    The Lorenz attractor is a system of three non-linear ordinary differential 
    equations. These differential equations define a continuous-time dynamical 
    system that exhibits chaotic dynamics associated with the fractal properties 
    of the attractor.

    Parent class: PyoObject

    Parameters:

    pitch : float or PyoObject, optional
        Controls the speed, in the range 0 -> 1, of the variations. With values 
        below 0.2, this object can be used as a low frequency oscillator (LFO) 
        and above 0.2, it will generate a broad spectrum noise with harmonic peaks. 
        Defaults to 0.25.
    chaos : float or PyoObject, optional
        Controls the chaotic behavior, in the range 0 -> 1, of the oscillator. 
        0 means nearly periodic while 1 is totally chaotic. Defaults to 0.5
    stereo, boolean, optional
        If True, 2 streams will be generated, one with the X variable signal of 
        the algorithm and a second composed of the Y variable signal of the algorithm.
        These two signal are strongly related in their frequency spectrum but 
        the Y signal is out-of-phase by approximatly 180 degrees. Useful to create
        alternating LFOs. Available at initialization only. Defaults to False.

    Methods:

    setPitch(x) : Replace the `pitch` attribute.
    setChaos(x) : Replace the `chaos` attribute.

    Attributes:

    pitch : float or PyoObject, Speed of the variations {0. -> 1.}.
    chaos : float or PyoObject, Chaotic behavior {0. -> 1.}.

    See also: Rossler

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Lorenz(pitch=.003, stereo=True, mul=.2, add=.2)
    >>> b = Lorenz(pitch=[.4,.38], mul=a).out()

    """
    def __init__(self, pitch=0.25, chaos=0.5, stereo=False, mul=1, add=0):
        PyoObject.__init__(self)
        self._pitch = pitch
        self._chaos = chaos
        self._stereo = stereo
        self._mul = mul
        self._add = add
        pitch, chaos, mul, add, lmax = convertArgsToLists(pitch, chaos, mul, add)
        self._base_objs = []
        self._alt_objs = []
        for i in range(lmax):
            self._base_objs.append(Lorenz_base(wrap(pitch,i), wrap(chaos,i), wrap(mul,i), wrap(add,i)))
            if self._stereo:
                self._base_objs.append(LorenzAlt_base(self._base_objs[-1], wrap(mul,i), wrap(add,i)))

    def __dir__(self):
        return ['pitch', 'chaos', 'mul', 'add']

    def setPitch(self, x):
        """
        Replace the `pitch` attribute.

        Parameters:

        x : float or PyoObject
            new `pitch` attribute. {0. -> 1.}

        """
        self._pitch = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setPitch(wrap(x,i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setPitch(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setChaos(self, x):
        """
        Replace the `chaos` attribute.

        Parameters:

        x : float or PyoObject
            new `chaos` attribute. {0. -> 1.}

        """
        self._chaos = x
        x, lmax = convertArgsToLists(x)
        if self._stereo:
            [obj.setChaos(wrap(x,i)) for i, obj in enumerate(self._base_objs) if (i % 2) == 0]
        else:
            [obj.setChaos(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMap(0., 1., "lin", "pitch", self._pitch), 
                          SLMap(0., 1., "lin", "chaos", self._chaos), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def pitch(self):
        """float or PyoObject. Speed of the variations.""" 
        return self._pitch
    @pitch.setter
    def pitch(self, x): self.setPitch(x)

    @property
    def chaos(self):
        """float or PyoObject. Chaotic behavior.""" 
        return self._chaos
    @chaos.setter
    def chaos(self, x): self.setChaos(x)

class LFO(PyoObject):
    """
    Band-limited Low Frequency Oscillator with different wave shapes.

    Parent class : PyoObject

    Parameters:

    freq : float or PyoObject, optional
        Oscillator frequency in cycles per second. Defaults to 100.
    sharp : float or PyoObject, optional
        Sharpness factor between 0 and 1. Sharper waveform results
        in more harmonics in the spectrum. Defaults to 0.5.
    type : int, optional
        Waveform type. eight possible values :
            0 = Saw up (default)
            1 = Saw down
            2 = Square
            3 = Triangle
            4 = Pulse
            5 = Bipolar pulse
            6 = Sample and hold
            7 = Modulated Sine

    Methods:

    setFreq(x) : Replace the `freq` attribute.
    setSharp(x) : Replace the `sharp` attribute.
    setType(x) : Replace the `type` attribute.

    Attributes:

    freq : float or PyoObject. Oscillator frequency in cycles per second.
    sharp : float or PyoObject. Sharpness factor between 0 and 1.
    type : int. Waveform type.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> lf = Sine([.31,.34], mul=15, add=20)
    >>> lf2 = LFO([.43,.41], sharp=.7, type=2, mul=.4, add=.4)
    >>> a = LFO(freq=lf, sharp=lf2, type=7, mul=100, add=300)
    >>> b = SineLoop(freq=a, feedback = 0.12, mul=.3).out()

    """
    def __init__(self, freq=100, sharp=0.5, type=0, mul=1, add=0):
        PyoObject.__init__(self)
        self._freq = freq
        self._sharp = sharp
        self._type = type
        self._mul = mul
        self._add = add
        freq, sharp, type, mul, add, lmax = convertArgsToLists(freq, sharp, type, mul, add)
        self._base_objs = [LFO_base(wrap(freq,i), wrap(sharp,i), wrap(type,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['freq', 'sharp', 'type', 'mul', 'add']

    def setFreq(self, x):
        """
        Replace the `freq` attribute.

        Parameters:

        x : float or PyoObject
            New `freq` attribute, in cycles per seconds.

        """
        self._freq = x
        x, lmax = convertArgsToLists(x)
        [obj.setFreq(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setSharp(self, x):
        """
        Replace the `sharp` attribute.

        Parameters:

        x : float or PyoObject
            New `sharp` attribute, in the range 0 -> 1.

        """
        self._sharp = x
        x, lmax = convertArgsToLists(x)
        [obj.setSharp(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def setType(self, x):
        """
        Replace the `type` attribute.

        Parameters:

        x : int
            New `type` attribute. Choices are :
            0 = Saw up, 1 = Saw down, 2 = Square, 3 = Triangle, 4 = Pulse
            5 = Bipolar pulse, 6 = Sample and hold, 7 = Modulated Sine
            

        """
        if x >= 0 and x < 8:
            self._type = x
            x, lmax = convertArgsToLists(x)
            [obj.setType(wrap(x,i)) for i, obj in enumerate(self._base_objs)]

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = [SLMapFreq(self._freq), SLMap(0., 1., "lin", "sharp", self._sharp), SLMapMul(self._mul)]
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def freq(self):
        """float or PyoObject. Oscillator frequency in cycles per second.""" 
        return self._freq
    @freq.setter
    def freq(self, x): self.setFreq(x)

    @property
    def sharp(self):
        """float or PyoObject. Sharpness factor {0 -> 1}.""" 
        return self._sharp
    @sharp.setter
    def sharp(self, x): self.setSharp(x)

    @property
    def type(self):
        """int. Waveform type.""" 
        return self._type
    @type.setter
    def type(self, x): self.setType(x)
