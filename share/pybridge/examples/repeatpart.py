"""
//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//=========================================================
"""

import Pyro.core
import sys
import time

SLEEPIVAL=0.3

def advanceToNextSection(oom, newlpos, newrpos):
      print "Advancing..."
      currpos = oom.getRPos()
      curlpos = oom.getLPos()
      curpos = oom.getCPos()
      oom.setLoop(False)

      while curpos < currpos:
            time.sleep(SLEEPIVAL)
            curpos = oom.getCPos()
      print "Leaving current section..."
      oom.setRPos(newrpos)
      curpos = oom.getCPos()

      while curpos < newlpos:
            time.sleep(SLEEPIVAL)
            curpos = oom.getCPos()
      print "Entered new section"
      oom.setLPos(newlpos)
      oom.setLoop(True)
      return

oom=Pyro.core.getProxyForURI('PYRONAME://:Default.oom')
oom.stopPlay()
parts = oom.getParts("Track 1")
oom.setLPos(parts[0]['tick'])
oom.setRPos(parts[0]['tick'] + parts[0]['len'])
oom.setCPos(0)
time.sleep(0.2) # Hmmm, don't like it but it seems necessary to pause a short while before starting play
oom.setLoop(True)
oom.startPlay()

for i in range(1, len(parts)):
      part = parts[i]
      tick = part['tick']
      len = part['len']
      print "Press enter to advance to next section/part!"
      sys.stdin.read(1)
      advanceToNextSection(oom, tick, tick + len)

print "This is the final section. Disabling loop and leaving..."
oom.setLoop(False)

#print "Press enter to leave final section"
#sys.stdin.read(1)
#oom.setLoop(False)

