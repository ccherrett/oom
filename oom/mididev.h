//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: mididev.h,v 1.3.2.4 2009/04/04 01:49:50 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIDEV_H__
#define __MIDIDEV_H__

#include <list>

#include "mpevent.h"
//#include "sync.h"
#include "route.h"
#include "globaldefs.h"

#include <QString>
#include <QHash>

//class RouteList;
class Xml;

struct NRPNCache {
	int msb;
	int lsb;
	int data_msb;
	int data_lsb;
};

//---------------------------------------------------------
//   MidiDevice
//---------------------------------------------------------

class MidiDevice {
    MPEventList _stuckNotes;
    MPEventList _playEvents;

    // Used for multiple reads of fifos during process.
    int _tmpRecordCount[MIDI_CHANNELS + 1];
    bool _sysexFIFOProcessed;


protected:
    QString _name;
    int _port; // connected to midi port; -1 - not connected
    int _rwFlags; // possible open flags, 1 write, 2 read, 3 rw
    int _openFlags; // configured open flags
    bool _readEnable; // set when opened/closed.
    bool _writeEnable; //
    bool _sysexReadingChunks;

	MidiFifo eventFifo;
	bool m_cachenrpn;
	//NRPNCache m_nrpnCache;
	QHash<int, NRPNCache*> m_nrpnCache;

    // Recording fifos. To speed up processing, one per channel plus one special system 'channel' for channel-less events like sysex.
    MidiRecFifo _recordFifo[MIDI_CHANNELS + 1];

    RouteList _inRoutes, _outRoutes;

    void init();
    virtual bool putMidiEvent(const MidiPlayEvent&) = 0;
	virtual void monitorEvent(const MidiRecordEvent&);
	virtual void monitorOutputEvent(const MidiPlayEvent&);
	virtual void resetNRPNCache(int chan)
	{
		NRPNCache* c = m_nrpnCache.value(chan);
		if(c)
		{
			c->msb = -1;
			c->lsb = -1;
			c->data_msb = -1;
			c->data_lsb = -1;
		}
	}

public:

    enum {
        ALSA_MIDI = 0, JACK_MIDI = 1, SYNTH_MIDI = 2
    };

    MidiDevice();
    MidiDevice(const QString& name);

    virtual ~MidiDevice() {
    }

    virtual int deviceType() = 0;

    virtual void* inClientPort() {
        return 0;
    }

    virtual void* outClientPort() {
        return 0;
    }

    virtual QString open() = 0;
    virtual void close() = 0;

    virtual void writeRouting(int, Xml&) const {
    };

    RouteList* inRoutes() {
        return &_inRoutes;
    }

    RouteList* outRoutes() {
        return &_outRoutes;
    }

    bool noInRoute() const {
        return _inRoutes.empty();
    }

    bool noOutRoute() const {
        return _outRoutes.empty();
    }

    const QString& name() const {
        return _name;
    }

    virtual void setName(const QString& s) {
        _name = s;
    }

    int midiPort() const {
        return _port;
    }

    void setPort(int p) {
        _port = p;
    }

    int rwFlags() const {
        return _rwFlags;
    }

    int openFlags() const {
        return _openFlags;
    }

    void setOpenFlags(int val) {
        _openFlags = val;
    }

    void setrwFlags(int val) {
        _rwFlags = val;
    }

    virtual bool isSynthPlugin() const {
        return false;
    }

    virtual int selectRfd() {
        return -1;
    }

    virtual int selectWfd() {
        return -1;
    }

    virtual int bytesToWrite() {
        return 0;
    }

    virtual void flush() {
    }

    virtual void processInput() {
    }

    virtual void discardInput() {
    }

    virtual void recordEvent(MidiRecordEvent&);

    virtual bool putEvent(const MidiPlayEvent&);
	
    // For Jack-based devices - called in Jack audio process callback

    virtual void collectMidiEvents() {
    }

    virtual void processMidi() {
    }

    MPEventList* stuckNotes() {
        return &_stuckNotes;
    }

    MPEventList* playEvents() {
        return &_playEvents;
    }

    void beforeProcess();
    void afterProcess();

    int tmpRecordCount(const unsigned int ch) {
        return _tmpRecordCount[ch];
    }

    MidiRecFifo& recordEvents(const unsigned int ch) {
        return _recordFifo[ch];
    }

    bool sysexFIFOProcessed() {
        return _sysexFIFOProcessed;
    }

    void setSysexFIFOProcessed(bool v) {
        _sysexFIFOProcessed = v;
    }

    bool sysexReadingChunks() {
        return _sysexReadingChunks;
    }

    void setSysexReadingChunks(bool v) {
        _sysexReadingChunks = v;
    }

    bool sendNullRPNParams(int, bool);
	virtual bool cacheNRPN()
	{
		return m_cachenrpn;
	}
	virtual void setCacheNRPN(bool f)
	{
		m_cachenrpn = f;
	}

	bool hasNRPNIndex(int index) {
		return ((!m_nrpnCache.isEmpty() && m_nrpnCache.contains(index)) && m_nrpnCache.value(index)->msb >= 0 && m_nrpnCache.value(index)->lsb >= 0);
	}

	NRPNCache* rpnCache(int index)
	{
		return m_nrpnCache.value(index);;
	}
};

//---------------------------------------------------------
//   MidiDeviceList
//---------------------------------------------------------

typedef std::list<MidiDevice*>::iterator iMidiDevice;

class MidiDeviceList : public std::list<MidiDevice*> {
public:
    void add(MidiDevice* dev);
    void remove(MidiDevice* dev);
    MidiDevice* find(const QString& name, int typeHint = -1);
    iMidiDevice find(const MidiDevice* dev);
};

extern MidiDeviceList midiDevices;
extern void initMidiDevices();
extern bool filterEvent(const MEvent& event, int type, bool thru);

#endif

