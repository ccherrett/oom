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

for j in range(0,5):
      for i in range(0,30):
            oom.addMidiTrack("amiditrack" + str(i))
      for i in range(0,30):
            oom.deleteTrack("amiditrack" + str(i))

for i in range(0, 10):
      print i
      oom.addMidiTrack("amiditrack")
      oom.addWaveTrack("awavetrack")
      oom.addOutput("anoutput")
      oom.addInput("aninput")
      oom.setMute("aninput", False)
      oom.setAudioTrackVolume("aninput",1.0)
      oom.deleteTrack("amiditrack")
      oom.deleteTrack("awavetrack")
      oom.deleteTrack("anoutput")
      oom.deleteTrack("aninput")
      time.sleep(1)

