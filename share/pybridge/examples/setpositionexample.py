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
parts = oom.getParts("Track 1")

ptick = parts[0]['tick']
len = parts[0]['len']
oom.setLPos(ptick)
oom.setRPos(ptick + len)
oom.setCPos(ptick + len / 2)

songlen = oom.getSongLen()
#print "Song length: " + str(songlen)

#
# Copy first part to after current song length, thus increase song length with length of first part
#
newsonglen = songlen + parts[0]['len']
oom.setSongLen(newsonglen)
oom.createPart("Track 1", songlen + 1, parts[0]['len'], parts[0])
time.sleep(1)

lastpart = oom.getParts("Track 1").pop()
print lastpart['id']
oom.deletePart(lastpart['id'])
print oom.getDivision()

