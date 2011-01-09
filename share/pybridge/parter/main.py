import sys,time
from PyQt4 import QtGui

from parter import ParterMainwidget
import sys, os
import Pyro.core

#import oommock
#oom = oommock.OOMidiMock()
oom=Pyro.core.getProxyForURI('PYRONAME://:Default.oom')
"""
strack = oom.getSelectedTrack()
cpos = oom.getCPos()
oom.importPart(strack, "/home/ddskmlg/.oom/parts/testpart2.mpt", cpos)
sys.exit(0)
"""


if __name__ == '__main__':
      app = QtGui.QApplication(sys.argv)
      partsdir = os.getenv("HOME") + "/.oom/parts"
      mainw = ParterMainwidget(None, oom, partsdir)
      mainw.show()
      #oom.importPart("Track 1","/home/ddskmlg/.oom/parts/testpart2.mpt",18432)
      sys.exit(app.exec_())

