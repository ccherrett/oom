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
trackname = "wavtrack"

fxs = oom.getTrackEffects(trackname)
print fxs

for i in range (0,10):
      oom.toggleTrackEffect(trackname,0, False)
      time.sleep(1)
      oom.toggleTrackEffect(trackname,0, True)
      time.sleep(1)

