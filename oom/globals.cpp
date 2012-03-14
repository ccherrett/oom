//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: globals.cpp,v 1.15.2.11 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <QActionGroup>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QPixmap>

#include "globals.h"
#include "config.h"
#include "network/lsclient.h"
#include "network/LSThread.h"
#include "TrackManager.h"

int recFileNumber = 1;

int sampleRate   = 44100;
unsigned segmentSize  = 1024U;    // segmentSize in frames (set by JACK)
unsigned fifoLength =  128;       // 131072/segmentSize
                                  // 131072 - magic number that gives a sufficient buffer size
int segmentCount = 2;

// denormal bias value used to eliminate the manifestation of denormals by
// lifting the zero level slightly above zero
// denormal problems occur when values get extremely close to zero
const float denormalBias=1e-18;

bool overrideAudioOutput = false;
bool overrideAudioInput = false;

QTimer* heartBeatTimer;

bool hIsB = true;             // call note h "b"

const signed char sharpTab[14][7] = {
      { 0, 3, -1, 2, 5, 1, 4 },
      { 0, 3, -1, 2, 5, 1, 4 },
      { 0, 3, -1, 2, 5, 1, 4 },
      { 0, 3, -1, 2, 5, 1, 4 },
      { 2, 5,  1, 4, 7, 3, 6 },
      { 2, 5,  1, 4, 7, 3, 6 },
      { 2, 5,  1, 4, 7, 3, 6 },
      { 4, 0,  3, -1, 2, 5, 1 },
      { 7, 3,  6, 2, 5, 1, 4 },
      { 5, 8,  4, 7, 3, 6, 2 },
      { 3, 6,  2, 5, 1, 4, 7 },
      { 1, 4,  0, 3, 6, 2, 5 },
      { 6, 2,  5, 1, 4, 0, 3 },
      { 0, 3, -1, 2, 5, 1, 4 },
      };
const signed char flatTab[14][7]  = {
      { 4, 1, 5, 2, 6, 3, 7 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 6, 3, 7, 4, 8, 5, 9 },
      { 6, 3, 7, 4, 8, 5, 9 },
      { 6, 3, 7, 4, 8, 5, 9 },

      { 1, 5, 2, 6, 3, 7, 4 },
      { 4, 1, 5, 2, 6, 3, 7 },
      { 2, 6, 3, 7, 4, 8, 5 },
      { 7, 4, 1, 5, 2, 6, 3 },
      { 5, 2, 6, 3, 7, 4, 8 },
      { 3, 0, 4, 1, 5, 2, 6 },
      { 4, 1, 5, 2, 6, 3, 7 },
      };

QString oomGlobalLib;
QString oomGlobalShare;
QString oomUser;
QString oomProject;
QString oomProjectFile;
QString oomProjectInitPath("./");
QString configPath = QString(getenv("HOME")) + QString("/.config/OOMidi");
QString configName = configPath + QString("/OOMidi-").append(VERSION).append(".cfg");
QString routePath = configPath + QString("/routes");
QString oomInstruments;
QString oomUserInstruments;
QString gJackSessionUUID;

QString lastWavePath(".");
QString lastMidiPath(".");

bool debugMode = false;
bool debugMsg = false;
bool midiInputTrace = false;
bool midiOutputTrace = false;
bool realTimeScheduling = false;
int realTimePriority = 40;  // 80
int midiRTPrioOverride = -1;
bool loadPlugins = true;
bool loadVST = true;
bool loadDSSI = true;
bool usePythonBridge = false;
bool useLASH = true;

const QStringList midi_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Midi/Kar (*.mid *.MID *.kar *.KAR *.mid.gz *.mid.bz2);;") +
      QString("Midi (*.mid *.MID *.mid.gz *.mid.bz2);;") +
      QString("Karaoke (*.kar *.KAR *.kar.gz *.kar.bz2);;") +
      QString("All Files (*)")).split(";;");

const QStringList midi_file_save_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Midi (*.mid);;") +
      QString("Karaoke (*.kar);;") +
      QString("All Files (*)")).split(";;");

const QStringList med_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("oom Files (*.oom *.oom.gz *.oom.bz2);;") +
      QString("Uncompressed oom Files (*.oom);;") +
      QString("gzip compressed oom Files (*.oom.gz);;") +
      QString("bzip2 compressed oom Files (*.oom.bz2);;") +
      QString("All Files (*)")).split(";;");
const QStringList med_file_save_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Uncompressed oom Files (*.oom);;") +
      QString("gzip compressed oom Files (*.oom.gz);;") +
      QString("bzip2 compressed oom Files (*.oom.bz2);;") +
      QString("All Files (*)")).split(";;");

const QStringList image_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("(*.jpg *.gif *.png);;") +
      QString("(*.jpg);;") +
      QString("(*.gif);;") +
      QString("(*.png);;") +
      QString("All Files (*)")).split(";;");

const QStringList part_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      //QString("part Files (*.mpt *.mpt.gz *.mpt.bz2);;") +
      QString("part Files (*.mpt);;") +
      QString("All Files (*)")).split(";;");

