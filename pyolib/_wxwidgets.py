"""
Copyright 2010 Olivier Belanger

This file is part of pyo.

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

import wx, os, sys, math, time
from types import ListType, FloatType, IntType
from wx.lib.embeddedimage import PyEmbeddedImage

try:
    from PIL import Image, ImageDraw, ImageTk
except:
    pass
    
BACKGROUND_COLOUR = "#EBEBEB"

vu_metre = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAAMgAAAAFCAIAAACPTDSjAAAAAXNSR0IArs4c6QAAACF0RVh0"
    "U29mdHdhcmUAR3JhcGhpY0NvbnZlcnRlciAoSW50ZWwpd4f6GQAAAMJJREFUeJxiYBgFo4BG"
    "wHWbO1bEKc6JS4pXiddxtTNWKTFLMYspVlilpFyk9WsNsUopR6toF+lglVJP0wDKYpUCmiYX"
    "II9VyqTTFOgSrFI28+0E9YSwSgE9hcfXLNwsZEgBDcQVVkBnAB2DKxi7qg13LHHERIEeMvWF"
    "ulilYoIUM2JUsUoVp2vGKyjsdbDHRH0G+u4SElilZpkYW4uIYJXaaGNtwMDwHxsaTVijCWs0"
    "YY0mrNGENZqwRhMWAAAA//8DAHGDnlocOW36AAAAAElFTkSuQmCC")

vu_metre_dark = PyEmbeddedImage(
    "iVBORw0KGgoAAAANSUhEUgAAAMgAAAAFCAYAAAAALqP0AAAAAXNSR0IArs4c6QAAAAlwSFlz"
    "AAALEgAACxIB0t1+/AAADst0RVh0Q29tbWVudABwclZXIGNodW5rbGVuIDMwMiBpZ25vcmVk"
    "Og1BU0NJSTogeJzt0U1WwjAuwPHpLohUKS5tibG3yM4ude11ei4u4OtdvIE4ky76cOVz+/9l"
    "LuYjaS68f759yKu8nMys6zTPc8rm9Exq1C6nLicuS7UwcS5ljHGMMopEyyQu0S5FJGUuLi4u"
    "Li5Xdb2pd/cuu1pj899y+6ixrTV+lufcktvvLl7p1ut+8C7r9efnUut2Kb/PhOshu5vK9I5l"
    "LtrQtiG0wdmmq3IuT7ffLp1vOt9rLnvfaVjprfSNdo69jvy+P5fPjZbDfunZuSYNSEVYOiA3"
    "ODlDRUREMTRENTZDMjMwMTBDMEYxRTkwNzg4NTQyOTBENkQ4OUIxQjdDOENFMDM3NUVENzU3"
    "QTE5MEZFMEVCNURCQzgxMzg5MzAyRkE3MEU1NzNGQkZGNjUxQUU2MjM2OTE3QkM3RkJFN0RD"
    "OEFCQkM5Q0NDQUNFQjM0Q0Y3M0NBRTZGNDRDNkFENDE4QTcxODI3MTE0QkI1MzA3MTFDNjU4"
    "QzcxOEMzMjhBNDRDQjI0MUFEMTE1NDUyNDY1MDAwMDAwMDAwMA1ta0JGIGNodW5rbGVuIDcy"
    "IGlnbm9yZWQ6DUFTQ0lJOiD63sr+Li4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4u"
    "Li4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4NSEVYOiBGQURFQ0FGRTAwMDAw"
    "MDA0MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDANbWtUUyBjaHVua2xlbiA4ODg2IGlnbm9yZWQ6DUFT"
    "Q0lJOiB4nO1dWXPbSJLG9ozbLd/unph92C5FbGzsU2twLnwuRVHSWC6HpHy9OHjB1rSPLllW"
    "t5fB/76ZWVUgUCiAOChSni6rW0WiUC6+zPwqqyouOnnWup51ensuM2ve+8cpJKHfLobj+cvj"
    "vXBmzl+x5MVRO5zZtjk/PC6EM2/e2++Hs4Y97/XPLyC7dS4uhPRv3j0+vp61uvBrb3fweWZs"
    "LiNjbLwxusbU+C6fLoz386PTLsi5LjkuIccyfobcLuN3uOP9vNc+LmGVuw1IRVg6IDc4OUNF"
    "RDVENTk3M0RCNDg5MkM2RjY4Q0RCMkRERkVFOUU5ODdERDgxNzQ1NkM2Q0VDNTM2QjcwMTM3"
    "QzE0NDU1MUQyNTgwNzg3QTQ3Q0JEMzg3OEMxRDZCNDhGMUU1OTU2Qjc5N0MxRkZCRTk5NTk1"
    "NTIwNTAyODgwMzgyODUyOUUyRUFCNUI0NUEyNTAwN0JFQ0NGQzJBQUIyQTBCM0E3OUQ2QkE5"
    "RTc1N0E3QjE3MzM2QkRFRkJDNzI5MjRBMURGMDg4NkUzDW1rQlMgY2h1bmtsZW4gMTkwIGln"
    "bm9yZWQ6DUFTQ0lJOiB4nF1Oyy6CMC7szd/wLi6DwFHKq2GrLmoub2hswlWTJmaz/27Lw4Nz"
    "mcnMzmZknS4sLj6iTy5wjS71M11FpjEu91QupdGPLmryVqPj9jLag7S0Lb2AoC6DcOgupnV5"
    "t/GlLkdwlG9kLi5sYC72ZC+2ZT7Jdi452C7PXZPXzshBLi6y/C7dqZg2zfS38NzZ2Z5HlS7D"
    "g1R7LjH2SC77UYlsxEgnOopp0YOOnqvexY9w1WEuJ0SZOi6kLl+6Ll+mDUhFWDogNzg5QzVE"
    "NEVDQjBFODIzMDEwRUNDRERGRjAxMzAwODNDMDUxQ0FBQjYxQUIwNjZBMDQ2RjY4NkNDMjU1"
    "OTMyNjY2QjNGRjZFQ0JDMzgzNzM5OUM5Q0NDRTY2NjQ5RDFBMkMxQTNFQTI0RjFENzA4RDFF"
    "RjUzMzVENDVBNjMxMDhGNzU0MDlBNUQxOEYwMjZBRjI1NkEzRTNGNjMyREE4M0I0QjQyREJE"
    "ODBBMDA3ODM3MEU4MERBNjc1NzlCN0YxQTUwMTQ3NzANbWtCVCBjaHVua2xlbiAxMTQ1IGln"
    "bm9yZWQ6DUFTQ0lJOiD6zsr+Ln84xS4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4u"
    "Li4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4ueJztmolt6zAuLl1ILkkhKSSN"
    "pJAukkZSiD82+GM8bEjZsWT4mi4udJDisctDIrXfK6WUUkoppZRSSv3X9/f3/uvra0qF34Oy"
    "LpdM+y7pX1NVn91uN+Xz83P/+vr6c37LdacuVdYtVb5/eXk52GPr9K+t9P/7+/svSnWseg1I"
    "RVg6IEZBQ0VDQUZFMDA3RjM4QzUwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwNzg5Q0VE"
    "OUE4OTZERUIzMDEwMDU1RDQ4MUE0OTIxMjkyNDhEQTQ5MDE0OTI0NjUyDW1rQlQgY2h1bmts"
    "ZW4gMzM5IGlnbm9yZWQ6DUFTQ0lJOiD6zsr+Ln9ViS4uLi4uLi4uLi4uLi4uLi4uLi4uLi4u"
    "Li4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4ueJzt1uFpg2Au"
    "hlEucS4ucS4ucS4ucS4usbyBLremIf+KLueBQ5tP++tNbM5TkiRJkiRJkiRJkiRJkiRJkiRJ"
    "LtFxLue+70/nOcu1d/e/uk/3b13Xcy7Hc5qmx8/sLv0s99S9dS7LsjxexzAuf76HdO+yY5V9"
    "s2F2rc37PQ1IRVg6IEZBQ0VDQUZFMDA3RjU1ODkwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwNzg5Q0VERDZFMTY5ODM2MDE0ODY1MTA3NzExMTA3NzExMDE3NzExMDA3NzExMTA3DW1r"
    "QlQgY2h1bmtsZW4gMzc5OSBpZ25vcmVkOg1BU0NJSTog+s7K/i5/n3guLi4uLi4uLi4uLi4u"
    "Li4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4u"
    "Lnic7Z2NkS4pLoUuiC5xIC7EiTguLuJELshe6eo+17tnSUDPz/5Yr2pqZ7tpLi4u0IOel5fB"
    "YDAuLi6DwWAwLi4ug8HgP/z69evl58+ff3ziOveq5+JzpawuZfj3wf9R6fmK/jN8//795dOn"
    "T3984jr3Mnz58uXfzy6+ffsNSEVYOiBGQUNFQ0FGRTAwN0Y5Rjc4MDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDc4OUNFRDlEOEQ5MTFDMjkwQzg1MUQ4ODEzNzEyMDBFQzQ4OTM4MTAw"
    "N0UyNDQxQw1ta0JUIGNodW5rbGVuIDI3NDEgaWdub3JlZDoNQVNDSUk6IPrOyv4uf69+Li4u"
    "Li4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4u"
    "Li4uLi4uLi4uLi54nO2djZHbOC5GU0guSSEpJI2kkC6SRlJIbpCbd/PuLkjJWa8u23gzntXq"
    "h6QuLqIukPr5cy6GYS6GYS6GYS6GYXhJvn///tvvx48u/y67J1WOe5fh2fnw4cNvv69fv/6q"
    "99q+Z/1XOaouw/uBvM/i9vCW/rm7to7Vbyd/DUhFWDogRkFDRUNBRkUwMDdGQUY3RTAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDA3ODlDRUQ5RDhEOTFEQjM4MEM0NjUzNDgxQTQ5MjEy"
    "OTI0OERBNDkwMTQ5MjQ2NTINbWtCVCBjaHVua2xlbiAxMDc3NSBpZ25vcmVkOg1BU0NJSTog"
    "+s7K/i5/1PAuLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4uLi4u"
    "Li4uLi4uLi4uLi4uLi4uLi4uLi4uLnic7X0ruOwo1vaSSCwuicQikUgkLi6JxCIjkVgkLi6J"
    "jYyMjI0smX9R+5zunp7p+dT/1Ihac+k+VXvXLrAu77suVObnfTaeLtqzkS3G10Zgh6PDLnBd"
    "xS5rLt+FfsPzYi7ggS4uLrYuLtCeJMF33ZPZsYTB8c18c/zxQ28NSEVYOiBGQUNFQ0FGRTAw"
    "N0ZENEYwMDAwMDAwMDEwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAw"
    "MDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDc4OUNFRDdEMkJCOEVDMjhENkY2"
    "OTI0ODJDMTI4OUM0MjI5MTQ4MjQxNjE5ODlDNIWzHPoAAAAhdEVYdFNvZnR3YXJlAEdyYXBo"
    "aWNDb252ZXJ0ZXIgKEludGVsKXeH+hkAAADWSURBVHic7JO7CsJAEEVH95UHpFBEhBUTJIpB"
    "I0GFFDYRBNHSxr/Zn/B/R9c+Ewvt5sLtDswwwwHgcDh06muAVE3Y62TipA+HM80MxgLKoyGZ"
    "kRWw3GmSsbmEealIJi2Ue3Mk4+dMUukopqj1Z2+KqRqDybBPMv4239xRqt8wflbXP/zOfveu"
    "n9VVgLcmam1mJew3hmQWmXJFrklmu9K41to94hjbegoCzKQEirmEIVohSOYeRWiVhud0hm1l"
    "QVgQFoQFYUFYEBaEBWFB/iLICwAA//8DAHqeTXUOgGpTAAAAAElFTkSuQmCC")

def interpFloat(t, v1, v2):
    "interpolator for a single value; interprets t in [0-1] between v1 and v2"
    return (v2-v1)*t + v1

def tFromValue(value, v1, v2):
    "returns a t (in range 0-1) given a value in the range v1 to v2"
    return float(value-v1)/(v2-v1)

def clamp(v, minv, maxv):
    "clamps a value within a range"
    if v<minv: v=minv
    if v> maxv: v=maxv
    return v

def toLog(t, v1, v2):
    return math.log10(t/v1) / math.log10(v2/v1)

def toExp(t, v1, v2):
    return math.pow(10, t * (math.log10(v2) - math.log10(v1)) + math.log10(v1))

POWOFTWO = {2:1, 4:2, 8:3, 16:4, 32:5, 64:6, 128:7, 256:8, 512:9, 1024:10, 2048:11, 4096:12, 8192:13, 16384:14, 32768:15, 65536:16}
def powOfTwo(x):
    return 2**x

def powOfTwoToInt(x):
    return POWOFTWO[x]
        
def GetRoundBitmap( w, h, r ):
    maskColor = wx.Color(0,0,0)
    shownColor = wx.Color(5,5,5)
    b = wx.EmptyBitmap(w,h)
    dc = wx.MemoryDC(b)
    dc.SetBrush(wx.Brush(maskColor))
    dc.DrawRectangle(0,0,w,h)
    dc.SetBrush(wx.Brush(shownColor))
    dc.SetPen(wx.Pen(shownColor))
    dc.DrawRoundedRectangle(0,0,w,h,r)
    dc.SelectObject(wx.NullBitmap)
    b.SetMaskColour(maskColor)
    return b

def GetRoundShape( w, h, r ):
    return wx.RegionFromBitmap( GetRoundBitmap(w,h,r) )

class ControlSlider(wx.Panel):
    def __init__(self, parent, minvalue, maxvalue, init=None, pos=(0,0), size=(200,16), log=False, outFunction=None, integer=False, powoftwo=False, backColour=None):
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, pos=pos, size=size, style=wx.NO_BORDER | wx.WANTS_CHARS | wx.EXPAND)
        self.parent = parent
        if backColour: 
            self.backgroundColour = backColour
        else: 
            self.backgroundColour = BACKGROUND_COLOUR
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.SetBackgroundColour(self.backgroundColour)
        self.SetMinSize(self.GetSize())
        self.knobSize = 40
        self.knobHalfSize = 20
        self.sliderHeight = 11
        self.outFunction = outFunction
        self.integer = integer
        self.log = log
        self.powoftwo = powoftwo
        if self.powoftwo:
            self.integer = True
            self.log = False
        self.SetRange(minvalue, maxvalue)
        self.borderWidth = 1
        self.selected = False
        self._enable = True
        self.propagate = True
        self.new = ''
        if init != None: 
            self.SetValue(init)
            self.init = init
        else: 
            self.SetValue(minvalue)
            self.init = minvalue
        self.clampPos()
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_LEFT_DCLICK, self.DoubleClick)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_KEY_DOWN, self.keyDown)
        self.Bind(wx.EVT_KILL_FOCUS, self.LooseFocus)
        self.createSliderBitmap()
        self.createKnobBitmap()

    def getMinValue(self):
        return self.minvalue

    def getMaxValue(self):
        return self.maxvalue

    def Enable(self):
        self._enable = True
        self.Refresh()

    def Disable(self):
        self._enable = False
        self.Refresh()
        
    def setSliderHeight(self, height):
        self.sliderHeight = height
        self.createSliderBitmap()
        self.createKnobBitmap()
        self.Refresh()

    def createSliderBitmap(self):
        w, h = self.GetSize()
        b = wx.EmptyBitmap(w,h)
        dc = wx.MemoryDC(b)
        dc.SetPen(wx.Pen(self.backgroundColour, width=1))
        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.DrawRectangle(0,0,w,h)
        dc.SetBrush(wx.Brush("#999999"))
        dc.SetPen(wx.Pen(self.backgroundColour, width=1))
        h2 = self.sliderHeight / 4
        dc.DrawRoundedRectangle(0,h2,w,self.sliderHeight,2)
        dc.SelectObject(wx.NullBitmap)
        b.SetMaskColour("#999999")
        self.sliderMask = b

    def createKnobBitmap(self):
        w, h = self.knobSize, self.GetSize()[1]
        b = wx.EmptyBitmap(w,h)
        dc = wx.MemoryDC(b)
        rec = wx.Rect(0, 0, w, h)
        dc.SetPen(wx.Pen(self.backgroundColour, width=1))
        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.DrawRectangleRect(rec)
        h2 = self.sliderHeight / 4
        rec = wx.Rect(0, h2, w, self.sliderHeight)
        dc.GradientFillLinear(rec, "#414753", "#99A7CC", wx.BOTTOM)
        dc.SetBrush(wx.Brush("#999999"))
        dc.DrawRoundedRectangle(0,0,w,h,2)
        dc.SelectObject(wx.NullBitmap)
        b.SetMaskColour("#999999")
        self.knobMask = b

    def getInit(self):
        return self.init

    def SetRange(self, minvalue, maxvalue):   
        self.minvalue = minvalue
        self.maxvalue = maxvalue

    def getRange(self):
        return [self.minvalue, self.maxvalue]

    def scale(self):
        inter = tFromValue(self.pos, self.knobHalfSize, self.GetSize()[0]-self.knobHalfSize)
        if not self.integer:
            return interpFloat(inter, self.minvalue, self.maxvalue)
        elif self.powoftwo:
            return powOfTwo(int(interpFloat(inter, self.minvalue, self.maxvalue)))    
        else:
            return int(interpFloat(inter, self.minvalue, self.maxvalue))

    def SetValue(self, value, propagate=True):
        self.propagate = propagate
        if self.HasCapture():
            self.ReleaseMouse()
        if self.powoftwo:
            value = powOfTwoToInt(value)  
        value = clamp(value, self.minvalue, self.maxvalue)
        if self.log:
            t = toLog(value, self.minvalue, self.maxvalue)
            self.value = interpFloat(t, self.minvalue, self.maxvalue)
        else:
            t = tFromValue(value, self.minvalue, self.maxvalue)
            self.value = interpFloat(t, self.minvalue, self.maxvalue)
        if self.integer:
            self.value = int(self.value)
        if self.powoftwo:
            self.value = powOfTwo(self.value)    
        self.clampPos()
        self.selected = False
        self.Refresh()

    def GetValue(self):
        if self.log:
            t = tFromValue(self.value, self.minvalue, self.maxvalue)
            val = toExp(t, self.minvalue, self.maxvalue)
        else:
            val = self.value
        if self.integer:
            val = int(val)
        return val

    def LooseFocus(self, event):
        self.selected = False
        self.Refresh()

    def keyDown(self, event):
        if self.selected:
            char = ''
            if event.GetKeyCode() in range(324, 334):
                char = str(event.GetKeyCode() - 324)
            elif event.GetKeyCode() == 390:
                char = '-'
            elif event.GetKeyCode() == 391:
                char = '.'
            elif event.GetKeyCode() == wx.WXK_BACK:
                if self.new != '':
                    self.new = self.new[0:-1]
            elif event.GetKeyCode() < 256:
                char = chr(event.GetKeyCode())
            if char in ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.', '-']:
                self.new += char
            elif event.GetKeyCode() in [wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER]:
                self.SetValue(eval(self.new))
                self.new = ''
                self.selected = False
            self.Refresh()
 
    def MouseDown(self, evt):
        if evt.ShiftDown():
            self.DoubleClick(evt)
            return
        if self._enable:
            size = self.GetSize()
            self.pos = clamp(evt.GetPosition()[0], self.knobHalfSize, size[0]-self.knobHalfSize)
            self.value = self.scale()
            self.CaptureMouse()
            self.selected = False
            self.Refresh()
        evt.Skip()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def DoubleClick(self, event):
        if self._enable:
            w, h = self.GetSize()
            pos = event.GetPosition()
            if wx.Rect(self.pos-self.knobHalfSize, 0, self.knobSize, h).Contains(pos):
                self.selected = True
            self.Refresh()
        event.Skip()
            
    def MouseMotion(self, evt):
        if self._enable:
            size = self.GetSize()
            if self.HasCapture():
                self.pos = clamp(evt.GetPosition()[0], self.knobHalfSize, size[0]-self.knobHalfSize)
                self.value = self.scale()
                self.selected = False
                self.Refresh()

    def OnResize(self, evt):
        self.createSliderBitmap()
        self.clampPos()    
        self.Refresh()

    def clampPos(self):
        size = self.GetSize()
        if self.powoftwo:
            val = powOfTwoToInt(self.value)
        else:
            val = self.value    
        self.pos = tFromValue(val, self.minvalue, self.maxvalue) * (size[0] - self.knobSize) + self.knobHalfSize
        self.pos = clamp(self.pos, self.knobHalfSize, size[0]-self.knobHalfSize)
        
    def setBackgroundColour(self, colour):
        self.backgroundColour = colour
        self.SetBackgroundColour(self.backgroundColour)
        self.createSliderBitmap()
        self.createKnobBitmap()
        self.Refresh()

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)

        dc.SetBrush(wx.Brush(self.backgroundColour, wx.SOLID))
        dc.Clear()

        # Draw background
        dc.SetPen(wx.Pen(self.backgroundColour, width=self.borderWidth, style=wx.SOLID))
        dc.DrawRectangle(0, 0, w, h)

        # Draw inner part
        if self._enable: sliderColour =  "#99A7CC"
        else: sliderColour = "#BBBBBB"
        h2 = self.sliderHeight / 4
        rec = wx.Rect(0, h2, w, self.sliderHeight)
        dc.GradientFillLinear(rec, "#646986", sliderColour, wx.BOTTOM)
        dc.DrawBitmap(self.sliderMask, 0, 0, True)

        # Draw knob
        if self._enable: knobColour = '#888888'
        else: knobColour = "#DDDDDD"
        rec = wx.Rect(self.pos-self.knobHalfSize, 0, self.knobSize, h)  
        dc.GradientFillLinear(rec, "#424864", knobColour, wx.RIGHT)
        dc.DrawBitmap(self.knobMask, rec[0], rec[1], True)
        
        if self.selected:
            rec2 = wx.Rect(self.pos-self.knobHalfSize, 0, self.knobSize, h)  
            dc.SetBrush(wx.Brush('#333333', wx.SOLID))
            dc.SetPen(wx.Pen('#333333', width=self.borderWidth, style=wx.SOLID))  
            dc.DrawRoundedRectangleRect(rec2, 3)

        if sys.platform in ['win32', 'linux2']:
            dc.SetFont(wx.Font(7, wx.ROMAN, wx.NORMAL, wx.NORMAL))
        else:    
            dc.SetFont(wx.Font(10, wx.ROMAN, wx.NORMAL, wx.NORMAL))

        # Draw text
        if self.selected and self.new:
            val = self.new
        else:
            if self.integer:
                val = '%d' % self.GetValue()
            elif abs(self.GetValue()) >= 10000:
                val = '%.1f' % self.GetValue()
            elif abs(self.GetValue()) >= 1000:
                val = '%.2f' % self.GetValue()
            elif abs(self.GetValue()) >= 100:
                val = '%.3f' % self.GetValue()
            elif abs(self.GetValue()) < 100:
                val = '%.4f' % self.GetValue()
        if sys.platform == 'linux2':
            width = len(val) * (dc.GetCharWidth() - 3)
        else:
            width = len(val) * dc.GetCharWidth()
        dc.SetTextForeground('#FFFFFF')
        dc.DrawLabel(val, rec, wx.ALIGN_CENTER)

        # Send value
        if self.outFunction and self.propagate:
            self.outFunction(self.GetValue())
        self.propagate = True

        evt.Skip()

class MultiSlider(wx.Panel):
    def __init__(self, parent, init, key, command, slmap): 
        wx.Panel.__init__(self, parent, size=(250,250))
        self.backgroundColour = BACKGROUND_COLOUR
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.SetBackgroundColour(self.backgroundColour)
        self.Bind(wx.EVT_SIZE, self.OnResize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self._slmap = slmap
        self._values = [slmap.set(x) for x in init]
        self._nchnls = len(init)
        self._labels = init
        self._key = key
        self._command = command
        self._height = 16
        if sys.platform in ['win32', 'linux2']:
            self._font = wx.Font(7, wx.ROMAN, wx.NORMAL, wx.NORMAL)
        else:    
            self._font = wx.Font(10, wx.ROMAN, wx.NORMAL, wx.NORMAL)
            
        self.SetSize((250, self._nchnls*16))
        self.SetMinSize((250,self._nchnls*16))

    def OnResize(self, event):
        self.Layout()
        self.Refresh()
        
    def OnPaint(self, event):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush(self.backgroundColour))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        dc.SetBrush(wx.Brush("#000000"))
        dc.SetFont(self._font)
        dc.SetTextForeground('#999999')
        for i in range(self._nchnls):
            x = int(self._values[i] * w)
            y = self._height * i
            dc.DrawRectangle(0, y+1, x, self._height-2)
            rec = wx.Rect(w/2-15, y, 30, self._height)
            dc.DrawLabel("%s" % self._labels[i], rec, wx.ALIGN_CENTER)

    def MouseDown(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        slide = pos[1] / self._height
        if 0 <= slide < self._nchnls:
            self._values[slide] = pos[0] / float(w)
            self._labels = [self._slmap.get(x) for x in self._values]
            self._command(self._key, self._labels)
            self.CaptureMouse()
        self.Refresh()
        evt.Skip()

    def MouseUp(self, evt):
        if self.HasCapture():
            self.ReleaseMouse()

    def MouseMotion(self, evt):
        w,h = self.GetSize()
        pos = evt.GetPosition()
        if evt.Dragging() and evt.LeftIsDown():
            slide = pos[1] / self._height
            if 0 <= slide < self._nchnls:
                self._values[slide] = pos[0] / float(w)
                self._labels = [self._slmap.get(x) for x in self._values]
                self._command(self._key, self._labels)
            self.Refresh()
        
class VuMeter(wx.Panel):
    def __init__(self, parent, size=(200,11), numSliders=2):
        wx.Panel.__init__(self, parent, -1, size=size)
        self.parent = parent
        self.SetBackgroundColour("#000000")
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.old_nchnls = numSliders
        self.numSliders = numSliders
        self.SetMinSize((200,5*self.numSliders+1))
        self.SetSize((200, 5*self.numSliders+1))
        self.bitmap = vu_metre.GetBitmap()
        self.backBitmap = vu_metre_dark.GetBitmap()
        self.amplitude = [0] * self.numSliders

        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_CLOSE, self.OnClose)   

    def setNumSliders(self, numSliders):
        oldChnls = self.old_nchnls
        self.numSliders = numSliders
        self.amplitude = [0] * self.numSliders
        gap = (self.numSliders - oldChnls) * 5
        parentSize = self.parent.GetSize()
        if sys.platform == 'linux2':
            self.SetSize((200, 5*self.numSliders+1))
            self.SetMinSize((200, 5*self.numSliders+1))
            self.parent.SetSize((parentSize[0], parentSize[1]+gap))
            self.parent.SetMinSize((parentSize[0], parentSize[1]+gap))
        else:
            self.SetSize((200, 5*self.numSliders+1))
            self.SetMinSize((200, 5*self.numSliders+1))
            self.parent.SetSize((parentSize[0], parentSize[1]+gap))
            self.parent.SetMinSize((parentSize[0], parentSize[1]+gap))
        self.Refresh()
        self.parent.Layout()

    def setRms(self, *args):
        if args[0] < 0: 
            return
        if not args:
            self.amplitude = [0 for i in range(self.numSliders)]                
        else:
            self.amplitude = args
        wx.CallAfter(self.Refresh)   

    def OnPaint(self, event):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#000000"))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        for i in range(self.numSliders):
            db = math.log10(self.amplitude[i]+0.00001) * 0.2 + 1.
            width = int(db*w)
            dc.DrawBitmap(self.backBitmap, 0, i*5)
            dc.SetClippingRegion(0, i*5, width, 5)
            dc.DrawBitmap(self.bitmap, 0, i*5)
            dc.DestroyClippingRegion()
        event.Skip()
        
    def OnClose(self, evt):
        self.Destroy()

######################################################################
### Control window for PyoObject
######################################################################
class Command:
    def __init__(self, func, key):
        self.func = func
        self.key = key

    def __call__(self, value):
        self.func(self.key, value)

class PyoObjectControl(wx.Frame):
    def __init__(self, parent=None, obj=None, map_list=None):
        wx.Frame.__init__(self, parent)
        from controls import SigTo
        self.menubar = wx.MenuBar()        
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_CLOSE, self._destroy)
        self._obj = obj
        self._map_list = map_list
        self._sliders = []
        self._excluded = []
        self._values = {}
        self._displays = {}
        self._maps = {}
        self._sigs = {}
        
        panel = wx.Panel(self)
        panel.SetBackgroundColour(BACKGROUND_COLOUR)
        mainBox = wx.BoxSizer(wx.VERTICAL)
        self.box = wx.FlexGridSizer(10,2,5,5)
        
        for i, m in enumerate(self._map_list):
            key, init, mini, maxi, scl, res = m.name, m.init, m.min, m.max, m.scale, m.res
            # filters PyoObjects
            if type(init) not in [ListType, FloatType, IntType]:
                self._excluded.append(key)
            else:    
                self._maps[key] = m
                # label (param name)
                label = wx.StaticText(panel, -1, key)
                # create and pack slider
                if type(init) != ListType:
                    if scl == 'log': scl = True
                    else: scl = False
                    if res == 'int': res = True
                    else: res = False
                    self._sliders.append(ControlSlider(panel, mini, maxi, init, log=scl, size=(225,16),
                                        outFunction=Command(self.setval, key), integer=res))
                    self.box.AddMany([(label, 0, wx.LEFT, 5), (self._sliders[-1], 1, wx.EXPAND | wx.LEFT, 5)])   
                else:
                    self._sliders.append(MultiSlider(panel, init, key, self.setval, m))
                    self.box.AddMany([(label, 0, wx.LEFT, 5), (self._sliders[-1], 1, wx.EXPAND | wx.LEFT, 5)])   
                # set obj attribute to PyoObject SigTo  
                self._values[key] = init
                self._sigs[key] = SigTo(init, .025, init)
                refStream = self._obj.getBaseObjects()[0]._getStream()
                server = self._obj.getBaseObjects()[0].getServer()
                for k in range(len(self._sigs[key].getBaseObjects())):
                    curStream = self._sigs[key].getBaseObjects()[k]._getStream()
                    server.changeStreamPosition(refStream, curStream)
                setattr(self._obj, key, self._sigs[key])
        self.box.AddGrowableCol(1, 1) 
        mainBox.Add(self.box, 1, wx.EXPAND | wx.TOP | wx.BOTTOM | wx.RIGHT, 10)

        if sys.platform == "linux2":
            Y_OFF = 15
        elif sys.platform == "win32":
            Y_OFF = 55
        else:
            Y_OFF = 20    
        panel.SetSizerAndFit(mainBox)
        x,y = panel.GetSize()
        self.SetSize((-1, y+Y_OFF))
        self.SetMinSize((-1, y+Y_OFF))
        self.SetMaxSize((-1, y+Y_OFF))
        
    def _destroy(self, event):
        for m in self._map_list:
            key = m.name
            if key not in self._excluded:
                setattr(self._obj, key, self._values[key])
                del self._sigs[key]
        self.Destroy()        

    def setval(self, key, x):
        self._values[key] = x
        setattr(self._sigs[key], "value", x)

######################################################################
### View window for PyoTableObject
######################################################################
class ViewTable_withPIL(wx.Frame):
    def __init__(self, parent, samples=None, tableclass=None):
        wx.Frame.__init__(self, parent)
        self.menubar = wx.MenuBar()        
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.width = 500
        self.height = 200
        self.half_height = self.height / 2
        if sys.platform == "linux2":
            Y_OFF = 25
        elif sys.platform == "win32":
            Y_OFF = 55
        else:
            Y_OFF = 30
        self.SetSize((self.width+10, self.height+Y_OFF))
        self.SetMinSize((self.width+10, self.height+Y_OFF))
        self.SetMaxSize((self.width+10, self.height+Y_OFF))
        im = Image.new("L", (self.width, self.height), 255)
        draw = ImageDraw.Draw(im)
        draw.line(samples, fill=0, width=1)
        image = wx.EmptyImage(self.width, self.height)
        image.SetData(im.convert("RGB").tostring())
        self.img = wx.BitmapFromImage(image)

    def _destroy(self, evt):
        self.Destroy()

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        dc.DrawBitmap(self.img, 0, 0)
        dc.SetPen(wx.Pen('#BBBBBB', width=1, style=wx.SOLID))  
        dc.DrawLine(0, self.half_height, self.width, self.half_height)

class ViewTable_withoutPIL(wx.Frame):
    def __init__(self, parent, samples=None, tableclass=None):
        wx.Frame.__init__(self, parent)
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)
        self.menubar = wx.MenuBar()        
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.width = 500
        self.height = 200
        self.half_height = self.height / 2
        if sys.platform == "linux2":
            Y_OFF = 35
        else:
            Y_OFF = 40
        self.SetSize((self.width+10, self.height+Y_OFF))
        self.SetMinSize((self.width+10, self.height+Y_OFF))
        self.SetMaxSize((self.width+10, self.height+Y_OFF))
        self.tableclass = tableclass
        if sys.platform == 'win32':
            if tableclass == 'SndTable':
                self.samples = [(samples[i], samples[i+1], samples[i+2], samples[i+3]) for i in range(0, len(samples), 4)]
            else:    
                self.samples = [(samples[i], samples[i+1]) for i in range(0, len(samples), 2)]
        else:        
            self.samples = [(samples[i], samples[i+1], samples[i+2], samples[i+3]) for i in range(0, len(samples), 4)]

    def _destroy(self, evt):
        self.Destroy()

    def OnPaint(self, evt):
        w,h = self.GetSize()
        dc = wx.AutoBufferedPaintDC(self)
        dc.SetBrush(wx.Brush("#FFFFFF"))
        dc.Clear()
        dc.DrawRectangle(0,0,w,h)
        if sys.platform == 'win32':
            if self.tableclass == 'SndTable':
                dc.DrawLineList(self.samples)
            else:
                dc.DrawPointList(self.samples)
        else:
            dc.DrawLineList(self.samples)
        dc.SetPen(wx.Pen('#BBBBBB', width=1, style=wx.SOLID))  
        dc.DrawLine(0, self.half_height, self.width, self.half_height)

######################################################################
## View window for PyoMatrixObject
#####################################################################
class ViewMatrix_withPIL(wx.Frame):
    def __init__(self, parent, samples=None, size=None):
        wx.Frame.__init__(self, parent)
        self.menubar = wx.MenuBar()        
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        if sys.platform == "linux2":
            Y_OFF = 0
        elif sys.platform == "win32":
            Y_OFF = 45
        else:
            Y_OFF = 22
        self.SetSize((size[0], size[1]+Y_OFF))
        self.SetMinSize((size[0], size[1]+Y_OFF))
        self.SetMaxSize((size[0], size[1]+Y_OFF))
        im = Image.new("L", size, None)
        im.putdata(samples)
        image = wx.EmptyImage(size[0], size[1])
        image.SetData(im.convert("RGB").tostring())
        self.img = wx.BitmapFromImage(image)

    def _destroy(self, evt):
        self.Destroy()

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        dc.DrawBitmap(self.img, 0, 0)

class ViewMatrix_withoutPIL(wx.Frame):
    def __init__(self, parent, samples=None, size=None):
        wx.Frame.__init__(self, parent)
        self.menubar = wx.MenuBar()        
        self.fileMenu = wx.Menu()
        self.fileMenu.Append(-1, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
        self.fileMenu.Bind(wx.EVT_MENU, self._destroy)
        self.menubar.Append(self.fileMenu, "&File")
        self.SetMenuBar(self.menubar)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        if sys.platform == "linux2":
            Y_OFF = 0
        else:
            Y_OFF = 22
        self.SetSize((size[0], size[1]+Y_OFF))
        self.SetMinSize((size[0], size[1]+Y_OFF))
        self.SetMaxSize((size[0], size[1]+Y_OFF))
        self.width = size[0]
        self.height = size[1]
        self.samples = samples

    def _destroy(self, evt):
        self.Destroy()

    def OnPaint(self, evt):
        dc = wx.PaintDC(self)
        for i in range(self.width*self.height):
            x = i % self.width
            y = i / self.width
            amp = int(self.samples[i])
            amp = hex(amp).replace('0x', '')
            if len(amp) == 1:
                amp = "0%s" % amp
            amp = "#%s%s%s" % (amp, amp, amp)
            dc.SetPen(wx.Pen(amp, width=1, style=wx.SOLID))  
            dc.DrawPoint(x, y)

######################################################################
## Grapher window for PyoTableObject control
######################################################################
OFF = 15
OFF2 = OFF*2
RAD = 3
RAD2 = RAD*2
AREA = RAD+2
AREA2 = AREA*2
class Grapher(wx.Panel):
    def __init__(self, parent, xlen=8192, yrange=(0.0, 1.0), init=[(0.0,0.0),(1.0,1.0)], mode=0, 
                 exp=10.0, inverse=True, tension=0.0, bias=0.0, outFunction=None): 
        wx.Panel.__init__(self, parent, size=(500,250), style=wx.SUNKEN_BORDER)
        self.backgroundColour = BACKGROUND_COLOUR
        self.SetBackgroundStyle(wx.BG_STYLE_CUSTOM)  
        self.SetBackgroundColour(self.backgroundColour)
        self.Bind(wx.EVT_LEAVE_WINDOW, self.OnLeave)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.MouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.MouseUp)
        self.Bind(wx.EVT_MOTION, self.MouseMotion)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        
        self.mode = mode
        self.exp = exp
        self.inverse = inverse
        self.tension = tension
        self.bias = bias
        self.pos = (OFF+RAD,OFF+RAD)
        self.selected = None
        self.xlen = xlen
        self.yrange = yrange
        self.init = [tup for tup in init]
        self.points = init
        self.outFunction = outFunction

    def setInitPoints(self, pts):
        self.init = [(p[0],p[1]) for p in pts]
        self.points = [(p[0],p[1]) for p in pts]
        self.selected = None
        self.Refresh()

    def pointToPixels(self, pt):
        w,h = self.GetSize()
        w,h = w-OFF2-RAD2, h-OFF2-RAD2
        x = int(round(pt[0] * w)) + OFF + RAD
        y = int(round(pt[1] * h)) + OFF + RAD
        return x, y

    def pixelsToPoint(self, pos):
        w,h = self.GetSize()
        w,h = w-OFF2-RAD2, h-OFF2-RAD2
        x = (pos[0] - OFF - RAD) / float(w)
        y = (pos[1] - OFF - RAD) / float(h)
        return x, y

    def pointToValues(self, pt):
        x = pt[0] * self.xlen
        if type(self.xlen) == IntType:
            x = int(x)
        y = pt[1] * (self.yrange[1]-self.yrange[0]) + self.yrange[0]
        return x, y

    def borderClip(self, pos):
        w,h = self.GetSize()
        if pos[0] < (OFF+RAD): pos[0] = (OFF+RAD)
        elif pos[0] > (w-OFF-RAD): pos[0] = w-OFF-RAD
        if pos[1] < (OFF+RAD): pos[1] = (OFF+RAD)
        elif pos[1] > (h-OFF-RAD): pos[1] = h-OFF-RAD
        return pos

    def pointClip(self, pos):
        w,h = self.GetSize()
        if self.selected == 0:
            leftclip = OFF+RAD
        else:
            x,y = self.pointToPixels(self.points[self.selected-1])
            leftclip = x
        if self.selected == (len(self.points) - 1):
            rightclip = w-OFF-RAD
        else:    
            x,y = self.pointToPixels(self.points[self.selected+1])
            rightclip = x

        if pos[0] < leftclip: pos[0] = leftclip
        elif pos[0] > rightclip: pos[0] = rightclip
        if pos[1] < (OFF+RAD): pos[1] = (OFF+RAD)
        elif pos[1] > (h-OFF-RAD): pos[1] = h-OFF-RAD
        return pos

    def reset(self):
        self.points = self.init
        self.Refresh()

    def getPoints(self):
        return [tup for tup in self.points]

    def getValues(self):
        values = []
        for pt in self.points:
            x,y = self.pointToValues(pt)
            values.append((x,y))
        return values

    def sendValues(self):
        if self.outFunction != None:
            values = self.getValues()
            self.outFunction(values)

    def OnLeave(self, evt):
        self.pos = (OFF+RAD,OFF+RAD)
        self.Refresh()

    def OnKeyDown(self, evt):
        if self.selected != None and evt.GetKeyCode() in [wx.WXK_BACK, wx.WXK_DELETE, wx.WXK_NUMPAD_DELETE]:
            del self.points[self.selected]
            self.sendValues()
            self.selected = None
            self.Refresh()
        elif evt.GetKeyCode() in [wx.WXK_UP, wx.WXK_NUMPAD_UP]:
            self.points = [(pt[0], pt[1]+0.002) for pt in self.points]
            self.sendValues()
            self.Refresh()
        elif evt.GetKeyCode() in [wx.WXK_DOWN, wx.WXK_NUMPAD_DOWN]:
            self.points = [(pt[0], pt[1]-0.002) for pt in self.points]
            self.sendValues()
            self.Refresh()

    def MouseDown(self, evt):
        self.CaptureMouse()
        w,h = self.GetSize()
        self.pos = self.borderClip(evt.GetPosition())
        self.pos[1] = h - self.pos[1]
        for i, p in enumerate(self.points):
            x, y = self.pointToPixels(p)
            if wx.Rect(x-AREA, y-AREA, AREA2, AREA2).Contains(self.pos):
                # Grab a point
                self.selected = i
                self.Refresh()
                return
        # Add a point
        pt = self.pixelsToPoint(self.pos)
        for i, p in enumerate(self.points):
            if p >= pt:
                self.points.insert(i, pt)
                break
        self.selected = self.points.index(pt)
        self.Refresh()

    def MouseUp(self, evt):
        if self.HasCapture(): 
            self.ReleaseMouse()
            self.sendValues()

    def MouseMotion(self, evt):
        w,h = self.GetSize()
        self.pos = self.borderClip(evt.GetPosition())
        self.pos[1] = h - self.pos[1]
        if self.HasCapture():
            if self.selected != None:
                self.pos = self.pointClip(self.pos)
                x, y = self.pixelsToPoint(self.pos)
                self.points[self.selected] = (x, y)
        self.Refresh()

    def getCosPoints(self, pt1, pt2):
        tmp = []
        steps = pt2[0] - pt1[0]
        for i in range(steps):
            mu = float(i) / steps
            mu2 = (1. - math.cos(mu*math.pi)) * 0.5
            tmp.append((pt1[0]+i, pt1[1] * (1. - mu2) + pt2[1] * mu2))
        return tmp    

    def getExpPoints(self, pt1, pt2):
        tmp = []
        ambitus = pt2[1] - pt1[1]
        steps = pt2[0] - pt1[0]
        if steps == 0:
            inc = 1.0 / 0.0001
        else:
            inc = 1.0 / steps
        pointer = 0.0
        if self.inverse:
            if ambitus >= 0:
                for i in range(steps):
                    scl = 1.0 - pow(1.0 - pointer, self.exp)
                    tmp.append((pt1[0]+i, scl * ambitus + pt1[1]))
                    pointer += inc
            else:
                for i in range(steps):
                    scl = pow(pointer, self.exp)
                    tmp.append((pt1[0]+i, scl * ambitus + pt1[1]))
                    pointer += inc
        else:
            for i in range(steps):
                scl = pow(pointer, self.exp)
                tmp.append((pt1[0]+i, scl * ambitus + pt1[1]))
                pointer += inc
        return tmp

    def addImaginaryPoints(self, tmp):
        lst = []
        x = tmp[1][0] - tmp[0][0]
        if tmp[0][1] < tmp[1][1]:
            y = tmp[0][1] - tmp[1][1]
        else:
            y = tmp[0][1] + tmp[1][1]
        lst.append((x,y))
        lst.extend(tmp)
        x = tmp[-2][0] - tmp[-1][0]
        if tmp[-2][1] < tmp[-1][1]:
            y = tmp[-1][1] + tmp[-2][1]
        else:
            y = tmp[-1][1] - tmp[-2][1]
        lst.append((x,y))
        return lst

    def getCurvePoints(self, pt0, pt1, pt2, pt3):
        tmp = []
        y0, y1, y2, y3 = pt0[1], pt1[1], pt2[1], pt3[1]
        steps = pt2[0] - pt1[0]
        for i in range(steps):
            mu = float(i) / steps
            mu2 = mu * mu
            mu3 = mu2 * mu
            m0 = (y1 - y0) * (1.0 + self.bias) * (1.0 - self.tension) * 0.5
            m0 += (y2 - y1) * (1.0 - self.bias) * (1.0 - self.tension) * 0.5
            m1 = (y2 - y1) * (1.0 + self.bias) * (1.0 - self.tension) * 0.5
            m1 += (y3 - y2) * (1.0 - self.bias) * (1.0 - self.tension) * 0.5
            a0 = 2.0 * mu3 - 3.0 * mu2 + 1.0
            a1 = mu3 - 2.0 * mu2 + mu
            a2 = mu3 - mu2
            a3 = -2.0 * mu3 + 3.0 * mu2
            tmp.append((pt1[0]+i, a0*y1 + a1*m0 + a2*m1 + a3*y2))
        return tmp    
        
    def OnPaint(self, evt):
        w,h = self.GetSize()
        corners = [(OFF,OFF),(w-OFF,OFF),(w-OFF,h-OFF),(OFF,h-OFF)]
        dc = wx.AutoBufferedPaintDC(self)
        if sys.platform != "win32":
            font, ptsize = dc.GetFont(), dc.GetFont().GetPointSize()
        else:
            font, ptsize = dc.GetFont(), 10
        font.SetPointSize(ptsize-4)
        dc.SetFont(font)
        dc.SetTextForeground("#888888")
        dc.Clear()

        # Draw grid
        dc.SetPen(wx.Pen("#CCCCCC", 1))
        xstep = int(round((w-OFF2) / float(10)))
        ystep = int(round((h-OFF2) / float(10)))
        for i in range(10):
            xpos = i * xstep + OFF
            dc.DrawLine(xpos, OFF, xpos, h-OFF)
            ypos = i * ystep + OFF
            dc.DrawLine(OFF, ypos, w-OFF, ypos)
            if i > 0:
                if type(self.xlen) == IntType:
                    t = "%d" % int(self.xlen * i * 0.1)
                else:
                    t = "%.2f" % (self.xlen * i * 0.1)
                dc.DrawText(t, xpos+2, h-OFF-10)
            if i < 9:
                t = "%.2f" % ((9-i) * 0.1 * (self.yrange[1]-self.yrange[0]) + self.yrange[0])    
                dc.DrawText(t, OFF+1, ypos+ystep-10)
            else:
                t = "%.2f" % ((9-i) * 0.1 * (self.yrange[1]-self.yrange[0]) + self.yrange[0])    
                dc.DrawText(t, OFF+1, h-OFF-10)

        dc.SetPen(wx.Pen("#000000", 1))
        dc.SetBrush(wx.Brush("#000000"))
        # Draw bounding box        
        for i in range(4):
            dc.DrawLinePoint(corners[i], corners[(i+1)%4])

        # Convert points in pixels
        w,h = w-OFF2-RAD2, h-OFF2-RAD2
        tmp = []
        for p in self.points:
            x = int(round(p[0] * w)) + OFF + RAD
            y = int(round((1.0-p[1]) * h)) + OFF + RAD
            tmp.append((x,y))

        # Draw lines
        dc.SetPen(wx.Pen("#000000", 1))
        last_p = None
        if len(tmp) > 1:
            if self.mode == 0:
                for i in range(len(tmp)-1):
                    dc.DrawLinePoint(tmp[i], tmp[i+1])
            elif self.mode == 1:
                for i in range(len(tmp)-1):
                    tmp2 = self.getCosPoints(tmp[i], tmp[i+1])
                    if i == 0 and len(tmp2) < 2:
                        dc.DrawLinePoint(tmp[i], tmp[i+1])
                    if last_p != None:
                        dc.DrawLinePoint(last_p, tmp[i])
                    for j in range(len(tmp2)-1):
                        dc.DrawLinePoint(tmp2[j], tmp2[j+1])
                        last_p = tmp2[j+1]
                if last_p != None:
                    dc.DrawLinePoint(last_p, tmp[-1])
            elif self.mode == 2:
                for i in range(len(tmp)-1):
                    tmp2 = self.getExpPoints(tmp[i], tmp[i+1])
                    if i == 0 and len(tmp2) < 2:
                        dc.DrawLinePoint(tmp[i], tmp[i+1])
                    if last_p != None:
                        dc.DrawLinePoint(last_p, tmp[i])
                    for j in range(len(tmp2)-1):
                        dc.DrawLinePoint(tmp2[j], tmp2[j+1])
                        last_p = tmp2[j+1]
                if last_p != None:
                    dc.DrawLinePoint(last_p, tmp[-1])
            elif self.mode == 3:
                curvetmp = self.addImaginaryPoints(tmp)
                for i in range(1, len(curvetmp)-2):
                    tmp2 = self.getCurvePoints(curvetmp[i-1], curvetmp[i], curvetmp[i+1], curvetmp[i+2])
                    if i == 1 and len(tmp2) < 2:
                        dc.DrawLinePoint(curvetmp[i], curvetmp[i+1])
                    if last_p != None:
                        dc.DrawLinePoint(last_p, curvetmp[i])
                    for j in range(len(tmp2)-1):
                        dc.DrawLinePoint(tmp2[j], tmp2[j+1])
                        last_p = tmp2[j+1]
                if last_p != None:
                    dc.DrawLinePoint(last_p, tmp[-1])

        # Draw points
        for i,p in enumerate(tmp):
            if i == self.selected:
                dc.SetBrush(wx.Brush("#FFFFFF"))
            else:
                dc.SetBrush(wx.Brush("#000000"))
            dc.DrawCircle(p[0],p[1],RAD)
        
        # Draw position values
        font.SetPointSize(ptsize-3)
        dc.SetFont(font)
        dc.SetTextForeground("#222222")
        posptx, pospty = self.pixelsToPoint(self.pos)
        xval, yval = self.pointToValues((posptx, pospty))
        if type(self.xlen) == IntType:
            dc.DrawText("%d, %.3f" % (xval, yval), w-75, OFF)
        else:
            dc.DrawText("%.3f, %.3f" % (xval, yval), w-75, OFF)

class TableGrapher(wx.Frame):
        def __init__(self, parent=None, obj=None, mode=0, xlen=8192, yrange=(0.0, 1.0)):
            wx.Frame.__init__(self, parent)
            pts = obj.getPoints()
            self.yrange = yrange
            for i in range(len(pts)):
                x = pts[i][0] / float(xlen)
                y = (pts[i][1] - float(yrange[0])) / (yrange[1]-yrange[0])
                pts[i] = (x,y)
            if mode == 2:
                self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, exp=obj.exp, inverse=obj.inverse, outFunction=obj.replace)
            elif mode == 3:
                self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, tension=obj.tension, bias=obj.bias, outFunction=obj.replace)
            else:
                self.graph = Grapher(self, xlen=xlen, yrange=yrange, init=pts, mode=mode, outFunction=obj.replace)

            self.menubar = wx.MenuBar()        
            self.fileMenu = wx.Menu()
            self.fileMenu.Append(9999, 'Close\tCtrl+W', kind=wx.ITEM_NORMAL)
            self.Bind(wx.EVT_MENU, self.close, id=9999)
            self.fileMenu.AppendSeparator()
            self.fileMenu.Append(10000, 'Copy all points to the clipboard (4 digits of precision)\tCtrl+C', kind=wx.ITEM_NORMAL)
            self.Bind(wx.EVT_MENU, self.copy, id=10000)
            self.fileMenu.Append(10001, 'Copy all points to the clipboard (full precision)\tShift+Ctrl+C', kind=wx.ITEM_NORMAL)
            self.Bind(wx.EVT_MENU, self.copy, id=10001)
            self.fileMenu.AppendSeparator()
            self.fileMenu.Append(10002, 'Reset\tCtrl+R', kind=wx.ITEM_NORMAL)
            self.Bind(wx.EVT_MENU, self.reset, id=10002)
            self.menubar.Append(self.fileMenu, "&File")
            self.SetMenuBar(self.menubar)

            self.clipboard = wx.Clipboard()

        def close(self, evt):
            self.Destroy()

        def copy(self, evt):
            pts = self.graph.getValues()
            if evt.GetId() == 10000:
                pstr = "["
                for i, pt in enumerate(pts):
                    pstr += "("
                    if type(pt[0]) == IntType:
                        pstr += "%d," % pt[0]
                    else:
                        pstr += "%.4f," % pt[0]
                    pstr += "%.4f)" % pt[1]
                    if i < (len(pts)-1):
                        pstr += ","
                pstr += "]" 
            else:
                pstr = str(pts)           
            data = wx.TextDataObject(pstr)
            ret = self.clipboard.Open()
            if ret:
                self.clipboard.SetData(data)
                self.clipboard.Close()
        
        def reset(self, evt):
            self.graph.reset()

class ServerGUI(wx.Frame):
    def __init__(self, parent=None, nchnls=2, startf=None, stopf=None, recstartf=None, 
                recstopf=None, ampf=None, started=0, locals=None, shutdown=None, meter=True, timer=True, amp=1.):
        wx.Frame.__init__(self, parent)

        self.SetTitle("pyo server")
        
        self.menubar = wx.MenuBar()
        self.menu = wx.Menu()
        self.menu.Append(22999, 'Start/Stop\tCtrl+R', kind=wx.ITEM_NORMAL)
        self.Bind(wx.EVT_MENU, self.start, id=22999)
        quit_item = self.menu.Append(23000, "Quit\tCtrl+Q")  
        self.Bind(wx.EVT_MENU, self.on_quit, id=23000)
        if wx.Platform == "__WXMAC__":
            wx.App.SetMacExitMenuItemId(quit_item.GetId())
        self.menubar.Append(self.menu, "&File")
        self.SetMenuBar(self.menubar)

        self.shutdown = shutdown
        self.locals = locals
        self.nchnls = nchnls
        self.startf = startf
        self.stopf = stopf
        self.recstartf = recstartf
        self.recstopf = recstopf
        self.ampf = ampf
        self._started = False
        self._recstarted = False
        self._history = []
        self._histo_count = 0

        panel = wx.Panel(self)
        panel.SetBackgroundColour(BACKGROUND_COLOUR)
        box = wx.BoxSizer(wx.VERTICAL)

        if sys.platform == "linux2":
            X_OFF = 0
            Y_OFF = 30
            buttonSize = (72,-1)
            leftMargin = 25
        elif sys.platform == "win32":
            try:
                if sys.getwindowsversion()[0] >= 6:
                    X_OFF = 16
                else:
                    X_OFF = 8
            except:
                X_OFF = 8
            Y_OFF = 65
            buttonSize = (72,-1)
            leftMargin = 24
        else:
            X_OFF = 0
            Y_OFF = 35
            buttonSize = wx.DefaultSize
            leftMargin = 25

        buttonBox = wx.BoxSizer(wx.HORIZONTAL)
        self.startButton = wx.Button(panel, -1, 'Start', (20,20), buttonSize)
        self.startButton.Bind(wx.EVT_BUTTON, self.start)
        buttonBox.Add(self.startButton, 0, wx.RIGHT, 5)

        self.recButton = wx.Button(panel, -1, 'Rec Start', (20,20), buttonSize)
        self.recButton.Bind(wx.EVT_BUTTON, self.record)
        buttonBox.Add(self.recButton, 0, wx.RIGHT, 5)

        self.quitButton = wx.Button(panel, -1, 'Quit', (20,20), buttonSize)
        self.quitButton.Bind(wx.EVT_BUTTON, self.on_quit)
        buttonBox.Add(self.quitButton, 0, wx.RIGHT, 0)

        box.Add(buttonBox, 0, wx.TOP | wx.LEFT | wx.RIGHT, 10)
        box.AddSpacer(10)

        box.Add(wx.StaticText(panel, -1, "Amplitude (dB)"), 0, wx.LEFT, leftMargin)
        ampBox = wx.BoxSizer(wx.HORIZONTAL)
        self.ampScale = ControlSlider(panel, -60, 18, 20.0 * math.log10(amp), size=(203, 16), outFunction=self.setAmp)
        ampBox.Add(self.ampScale, 0, wx.LEFT, leftMargin-10)
        box.Add(ampBox, 0, wx.LEFT | wx.RIGHT, 8)
        
        if meter:
            box.AddSpacer(10)
            self.meter = VuMeter(panel, size=(200,5*self.nchnls+1), numSliders=self.nchnls)
            box.Add(self.meter, 0, wx.LEFT, leftMargin-1)
            box.AddSpacer(5)

        if timer:
            box.AddSpacer(10)
            tt = wx.StaticText(panel, -1, "Elapsed time (hh : mm : ss : ms)")
            box.Add(tt, 0, wx.LEFT, leftMargin)
            box.AddSpacer(3)
            self.timetext = wx.StaticText(panel, -1, "00 : 00 : 00 : 000")
            box.Add(self.timetext, 0, wx.LEFT, leftMargin)

        if self.locals != None:
            box.AddSpacer(10)
            t = wx.StaticText(panel, -1, "Interpreter")
            box.Add(t, 0, wx.LEFT, leftMargin)
            self.text = wx.TextCtrl(panel, -1, "", size=(200, -1), style=wx.TE_PROCESS_ENTER)
            self.text.Bind(wx.EVT_TEXT_ENTER, self.getText)
            self.text.Bind(wx.EVT_CHAR, self.onChar)
            box.Add(self.text, 0, wx.LEFT, leftMargin)

        panel.SetSizerAndFit(box)
        x, y = panel.GetSize()
        panel.SetSize((x+X_OFF, y+Y_OFF))
        self.SetSize((x+X_OFF, y+Y_OFF))
        self.SetMinSize(self.GetSize())
        self.SetMaxSize(self.GetSize())

        if started == 1:
            self.start(None, True)

    def setTime(self, *args):
        wx.CallAfter(self.timetext.SetLabel, "%02d : %02d : %02d : %03d" % (args[0], args[1], args[2], args[3]))
        
    def start(self, evt=None, justSet=False):
        if self._started == False:
            if not justSet:
                self.startf()
            self._started = True
            self.startButton.SetLabel('Stop')
            self.quitButton.Disable()
        else:
            self.stopf()
            self._started = False
            self.startButton.SetLabel('Start')
            self.quitButton.Enable()

    def record(self, evt):
        if self._recstarted == False:
            self.recstartf()
            self._recstarted = True
            self.recButton.SetLabel('Rec Stop')
        else:
            self.recstopf()
            self._recstarted = False
            self.recButton.SetLabel('Rec Start')

    def on_quit(self, evt):
        self.shutdown()
        self.Destroy()
        sys.exit()

    def getPrev(self):
        self.text.Clear()
        self._histo_count -= 1
        if self._histo_count < 0:
            self._histo_count = 0
        self.text.SetValue(self._history[self._histo_count])
        wx.CallAfter(self.text.SetInsertionPointEnd)

    def getNext(self):
        self.text.Clear()
        self._histo_count += 1
        if self._histo_count >= len(self._history):
            self._histo_count = len(self._history)
        else:    
            self.text.SetValue(self._history[self._histo_count])
            self.text.SetInsertionPointEnd()

    def getText(self, evt):
        source = self.text.GetValue()
        self.text.Clear()
        exec source in self.locals
        self._history.append(source)
        self._histo_count = len(self._history)

    def onChar(self, evt):
        key = evt.GetKeyCode()
        if key == 315:
            self.getPrev()
        elif key == 317:
            self.getNext()  
        evt.Skip()      

    def setAmp(self, value):
        self.ampf(math.pow(10.0, float(value) * 0.05))

    def setRms(self, *args):
        self.meter.setRms(*args)

if __name__ == "__main__":
    def pprint(values):
        print values

    app = wx.PySimpleApp()
    f = wx.Frame(None, title="test frame", size=(525,275))
    graph = Grapher(f, xlen=8192, yrange=(0.0, 1.0), init=[(0,0),(.5,1),(1,0)], outFunction=pprint)
    f.Show()
    app.MainLoop()
