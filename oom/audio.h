//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: audio.h,v 1.25.2.13 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "thread.h"
#include "pos.h"
#include "mpevent.h"
#include "route.h"
#include "event.h"
#include <QList>

class SndFile;
class BasePlugin;
class SynthI;
class MidiDevice;
class AudioDevice;
class Track;
class AudioTrack;
class Part;
class Event;
class MidiPlayEvent;
class Event;
class MidiPort;
class EventList;
class MidiInstrument;
class MidiTrack;

//---------------------------------------------------------
//   AudioMsgId
//    this are the messages send from the GUI thread to
//    the midi thread
//---------------------------------------------------------

enum
{
    SEQM_ADD_TRACK, SEQM_REMOVE_TRACK, SEQM_CHANGE_TRACK, SEQM_MOVE_TRACK,
    SEQM_ADD_PART, SEQM_REMOVE_PART, SEQM_REMOVE_PART_LIST, SEQM_CHANGE_PART,
    SEQM_ADD_EVENT, SEQM_ADD_EVENT_CHECK, SEQM_REMOVE_EVENT, SEQM_CHANGE_EVENT,
    SEQM_ADD_TEMPO, SEQM_SET_TEMPO, SEQM_REMOVE_TEMPO, SEQM_REMOVE_TEMPO_RANGE, SEQM_ADD_SIG, SEQM_REMOVE_SIG,
    SEQM_SET_GLOBAL_TEMPO,
    SEQM_UNDO, SEQM_REDO,
    SEQM_RESET_DEVICES, SEQM_INIT_DEVICES, SEQM_PANIC,
    SEQM_MIDI_LOCAL_OFF,
    SEQM_SET_MIDI_DEVICE,
    SEQM_PLAY_MIDI_EVENT,
    SEQM_SET_HW_CTRL_STATE,
    SEQM_SET_HW_CTRL_STATES,
    SEQM_SET_TRACK_OUT_PORT,
    SEQM_SET_TRACK_OUT_CHAN,
    SEQM_REMAP_PORT_DRUM_CTL_EVS,
    SEQM_CHANGE_ALL_PORT_DRUM_CTL_EVS,
    SEQM_SCAN_ALSA_MIDI_PORTS,
    SEQM_SET_AUX,
    SEQM_UPDATE_SOLO_STATES,
    MIDI_SHOW_INSTR_GUI,
    MIDI_SHOW_INSTR_NATIVE_GUI,
    AUDIO_RECORD,
    AUDIO_ROUTEADD, AUDIO_ROUTEREMOVE, AUDIO_REMOVEROUTES,
    AUDIO_VOL, AUDIO_PAN,
    AUDIO_ADDPLUGIN,
    AUDIO_IDLEPLUGIN,
    AUDIO_SET_SEG_SIZE,
    AUDIO_SET_PREFADER, AUDIO_SET_CHANNELS,
    AUDIO_SET_PLUGIN_CTRL_VAL,
    AUDIO_SWAP_CONTROLLER_IDX,
    AUDIO_CLEAR_CONTROLLER_EVENTS,
    AUDIO_SEEK_PREV_AC_EVENT,
    AUDIO_SEEK_NEXT_AC_EVENT,
    AUDIO_ERASE_AC_EVENT,
    AUDIO_ERASE_RANGE_AC_EVENTS,
    AUDIO_ADD_AC_EVENT,
    AUDIO_SET_SOLO, AUDIO_SET_SEND_METRONOME,
    MS_PROCESS, MS_STOP, MS_SET_RTC, MS_UPDATE_POLL_FD,
    SEQM_IDLE, SEQM_SEEK, SEQM_PRELOAD_PROGRAM, SEQM_REMOVE_TRACK_GROUP
};

extern const char* seqMsgList[]; // for debug

//---------------------------------------------------------
//   Msg
//---------------------------------------------------------

struct AudioMsg : public ThreadMsg
{ // this should be an union
    int serialNo;
	qint64 sid;
    SndFile* downmix;
    AudioTrack* snode;
    AudioTrack* dnode;
    Route sroute, droute;
    AudioDevice* device;
    int ival;
    int iival;
    double dval;
    BasePlugin* plugin;
    SynthI* synth;
    Part* spart;
    Part* dpart;
    Track* track;

    const void *p1, *p2, *p3;
    Event ev1, ev2;
    char port, channel, ctrl;
    int a, b, c;
    Pos pos;
	QList<qint64> list;
	QList<Part*> plist;
	QList<void*> objectList;
};

