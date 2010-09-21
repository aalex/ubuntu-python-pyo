"""
Hand-written granulation module.

"""

from pyo import *
import random

s = Server().boot()

t1 = SndTable(DEMOS_PATH + '/transparent.aif')
t2 = HannTable()
t3 = LinTable()

num_of_grains = 50
grain_dur = .1
snd_dur = 1./t1.getRate()

met = Metro(time=grain_dur/num_of_grains, poly=num_of_grains)
mvt = Randi(freq=.25)
pos = TrigRand(met, min=mvt, max=mvt+.05)
jit = TrigRand(met, min=.95, max=1.05)
env = TrigEnv(met, t2, dur=grain_dur, mul=.1)
ind = TrigEnv(met, t3, dur=grain_dur, mul=jit*(grain_dur/snd_dur), add=pos)
snd = Pointer(t1, ind, env).out()

def play():
    met.play()
    jit.max = random.uniform(1, 1.05)

def stop():
    met.stop()

play()
    
s.gui(locals())    