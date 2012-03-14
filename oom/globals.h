//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: globals.h,v 1.10.2.11 2009/11/25 09:09:43 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef GLOBALS_H
#define GLOBALS_H

#include <sys/types.h>

#include "value.h"
#include "mtc.h"
#include "route.h"

#include <unistd.h>
#include <QList>
#include <QHash>
#include <QPair>

class QString;
class QAction;
class QActionGroup;
class QStringList;
class QIcon;
class QColor;
class QPixmap;
class LSClient;
class TrackManager;
class LSThread;

extern const float denormalBias;

extern int recFileNumber;

extern int sampleRate;
extern unsigned segmentSize;
extern unsigned fifoLength; // inversely proportional to segmentSize
extern int segmentCount;

extern bool overrideAudioOutput;
extern bool overrideAudioInput;

class QTimer;
extern QTimer* heartBeatTimer;

extern bool hIsB;

extern const signed char sharpTab[14][7];
extern const signed char flatTab[14][7];

extern QString oomGlobalLib;
extern QString oomGlobalShare;
extern QString oomUser;
extern QString oomProject;
extern QString oomProjectFile;
extern QString oomProjectInitPath;
extern QString configName;
extern QString configPath;
extern QString routePath;
extern QString oomInstruments;
extern QString oomUserInstruments;
extern QString gJackSessionUUID;

extern QString lastWavePath;
extern QString lastMidiPath;

extern bool debugMode;
extern bool midiInputTrace;
extern bool midiOutputTrace;
extern bool debugMsg;
extern bool debugSync;
extern bool loadPlugins;
extern bool loadVST;
extern bool loadDSSI;
extern bool usePythonBridge;
extern bool useLASH;

extern bool realTimeScheduling;
extern int realTimePriority;
extern int midiRTPrioOverride;

extern const QStringList midi_file_pattern;
extern const QStringList midi_file_save_pattern;
extern const QStringList med_file_pattern;
extern const QStringList med_file_save_pattern;
extern const QStringList image_file_pattern;
extern const QStringList part_file_pattern;
extern const QStringList part_file_save_pattern;
extern const QStringList preset_file_pattern;
extern const QStringList preset_file_save_pattern;
extern const QStringList drum_map_file_pattern;
extern const QStringList drum_map_file_save_pattern;
extern const QStringList audio_file_pattern;

extern Qt::KeyboardModifiers globalKeyState;

extern int midiInputPorts; //!< receive from all devices
extern int midiInputChannel; //!< receive all channel
extern int midiRecordType; //!< receive all events

#define MIDI_FILTER_NOTEON    1
#define MIDI_FILTER_POLYP     2
#define MIDI_FILTER_CTRL      4
#define MIDI_FILTER_PROGRAM   8
#define MIDI_FILTER_AT        16
#define MIDI_FILTER_PITCH     32
#define MIDI_FILTER_SYSEX     64

extern int midiThruType; // transmit all events
extern int midiFilterCtrl1;
extern int midiFilterCtrl2;
extern int midiFilterCtrl3;
extern int midiFilterCtrl4;

#define CMD_RANGE_ALL         0
#define CMD_RANGE_SELECTED    1
#define CMD_RANGE_LOOP        2

extern QActionGroup* undoRedo;
extern QAction* undoAction;
extern QAction* redoAction;

extern QActionGroup* transportAction;
extern QAction* playAction;
extern QAction* startAction;
extern QAction* stopAction;
extern QAction* rewindAction;
extern QAction* forwardAction;
extern QAction* loopAction;
extern QAction* replayAction;
extern QAction* punchinAction;
extern QAction* punchoutAction;
extern QAction* recordAction;
extern QAction* panicAction;
extern QAction* feedbackAction;
extern QAction* pcloaderAction;
extern QAction* noteAlphaAction;
extern QAction* multiPartSelectionAction;
extern QAction* masterEnableAction;

class OOMidi;
extern OOMidi* oom;

extern int preMeasures;
extern unsigned char measureClickNote;
extern unsigned char measureClickVelo;
extern unsigned char beatClickNote;
extern unsigned char beatClickVelo;
extern unsigned char clickChan;
extern unsigned char clickPort;
extern bool precountEnableFlag;
extern bool precountFromMastertrackFlag;
extern int precountSigZ;
extern int precountSigN;
extern bool precountPrerecord;
extern bool precountPreroll;
extern bool midiClickFlag;
extern bool audioClickFlag;
extern float audioClickVolume;

extern bool rcEnable;
extern unsigned char rcStopNote;
extern unsigned char rcRecordNote;
extern unsigned char rcGotoLeftMarkNote;
extern unsigned char rcPlayNote;

extern int lastTrackPartColorIndex;

extern bool midiSeqRunning;
extern bool automation;

class QObject;
// Which audio strip, midi strip, or midi track info strip
//  was responsible for popping up the routing menu.
extern QObject* gRoutingPopupMenuMaster;
// Map of routing popup menu item IDs to Routes.
extern RouteMenuMap gRoutingMenuMap;
// Whether the routes popup was shown by clicking the output routes button, or input routes button.
extern bool gIsOutRoutingPopupMenu;

// p3.3.55
#define JACK_MIDI_OUT_PORT_SUFFIX "_out"
#define JACK_MIDI_IN_PORT_SUFFIX  "_in"

extern uid_t euid, ruid;
extern void doSetuid();
extern void undoSetuid();
extern bool checkAudioDevice();
extern QList<QIcon> partColorIcons;
extern QList<QIcon> partColorIconsSelected;
extern QHash<int, QColor> g_trackColorListLine;
extern QHash<int, QColor> g_trackColorList;
extern QHash<int, QColor> g_trackColorListSelected;
extern QHash<int, QPixmap> g_trackDragImageList;
extern int vuColorStrip;
extern bool lsClientStarted;
extern LSClient* lsClient;
extern bool gUpdateAuxes;
extern TrackManager* trackManager;
extern QList<QPair<int, QString> > gInputList;
extern QList<int> gInputListPorts;
extern LSThread* gLSThread;
extern bool gSamplerStarted;
#endif