//---------------------------------------------------------
//  Struct for controll preload processing
//---------------------------------------------------------
struct ProcessList {
	int port;
	int channel;
	int dataB;
};

class AudioOutput;

//---------------------------------------------------------
//   Audio
//---------------------------------------------------------

class Audio
{
public:

    enum State
    {
        STOP, START_PLAY, PLAY, LOOP1, LOOP2, SYNC, PRECOUNT
    };

private:
    bool _running; // audio is active
    bool recording; // recording is active
    bool idle; // do nothing in idle mode
    bool _freewheel;
    bool _bounce;
    //bool loopPassed;
    unsigned _loopFrame; // Startframe of loop if in LOOP mode. Not quite the same as left marker !
    int _loopCount; // Number of times we have looped so far

    Pos _pos; // current play position

    unsigned curTickPos; // pos at start of frame during play/record
    unsigned nextTickPos; // pos at start of next frame during play/record

    //metronome values
    unsigned midiClick;
    int clickno; // precount values
    int clicksMeasure;
    int ticksBeat;

    double syncTime; // wall clock at last sync point
    unsigned syncFrame; // corresponding frame no. to syncTime
    int frameOffset; // offset to free running hw frame counter

    State state;

    AudioMsg* msg;
    int fromThreadFdw, fromThreadFdr; // message pipe

    int sigFd; // pipe fd for messages to gui

    // record values:
    Pos startRecordPos;
    Pos endRecordPos;

    //
    AudioOutput* _audioMaster;
    AudioOutput* _audioMonitor;

    void sendLocalOff();
    bool filterEvent(const MidiPlayEvent* event, int type, bool thru);

    void startRolling();
    void stopRolling();

    void panic();
    void processMsg(AudioMsg* msg);
    void process1(unsigned samplePos, unsigned offset, unsigned samples);

    void collectEvents(MidiTrack*, unsigned int startTick, unsigned int endTick);

public:
    Audio();

    virtual ~Audio()
    {
    }

    void process(unsigned frames);
    bool sync(int state, unsigned frame);
    void shutdown();
    void writeTick();

    // transport:
    bool start();
    void stop(bool);
    void seek(const Pos& pos);

    bool isPlaying() const
    {
        return state == PLAY || state == LOOP1 || state == LOOP2;
    }

    bool isRecording() const
    {
        return state == PLAY && recording;
    }

    void setRunning(bool val)
    {
        _running = val;
    }

    bool isRunning() const
    {
        return _running;
    }

    //-----------------------------------------
    //   message interface
    //-----------------------------------------

    void msgSeek(const Pos&);
    void msgPlay(bool val);

