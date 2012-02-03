//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: midiport.h,v 1.9.2.6 2009/11/17 22:08:22 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIPORT_H__
#define __MIDIPORT_H__

#include "globaldefs.h"
#include "sync.h"
#include "route.h"
#include <QHash>

class MidiDevice;
class MidiInstrument;
class MidiCtrlValListList;
class MidiPlayEvent;
class MidiController;
class MidiCtrlValList;
class Part;
//class MidiSyncInfo;

typedef struct pnode
{
    QString name;
    int id;
    bool selected;
} PatchSequence;

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

class MidiPort
{
	qint64 m_portId;
    MidiCtrlValListList* _controller;
    MidiDevice* _device;
    QString _state; // result of device open
    QList<PatchSequence*> _patchSequences;
    MidiInstrument* _instrument;
    AutomationType _automationType[MIDI_CHANNELS];
    // Holds sync settings and detection monitors.
    MidiSyncInfo _syncInfo;
    // p3.3.50 Just a flag to say the port was found in the song file, even if it has no device right now.
    bool _foundInSongFile;
    // When creating a new midi track, add these global default channel routes to/from this port. Ignored if 0.
    int _defaultInChannels; // These are bit-wise channel masks.
    int _defaultOutChannels; //

	QHash<int, QString>  m_presets;

    RouteList _inRoutes, _outRoutes;

    void clearDevice();

public:
    MidiPort();
    ~MidiPort();

    //
    // manipulate active midi controller
    //

    MidiCtrlValListList* controller()
    {
        return _controller;
    }
	qint64 id()
	{
		return m_portId;
	}
	//FIXME: To be removed when MidiPort::read is implemented
	void setPortId(qint64 id)
	{
		if(id)
			m_portId = id;
	}
    int getCtrl(int ch, int tick, int ctrl) const;
    int getCtrl(int ch, int tick, int ctrl, Part* part) const;
    // Removed by T356.
    //bool setCtrl(int ch, int tick, int ctrl, int val);
    bool setControllerVal(int ch, int tick, int ctrl, int val, Part* part);
    // Can be CTRL_VAL_UNKNOWN until a valid state is set
    int lastValidHWCtrlState(int ch, int ctrl) const;
    int hwCtrlState(int ch, int ctrl) const;
    bool setHwCtrlState(int ch, int ctrl, int val);
    bool setHwCtrlStates(int ch, int ctrl, int val, int lastval);
    void deleteController(int ch, int tick, int ctrl, Part* part);

    bool guiVisible() const;
    bool hasGui() const;

    int portno() const;

    bool foundInSongFile() const
    {
        return _foundInSongFile;
    }

    void setFoundInSongFile(bool b)
    {
        _foundInSongFile = b;
    }

    MidiDevice* device() const
    {
        return _device;
    }

    const QString& state() const
    {
        return _state;
    }

    void setState(const QString& s)
    {
        _state = s;
    }
    void setMidiDevice(MidiDevice* dev);
    const QString& portname() const;

    MidiInstrument* instrument() const
    {
        return _instrument;
    }

    void setInstrument(MidiInstrument* i);

    MidiController* midiController(int num) const;
    MidiCtrlValList* addManagedController(int channel, int ctrl);
    void tryCtrlInitVal(int chan, int ctl, int val);
    int limitValToInstrCtlRange(int ctl, int val);
    int limitValToInstrCtlRange(MidiController* mc, int val);
    MidiController* drumController(int ctl);
    int nullSendValue();
    void setNullSendValue(int v);
    void addPatchSequence(PatchSequence*);
    void appendPatchSequence(PatchSequence*);
    void deletePatchSequence(PatchSequence*);
    void insertPatchSequence(int, PatchSequence*);

    QList<PatchSequence*>* patchSequences()
    {
        return &_patchSequences;
    };

    int defaultInChannels() const
    {
        return _defaultInChannels;
    }

    int defaultOutChannels() const
    {
        return _defaultOutChannels;
    }

    void setDefaultInChannels(int c)
    {
        _defaultInChannels = c;
    }

    void setDefaultOutChannels(int c)
    {
        _defaultOutChannels = c;
    }

    RouteList* inRoutes()
    {
        return &_inRoutes;
    }

    RouteList* outRoutes()
    {
        return &_outRoutes;
    }

    bool noInRoute() const
    {
        return _inRoutes.empty();
    }

    bool noOutRoute() const
    {
        return _outRoutes.empty();
    }
    void writeRouting(int, Xml&) const;

    // send events to midi device and keep track of
    // device state:
    void sendGmOn();
    void sendGsOn();
    void sendXgOn();
    void sendGmInitValues();
    void sendGsInitValues();
    void sendXgInitValues();
    void sendStart();
    void sendStop();
    void sendContinue();
    void sendSongpos(int);
    void sendClock();
    void sendSysex(const unsigned char* p, int n);
    void sendMMCLocate(unsigned char ht, unsigned char m,
            unsigned char s, unsigned char f, unsigned char sf, int devid = -1);
    void sendMMCStop(int devid = -1);
    void sendMMCDeferredPlay(int devid = -1);

    //bool sendEvent(const MidiPlayEvent&);
	bool sendEvent(const MidiPlayEvent&, bool forceSend = false );

    AutomationType automationType(int channel)
    {
        return _automationType[channel];
    }

    void setAutomationType(int channel, AutomationType t)
    {
        _automationType[channel] = t;
    }

    MidiSyncInfo& syncInfo()
    {
        return _syncInfo;
    }

	void addPreset(int i, QString p) { m_presets.insert(i, p); }
	bool hasPreset(int i)
	{
		return (!m_presets.isEmpty() && m_presets.contains(i));
	}
	//Return the preset sysex value as QString, returns an empty QString if there is no such preset.
	QString preset(int i) { 
		QString def;
		if(!hasPreset(i))
			return def;
		return m_presets.value(i);
	}
	void removePreset(int i)
	{
		if(hasPreset(i))
		{
			m_presets.remove(i);
		}
	}
	QHash<int, QString> * presets() { return &m_presets; }
};

extern MidiPort midiPorts[MIDI_PORTS];
extern QHash<qint64, MidiPort*> oomMidiPorts;
extern void initMidiPorts();

class QMenu;
class QWidget;
//extern QPopupMenu* midiPortsPopup(QWidget*);
extern QMenu* midiPortsPopup(QWidget* parent = 0, int checkPort = -1);
#endif

