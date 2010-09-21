#!/usr/bin/env python
# encoding: utf-8
"""
Algorithmic generation...

"""

from pyo import *

s = Server(sr=44100, nchnls=2, buffersize=1024, duplex=0).boot()

wav = SquareTable(6)
env = CosTable([(0,0), (100,1), (500,.3), (8191,0)])

met = Metro(.1, 10).play()
lfo = Phasor(.1, 0, .5, 0)
amp = TrigEnv(met, env, mul=.2)
pit = TrigXnoiseMidi(met, dist='loopseg', x1=1, x2=lfo, scale=1, mrange=(60,84))

b = Osc(wav, pit,  mul=amp).out()

s.gui(locals())

######################
######################
# 0 = uniform
# 1 = linear min
# 2 = linear max
# 3 = triangular
# 4 = exponential min (x1 = slope {0 =  no slope -> 10 = sharp slope})
# 5 = exponential max (x1 = slope {0 =  no slope -> 10 = sharp slope})
# 6 = biexponential (x1 = bandwidth {0 =  huge bandwidth -> 10 = narrow bandwidth})
# 7 = cauchy (x1 = bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth})
# 8 = weibull (x1 = locator {0 -> 1}, x2 = shape {0.5 = linear min, 1.5 = expon min, 3.2 = simili gaussian})
# 9 = gaussian (x1 = locator {0 -> 1}, x2 = bandwidth {0 =  narrow bandwidth -> 10 = huge bandwidth})
# 10 = poisson (x1 = gravity center {0 = low values -> 10 = high values}, x2 = compress/expand range {0.1 = full compress -> 10 full expand})
# 11 = walker (x1 = max value {0.1 -> 1}, x2 = max step {0.1 -> 1})
# 12 = loopseg (x1 = max value {0.1 -> 1}, x2 = max step {0.1 -> 1})
######################
######################
