"""
//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//=========================================================

This file is used by OOMidi for launching a Pyro name service and connecting a remote object to the global Python functions
"""

import Pyro.naming
import Pyro.core
from Pyro.errors import PyroError,NamingError
import sys, time
import threading

#
# Note: this module, 'oom' is activated from within OOMidi - thus it is not possible to execute the scripts without a running
# OOMidi instance
#
import oom 

#
# Class which implements the functionality that is used remotely. 
# In short just repeating the global functions in the oom-module
#
# TODO: It should be better to skip this class completely by implementing 
# functionality as a class in pyapi.cpp instead of global functions
# that need to be wrapped like this
#
class OOMidi:
      def getCPos(self): # Get current position
            return oom.getCPos()

      def startPlay(self): # Start playback
            return oom.startPlay()

      def stopPlay(self): # Stop playback
            return oom.stopPlay()

      def rewindStart(self): # Rewind current position to start
            return oom.rewindStart()

      def getLPos(self): # Get position of left locator
            return oom.getLPos()

      def getRPos(self): # Get position of right locator
            return oom.getRPos()

      def getTempo(self, tick): #Get tempo at particular tick
            return oom.getTempo(tick)

      def getTrackNames(self): # get track names
            return oom.getTrackNames()

      def getParts(self, trackname): # get parts in a particular track
            return oom.getParts(trackname)

      def createPart(self, trackname, starttick, lenticks, part): # create part in track
            return oom.createPart(trackname, starttick, lenticks, part)

      def modifyPart(self, part): # modify a part (the part to be modified is specified by its id
            return oom.modifyPart((part))

      def deletePart(self, part): # delete a part
            return oom.deletePart((part))

      def getSelectedTrack(self): # get first selected track in arranger window
            return oom.getSelectedTrack()

      def importPart(self, trackname, filename, tick): # import part file to a track at a given position
            return oom.importPart(trackname, filename, tick)

      def setCPos(self, tick): # set current position
            return oom.setPos(0, tick)

      def setLPos(self, tick): # set left locator
            return oom.setPos(1, tick)

      def setRPos(self, tick): # set right locator
            return oom.setPos(2, tick)
      
      def setSongLen(self, ticks): # set song length
            return oom.setSongLen(ticks)

      def getSongLen(self): # get song length
            return oom.getSongLen()

      def getDivision(self): # get division (ticks per 1/4, or per beat?)
            return oom.getDivision()

      def setMidiTrackParameter(self, trackname, paramname, value): # set midi track parameter (velocity, compression, len, transpose)
            return oom.setMidiTrackParameter(trackname, paramname, value);

      def getLoop(self): # get loop flag
            return oom.getLoop()

      def setLoop(self, loopFlag): # set loop flag
            return oom.setLoop(loopFlag)
      
      def getMute(self, trackname): # get track mute parameter
            return oom.getMute(trackname)

      def setMute(self, trackname, enabled): # set track mute parameter
            return oom.setMute(trackname, enabled)

      def setVolume(self, trackname, volume): # set mixer volume
            return oom.setVolume(trackname, volume)

      def getMidiControllerValue(self, trackname, ctrlno): # get a particular midi controller value for a track
            return oom.getMidiControllerValue(trackname, ctrlno)

      def setMidiControllerValue(self, trackname, ctrlno, value): # set a particular midi controller value for a track
            return oom.setMidiControllerValue(trackname, ctrlno, value)

      def setAudioTrackVolume(self, trackname, dvol): # set volume for audio track 
            return oom.setAudioTrackVolume(trackname, dvol)

      def getAudioTrackVolume(self, trackname): # get volume for audio track
            return oom.getAudioTrackVolume(trackname)

      def getTrackEffects(self, trackname): # get effect names for an audio track
            return oom.getTrackEffects(trackname)

      def toggleTrackEffect(self, trackname, effectno, onoff): # toggle specific effect on/off
            return oom.toggleTrackEffect(trackname, effectno, onoff)

      def findNewTrack(self, oldtracknames): #internal function
            tracknames = oom.getTrackNames()
            for trackname in tracknames:
                  if trackname in oldtracknames:
                        continue

                  return trackname

      def changeTrackName(self, trackname, newname): #change track name
            return oom.changeTrackName(trackname, newname)

      def nameNewTrack(self, newname, oldtracknames):# Internal function, wait until new track shows up in tracknames, then rename it
            tmpname = None
            for i in range(0,100):
                  tmpname = self.findNewTrack(oldtracknames)
                  if tmpname == None:
                        time.sleep(0.1)
                        continue
                  else:
                        self.changeTrackName(tmpname, newname)
                        time.sleep(0.1) # Ouch!!
                        break


      def addMidiTrack(self, trackname): # add midi track
            oldtracknames = oom.getTrackNames()
            if trackname in oldtracknames:
                  return None

            oom.addMidiTrack()
            self.nameNewTrack(trackname, oldtracknames)
            

      def addWaveTrack(self, trackname): # add wave track
            oldtracknames = oom.getTrackNames()
            if trackname in oldtracknames:
                  return None

            oom.addWaveTrack()
            self.nameNewTrack(trackname, oldtracknames)

      def addInput(self, trackname): # add audio input
            oldtracknames = oom.getTrackNames()
            if trackname in oldtracknames:
                  return None

            oom.addInput()
            self.nameNewTrack(trackname, oldtracknames)

      def addOutput(self, trackname): # add audio output
            oldtracknames = oom.getTrackNames()
            if trackname in oldtracknames:
                  return None

            oom.addOutput()
            self.nameNewTrack(trackname, oldtracknames)

      def addGroup(self, trackname): # add audio group
            oldtracknames = oom.getTrackNames()
            if trackname in oldtracknames:
                  return None

            oom.addGroup()
            self.nameNewTrack(trackname, oldtracknames)

      def deleteTrack(self, trackname): # delete a track
            tracknames = oom.getTrackNames()
            if trackname not in tracknames:
                  return False

            oom.deleteTrack(trackname)

#      def getOutputRoute(self, trackname):
#            return oom.getOutputRoute(trackname)

class NameServiceThread(threading.Thread):
      def __init__(self):
            threading.Thread.__init__(self)
            self.starter = Pyro.naming.NameServerStarter()

      def run(self):
            self.starter.start()

      def verifyRunning(self):
            return self.starter.waitUntilStarted(10)

#
# oomclass Pyro object
#
class oomclass(Pyro.core.ObjBase, OOMidi):
      pass

#
# main server program
#
def main():
      Pyro.core.initServer()
      nsthread = NameServiceThread()
      nsthread.start()
      if (nsthread.verifyRunning() == False):
            print "Failed to launch name service..."
            sys.exit(1)

      daemon = Pyro.core.Daemon()
      # locate the NS
      locator = Pyro.naming.NameServerLocator()
      #print 'searching for Name Server...'
      ns = locator.getNS()
      daemon.useNameServer(ns)

      # connect a new object implementation (first unregister previous one)
      try:
            # 'test' is the name by which our object will be known to the outside world
            ns.unregister('oom')
      except NamingError:
            pass

      # connect new object implementation
      daemon.connect(oomclass(),'oom')

      # enter the server loop.
      print 'OOMidi remote object published'
      daemon.requestLoop()

if __name__=="__main__":
        main()

main()


