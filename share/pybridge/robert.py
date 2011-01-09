# -*- coding: utf-8 -*-
#
# Example client for OOMidi Pyro bridge (Python Remote Object)
#
import Pyro.core
import time

oom=Pyro.core.getProxyForURI('PYRONAME://:Default.oom')
print "Current position is: " + str(oom.getCPos())

midiDevice=file("/dev/snd/midiC1D0")
nextIsCommand=False
while True:
  v=midiDevice.read(1)
  if nextIsCommand:
    print "   %d"%ord(v)
    if ord(v) == 0:
	print "set hh"
        oom.setMute("hh", False)
        oom.setMute("RIDE", True)
    if ord(v) == 1:
        oom.setMute("hh", True)
        oom.setMute("RIDE", False)
	print "set ride"
    if ord(v) == 2:
        oom.setMute("ACCENT1", False)
    if ord(v) == 3:
        oom.setMute("ACCENT2", False)
    if ord(v) == 127:
	print "mute all accents"
        oom.setMute("ACCENT1", True)
        oom.setMute("ACCENT2", True)
    nextIsCommand=False
  if ord(v) == 192:
     nextIsCommand=True

'''
oom.startPlay()
time.sleep(1) # Sleep one second
oom.stopPlay()
print "New position is: " + str(oom.getCPos())
oom.rewindStart()
print "Pos after rewind is: " + str(oom.getCPos())
print "Lpos, Rpos: " + str(oom.getLPos()) + ":" + str(oom.getRPos())

'''
