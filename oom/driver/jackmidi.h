//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: jackmidi.h,v 1.1.1.1 2010/01/27 09:06:43 terminator356 Exp $
//  (C) Copyright 1999-2010 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __JACKMIDI_H__
#define __JACKMIDI_H__

//#include <config.h>

#include <map>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "mididev.h"
#include "route.h"

class QString;
//class MidiFifo;
class MidiRecordEvent;
class MidiPlayEvent;
//class RouteList;
class Xml;

//---------------------------------------------------------
//   MidiJackDevice
//---------------------------------------------------------

class MidiJackDevice : public MidiDevice
{
public:
    //int adr;

private:
    // fifo for midi events sent from gui
    // direct to midi port:
    //MidiFifo eventFifo;

    //static int _nextOutIdNum;
    //static int _nextInIdNum;

    //jack_port_t* _client_jackport;
    // p3.3.55
    jack_port_t* _in_client_jackport;
    jack_port_t* _out_client_jackport;

    //RouteList _routes;

    virtual QString open();
    virtual void close();
    //bool putEvent(int*);

    bool processEvent(const MidiPlayEvent&);
    // Port is not midi port, it is the port(s) created for OOMidi.
    bool queueEvent(const MidiPlayEvent&);

    virtual bool putMidiEvent(const MidiPlayEvent&);
    //bool sendEvent(const MidiPlayEvent&);

    void eventReceived(jack_midi_event_t*);

public:
    //MidiJackDevice() {}  // p3.3.55  Removed.
    //MidiJackDevice(const int&, const QString& name);

    //MidiJackDevice(jack_port_t* jack_port, const QString& name);
    //MidiJackDevice(jack_port_t* in_jack_port, jack_port_t* out_jack_port, const QString& name); // p3.3.55 In or out port can be null.
    MidiJackDevice(const QString& name);

    //static MidiDevice* createJackMidiDevice(QString /*name*/, int /*rwflags*/); // 1:Writable 2: Readable. Do not mix.
    static MidiDevice* createJackMidiDevice(QString name = "", int rwflags = 3); // p3.3.55 1:Writable 2: Readable 3: Writable + Readable

    virtual inline int deviceType()
    {
        return JACK_MIDI;
    }

    virtual void setName(const QString&);

    virtual void processMidi();
    virtual ~MidiJackDevice();
    //virtual int selectRfd();
    //virtual int selectWfd();
    //virtual void processInput();

    virtual void recordEvent(MidiRecordEvent&);

    virtual bool putEvent(const MidiPlayEvent&);
    virtual void collectMidiEvents();

    //virtual jack_port_t* jackPort() { return _jackport; }
    //virtual jack_port_t* clientJackPort() { return _client_jackport; }

    //virtual void* clientPort() { return (void*)_client_jackport; }
    // p3.3.55

    virtual void* inClientPort()
    {
        return (void*) _in_client_jackport;
    }

    virtual void* outClientPort()
    {
        return (void*) _out_client_jackport;
    }

    //RouteList* routes()   { return &_routes; }
    //bool noRoute() const   { return _routes.empty();  }
    virtual void writeRouting(int, Xml&) const;
};

extern bool initMidiJack();
//extern int jackSelectRfd();
//extern int jackSelectWfd();
//extern void jackProcessMidiInput();
//extern void jackScanMidiPorts();

#endif


