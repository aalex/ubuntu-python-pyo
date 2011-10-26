"""
Objects to manage values on an Open Sound Control port.

OscSend take the first value of each buffersize and send it on an
OSC port.

OscReceive creates and returns audio streams from the value in its 
input port.

The audio streams of these objects are essentially intended to be
controls and can't be sent to the output soundcard.

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
### Open Sound Control
######################################################################                                       
class OscSend(PyoObject):
    """
    Sends values over a network via the Open Sound Control protocol.
    
    Uses the OSC protocol to share values to other softwares or other 
    computers. Only the first value of each input buffersize will be 
    sent on the OSC port.
    
    Parent class: PyoObject
    
    Parameters:
    
    input : PyoObject
        Input signal.
    port : int
        Port on which values are sent. Receiver should listen on the 
        same port.
    address : string
        Address used on the port to identify values. Address is in 
        the form of a Unix path (ex.: '/pitch').
    host : string, optional
        IP address of the target computer. The default, '127.0.0.1', 
        is the localhost.

    Methods:

    setInput(x, fadetime) : Replace the `input` attribute.

    Notes:

    The out() method is bypassed. OscSend's signal can not be sent 
    to audio outs.
    
    OscSend has no `mul` and `add` attributes.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = Sine(freq=[1,1.5], mul=100, add=[600, 1000])
    >>> b = OscSend(a, port=10000, address=['/pit1','/pit2'])
    
    """
    def __init__(self, input, port, address, host="127.0.0.1"):    
        PyoObject.__init__(self)
        self._input = input
        self._in_fader = InputFader(input)
        in_fader, port, address, host, lmax = convertArgsToLists(self._in_fader, port, address, host)
        self._base_objs = [OscSend_base(wrap(in_fader,i), wrap(port,i), wrap(address,i), wrap(host,i)) for i in range(lmax)]

    def __dir__(self):
        return ['input']

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
            
    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setMul(self, x):
        pass
        
    def setAdd(self, x):
        pass    

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

    @property
    def input(self):
        """PyoObject. Input signal.""" 
        return self._input
    @input.setter
    def input(self, x): self.setInput(x)
         
class OscReceive(PyoObject):
    """
    Receives values over a network via the Open Sound Control protocol.
    
    Uses the OSC protocol to receive values from other softwares or 
    other computers. Get a value at the beginning of each buffersize 
    and fill it's buffer with it.

    Parent class: PyoObject
    
    Parameters:
    
    port : int
        Port on which values are received. Sender should output on 
        the same port.
    address : string
        Address used on the port to identify values. Address is in 
        the form of a Unix path (ex.: '/pitch').

    Methods:

    get(identifier, all) : Return the first sample of the current 
        buffer as a float.

    Notes:
    
    Audio streams are accessed with the `address` string parameter. 
    The user should call :

    OscReceive['/pitch'] to retreive streams named '/pitch'.

    The out() method is bypassed. OscReceive's signal can not be sent 
    to audio outs.

    Examples:
    
    >>> s = Server().boot()
    >>> s.start()
    >>> a = OscReceive(port=10001, address=['/pitch', '/amp'])
    >>> b = Sine(freq=a['/pitch'], mul=a['/amp']).out()

    """

    def __init__(self, port, address, mul=1, add=0):    
        PyoObject.__init__(self)
        self._mul = mul
        self._add = add
        address, mul, add, lmax = convertArgsToLists(address, mul, add)
        self._address = address
        self._mainReceiver = OscReceiver_base(port, address)
        self._base_objs = [OscReceive_base(self._mainReceiver, wrap(address,i), wrap(mul,i), wrap(add,i)) for i in range(lmax)]

    def __dir__(self):
        return ['mul', 'add']

    def __del__(self):
        self._mainReceiver.free_port()
        for obj in self._base_objs:
            obj.deleteStream()
            del obj
        self._mainReceiver.deleteStream()
        del self._mainReceiver
            
    def __getitem__(self, i):
        if type(i) == type(''):
            return self._base_objs[self._address.index(i)]
        elif i < len(self._base_objs):
            return self._base_objs[i]
        else:
            print "'i' too large!"         
    
    def get(self, identifier=None, all=False):
        """
        Return the first sample of the current buffer as a float.
        
        Can be used to convert audio stream to usable Python data.
        
        Address as string must be given to `identifier` to specify
        which stream to get value from.
        
        Parameters:

            identifier : string
                Address string parameter identifying audio stream.
                Defaults to None, useful when `all` is True to 
                retreive all streams values.
            all : boolean, optional
                If True, the first value of each object's stream
                will be returned as a list. Otherwise, only the value
                of the first object's stream will be returned as a float.
                Defaults to False.
                 
        """
        if not all:
            return self._base_objs[self._address.index(identifier)]._getStream().getValue()
        else:
            return [obj._getStream().getValue() for obj in self._base_objs]
             
    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
        
class OscDataSend(PyoObject):
    """
    Sends data values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to share values to other softwares or other 
    computers. Values are sent on the form of a list containing `types`
    elements.

    Parent class: PyoObject

    Parameters:

    types : str
        String specifying the types sequence of the message to be sent.
        Possible values are :
            integer : "i"
            long integer : "h"
            float : "f"
            double : "d"
            string : "s"
        The string "ssfi" indicates that the value to send will be a list
        containing two strings followed by a float and an integer.
    port : int
        Port on which values are sent. Receiver should listen on the 
        same port.
    address : string
        Address used on the port to identify values. Address is in 
        the form of a Unix path (ex.: '/pitch').
    host : string, optional
        IP address of the target computer. The default, '127.0.0.1', 
        is the localhost.

    Methods:

    send(msg, address=None) : Method used to send `msg` values (a list)
        at the `address` destination if specified. Otherwise, values will 
        be sent to all destinations managed by the object.

    Notes:

    The out() method is bypassed. OscDataSend has no audio signal.

    OscDataSend has no `mul` and `add` attributes.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = OscDataSend("fissif", 9900, "/data/test")
    >>> def pp(address, *args):
    ...     print address
    ...     print args
    >>> b = OscDataReceive(9900, "/data/test", pp)
    >>> msg = [3.14159, 1, "Hello", "world!", 2, 6.18]
    >>> a.send(msg)

    """
    def __init__(self, types, port, address, host="127.0.0.1"):    
        PyoObject.__init__(self)
        types, port, address, host, lmax = convertArgsToLists(types, port, address, host)
        self._base_objs = [OscDataSend_base(wrap(types,i), wrap(port,i), wrap(address,i), wrap(host,i)) for i in range(lmax)]
        self._addresses = {}
        for i, adr in enumerate(address):
            self._addresses[adr] = self._base_objs[i]
            
    def __dir__(self):
        return []

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def setMul(self, x):
        pass

    def setAdd(self, x):
        pass    

    def send(self, msg, address=None):
        """
        Method used to send `msg` values as a list.
        
        Parameters:
        
        msg : list
            List of values to send. Types of values in list
            must be of the kind defined of `types` argument
            given at the object's initialization.
        address : string, optional
            Address destination to send values. If None, values
            will be sent to all addresses managed by the object.
        
        """
        if address == None:
            [obj.send(msg) for obj in self._base_objs]
        else:
            self._addresses[address].send(msg)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)

class OscDataReceive(PyoObject):
    """
    Receives data values over a network via the Open Sound Control protocol.

    Uses the OSC protocol to receive data values from other softwares or 
    other computers. When a message is received, the function given at the
    argument `function` is called with the current address destination in 
    argument followed by a tuple of values.

    Parent class: PyoObject

    Parameters:

    port : int
        Port on which values are received. Sender should output on 
        the same port. Only one port per object.
    address : string
        Address used on the port to identify values. Address is in 
        the form of a Unix path (ex.: '/pitch'). There can be as many
        addresses as needed on a single port.
    function : callable
        This function will be called whenever a message with a known
        address is received. Only one function per object.

    Methods:

    getAddresses() : Returns the addresses managed by the object.
    addAddress(path) : Adds new address(es) to the object's handler.

    Notes:

    The definition of function at `function` argument must be in this form :

    def my_func(address, *args):
        ...

    The out() method is bypassed. OscDataReceive has no audio signal.

    OscDataReceive has no `mul` and `add` attributes.

    Examples:

    >>> s = Server().boot()
    >>> s.start()
    >>> a = OscDataSend("fissif", 9900, "/data/test")
    >>> def pp(address, *args):
    ...     print address
    ...     print args
    >>> b = OscDataReceive(9900, "/data/test", pp)
    >>> msg = [3.14159, 1, "Hello", "world!", 2, 6.18]
    >>> a.send(msg)

    """

    def __init__(self, port, address, function, mul=1, add=0):    
        PyoObject.__init__(self)
        self._port = port
        self._address = address
        self._function = function
        address, lmax = convertArgsToLists(address)
        self._base_objs = [OscDataReceive_base(port, address, function)]

    def __dir__(self):
        return []

    def __del__(self):
        self._base_objs[0].free_port()
        for obj in self._base_objs:
            obj.deleteStream()
            del obj

    def out(self, chnl=0, inc=1, dur=0, delay=0):
        return self

    def getAddresses(self):
        """
        Returns the addresses managed by the object.
        
        """
        return self._address

    def addAddress(self, path):
        """
        Adds new address(es) to the object's handler.
        
        Parameters:
        
        path : string or list of strings
            New path(s) to handled.

        """
        self._base_objs[0].addAddress(path)

    def ctrl(self, map_list=None, title=None, wxnoserver=False):
        self._map_list = []
        PyoObject.ctrl(self, map_list, title, wxnoserver)