    void msgRemoveTrack(Track*, bool u = true);
    void msgRemoveTracks();
	void msgRemoveTrackGroup(QList<qint64>, bool undo = true);
    void msgChangeTrack(Track* oldTrack, Track* newTrack, bool u = true);
    void msgMoveTrack(int idx1, int dx2, bool u = true);
    void msgAddPart(Part*, bool u = true);
    void msgRemovePart(Part*, bool u = true);
    void msgRemoveParts(QList<Part*>, bool u = true);
    void msgChangePart(Part* oldPart, Part* newPart, bool u = true, bool doCtrls = true, bool doClones = false);
    //void msgAddEvent(Event&, Part*, bool u = true);
    void msgAddEvent(Event&, Part*, bool u = true, bool doCtrls = true, bool doClones = false, bool waitRead = true);
    void msgAddEventCheck(Track*, Event&, bool u = true, bool doCtrls = true, bool doClones = false, bool waitRead = true);
    //void msgDeleteEvent(Event&, Part*, bool u = true);
    void msgDeleteEvent(Event&, Part*, bool u = true, bool doCtrls = true, bool doClones = false, bool waitRead = true);
    //void msgChangeEvent(Event&, Event&, Part*, bool u = true);
    void msgChangeEvent(Event&, Event&, Part*, bool u = true, bool doCtrls = true, bool doClones = false, bool waitRead = true);
    void msgScanAlsaMidiPorts();
    void msgAddTempo(int tick, int tempo, bool doUndoFlag = true);
    void msgSetTempo(int tick, int tempo, bool doUndoFlag = true);
    void msgUpdateSoloStates();
    void msgSetAux(AudioTrack*, qint64, double);
    void msgSetGlobalTempo(int val);
    void msgDeleteTempo(int tick, int tempo, bool doUndoFlag = true);
    void msgDeleteTempoRange(QList<void*> tempo, bool doUndoFlag = true);
    void msgAddSig(int tick, int z, int n, bool doUndoFlag = true);
    void msgRemoveSig(int tick, int z, int n, bool doUndoFlag = true);
    void msgShowInstrumentGui(MidiInstrument*, bool);
    void msgShowInstrumentNativeGui(MidiInstrument*, bool);
    void msgPanic();
    void sendMsg(AudioMsg*, bool waitRead = true);
    bool sendMessage(AudioMsg* m, bool doUndo, bool waitRead = true);
    void msgRemoveRoute(Route, Route);
    void msgRemoveRoute1(Route, Route);
    void msgRemoveRoutes(Route, Route); // p3.3.55
    void msgRemoveRoutes1(Route, Route); // p3.3.55
    void msgAddRoute(Route, Route);
    void msgAddRoute1(Route, Route);
    void msgAddPlugin(AudioTrack*, int idx, BasePlugin* plugin);
    void msgIdlePlugin(AudioTrack*, BasePlugin* plugin);
    void msgSetMute(AudioTrack*, bool val);
    void msgSetVolume(AudioTrack*, double val);
    void msgSetPan(AudioTrack*, double val);
    void msgAddSynthI(SynthI* synth);
    void msgRemoveSynthI(SynthI* synth);
    void msgSetSegSize(int, int);
    void msgSetPrefader(AudioTrack*, int);
    void msgSetChannels(AudioTrack*, int);
    void msgSetOff(AudioTrack*, bool);
    void msgSetRecord(AudioTrack*, bool);
    void msgUndo();
    void msgRedo();
    void msgLocalOff();
    void msgInitMidiDevices();
    void msgResetMidiDevices();
    void msgIdle(bool);
    void msgBounce();
    //void msgSetPluginCtrlVal(BasePlugin* /*plugin*/, int /*param*/, double /*val*/);
    void msgSetPluginCtrlVal(AudioTrack*, int /*param*/, double /*val*/, bool waitRead = true);
    void msgSwapControllerIDX(AudioTrack*, int, int);
    void msgClearControllerEvents(AudioTrack*, int);
    void msgSeekPrevACEvent(AudioTrack*, int);
    void msgSeekNextACEvent(AudioTrack*, int);
    void msgEraseACEvent(AudioTrack*, int, int);
    void msgEraseRangeACEvents(AudioTrack*, int, int, int);
    void msgAddACEvent(AudioTrack*, int, int, double);
    void msgSetSolo(Track*, bool);
    void msgSetHwCtrlState(MidiPort*, int, int, int);
    void msgSetHwCtrlStates(MidiPort*, int, int, int, int);
    void msgSetTrackOutChannel(MidiTrack*, int);
    void msgSetTrackOutPort(MidiTrack*, int);
    void msgRemapPortDrumCtlEvents(int, int, int, int);
    void msgChangeAllPortDrumCtrlEvents(bool, bool);
    void msgSetSendMetronome(AudioTrack*, bool);

    void msgPlayMidiEvent(const MidiPlayEvent* event);
    void msgPreloadCtrl();
    void rescanAlsaPorts();

    void midiPortsChanged();

    const Pos& pos() const
    {
        return _pos;
    }

    const Pos& getStartRecordPos() const
    {
        return startRecordPos;
    }

    const Pos& getEndRecordPos() const
    {
        return endRecordPos;
    }

    int loopCount()
    {
        return _loopCount;
    } // Number of times we have looped so far

    unsigned loopFrame()
    {
        return _loopFrame;
    }

    int tickPos() const
    {
        return curTickPos;
    }
    int timestamp() const;
    void processMidi();
    unsigned curFrame() const;
    void recordStop();
    void preloadControllers();

    bool freewheel() const
    {
        return _freewheel;
    }
    void setFreewheel(bool val);

    int getFrameOffset() const
    {
        return frameOffset;
    }
    void initDevices();

    AudioOutput* audioMaster() const
    {
        return _audioMaster;
    }

    AudioOutput* audioMonitor() const
    {
        return _audioMonitor;
    }

    void setMaster(AudioOutput* track)
    {
        _audioMaster = track;
    }

    void setMonitor(AudioOutput* track)
    {
        _audioMonitor = track;
    }
    void sendMsgToGui(char c);

    bool bounce() const
    {
        return _bounce;
    }
};

extern int processAudio(unsigned long, void*);
extern void processAudio1(void*, void*);

extern Audio* audio;
extern AudioDevice* audioDevice; // current audio device in use
#endif

