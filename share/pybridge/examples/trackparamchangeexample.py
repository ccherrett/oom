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

oom.setMidiTrackParameter("Track 1", "velocity",10)
oom.setMidiTrackParameter("Track 1", "compression",101)
oom.setMidiTrackParameter("Track 1", "delay",2)
oom.setMidiTrackParameter("Track 1", "transposition",1)

for i in range(-127, 127):
      oom.setMidiTrackParameter("Track 1", "velocity",i)
      time.sleep(0.1)

