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
for i in range(0,10):
      oom.setMute("Strings", False)
      oom.setMute("Lead1", True)
      time.sleep(1)
      oom.setMute("Strings", True)
      oom.setMute("Lead1", False)
      time.sleep(1)