const QStringList part_file_save_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("part Files (*.mpt);;") +
      QString("All Files (*)")).split(";;");

const QStringList preset_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Presets (*.pre *.pre.gz *.pre.bz2);;") +
      QString("All Files (*)")).split(";;");

const QStringList preset_file_save_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Presets (*.pre);;") +
      QString("gzip compressed presets (*.pre.gz);;") +
      QString("bzip2 compressed presets (*.pre.bz2);;") +
      QString("All Files (*)")).split(";;");

const QStringList drum_map_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Presets (*.map *.map.gz *.map.bz2);;") +
      QString("All Files (*)")).split(";;");
const QStringList drum_map_file_save_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Presets (*.map);;") +
      QString("gzip compressed presets (*.map.gz);;") +
      QString("bzip2 compressed presets (*.map.bz2);;") +
      QString("All Files (*)")).split(";;");

const QStringList audio_file_pattern =  
      QT_TRANSLATE_NOOP("@default", 
      QString("Wave/Binary (*.wav *.ogg *.bin);;") +
      QString("Wave (*.wav *.ogg);;") +
      QString("Binary (*.bin);;") +
      QString("All Files (*)")).split(";;");

Qt::KeyboardModifiers globalKeyState;

// Midi Filter Parameter
int midiInputPorts   = 0;    // receive from all devices
int midiInputChannel = 0;    // receive all channel
int midiRecordType   = 0;    // receive all events
int midiThruType = 0;        // transmit all events
int midiFilterCtrl1 = 0;
int midiFilterCtrl2 = 0;
int midiFilterCtrl3 = 0;
int midiFilterCtrl4 = 0;

QActionGroup* undoRedo;
QAction* undoAction;
QAction* redoAction;
QActionGroup* transportAction;
QAction* playAction;
QAction* startAction;
QAction* stopAction;
QAction* rewindAction;
QAction* forwardAction;
QAction* loopAction;
QAction* replayAction;
QAction* punchinAction;
QAction* punchoutAction;
QAction* recordAction;
QAction* panicAction;
QAction* feedbackAction;
QAction* pcloaderAction;
QAction* noteAlphaAction;
QAction* multiPartSelectionAction;
QAction* masterEnableAction;

OOMidi* oom;

int preMeasures = 2;
unsigned char measureClickNote = 63;
unsigned char measureClickVelo = 127;
unsigned char beatClickNote    = 63;
unsigned char beatClickVelo    = 70;
unsigned char clickChan = 9;
unsigned char clickPort = 0;
bool precountEnableFlag = false;
bool precountFromMastertrackFlag = false;
int precountSigZ = 4;
int precountSigN = 4;
bool precountPrerecord = false;
bool precountPreroll = false;
bool midiClickFlag   = false;
bool audioClickFlag  = true;
float audioClickVolume = 0.5f;

bool rcEnable = false;
unsigned char rcStopNote = 28;
unsigned char rcRecordNote = 31;
unsigned char rcGotoLeftMarkNote = 33;
unsigned char rcPlayNote = 29;
bool automation = true;

QObject* gRoutingPopupMenuMaster = 0;
RouteMenuMap gRoutingMenuMap;
bool gIsOutRoutingPopupMenu = false;

uid_t euid, ruid;  // effective user id, real user id

bool midiSeqRunning = false;

int lastTrackPartColorIndex = 1;

QList<QIcon> partColorIcons;
QList<QIcon> partColorIconsSelected;
QHash<int, QColor> g_trackColorListLine;
QHash<int, QColor> g_trackColorList;
QHash<int, QColor> g_trackColorListSelected;
QHash<int, QPixmap> g_trackDragImageList;
int vuColorStrip = 0; //default vuColor is gradient
bool lsClientStarted = false;
LSClient* lsClient = 0;
LSThread* gLSThread = 0;
bool gUpdateAuxes = false;
TrackManager* trackManager;

QList<QPair<int, QString> > gInputList;
QList<int> gInputListPorts;
bool gSamplerStarted = false;
//---------------------------------------------------------
//   doSetuid
//    Restore the effective UID to its original value.
//---------------------------------------------------------

void doSetuid()
      {
#ifndef RTCAP
      int status;
#ifdef _POSIX_SAVED_IDS
      status = seteuid (euid);
#else
      status = setreuid (ruid, euid);
#endif
      if (status < 0) {
            perror("doSetuid: Couldn't set uid");
            }
#endif
      }

//---------------------------------------------------------
//   undoSetuid
//    Set the effective UID to the real UID.
//---------------------------------------------------------

void undoSetuid()
      {
#ifndef RTCAP
      int status;

#ifdef _POSIX_SAVED_IDS
      status = seteuid (ruid);
#else
      status = setreuid (euid, ruid);
#endif
      if (status < 0) {
            fprintf(stderr, "undoSetuid: Couldn't set uid (eff:%d,real:%d): %s\n",
               euid, ruid, strerror(errno));
            exit (status);
            }
#endif
      }

