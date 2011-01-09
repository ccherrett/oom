"""
//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//=========================================================
"""

import Pyro.core
import time

oom=Pyro.core.getProxyForURI('PYRONAME://:Default.oom')
#for i in range(0,10):
#      print "Ctrl no " + str(i) + " = " + str(oom.getMidiControllerValue("Track 1", i))

"""
for i in range(0,127):
      oom.setMidiControllerValue("Track 1", 7, i)
      time.sleep(0.1)
"""

oom.setMidiControllerValue("Track 1", 7, 56)
print oom.getMidiControllerValue("Track 1", 7)
print oom.getAudioTrackVolume("Out 1")
oom.setAudioTrackVolume("Out 1", -1.0)

