//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//  (C) Copyright 2012 Filipe Coelho (falktx@openoctave.org)
//=========================================================

#include "track.h"
#include "mididev.h"
#include "instruments/minstrument.h"

class Xml;

class SynthTrack :
        public AudioTrack,
        public MidiDevice,
        public MidiInstrument
{
public:
    SynthTrack() : AudioTrack(AUDIO_SOFTSYNTH)
    {
    }

    SynthTrack* clone(bool /*cloneParts*/) const
    {
        return new SynthTrack(*this);
    }

    virtual SynthTrack* newTrack() const
    {
        return new SynthTrack();
    }

    virtual void read(Xml&);
    virtual void write(int, Xml&) const;

    virtual bool hasAuxSend() const
    {
        return true;
    }

    virtual bool isMute() const
    {
        return _mute;
    }

    virtual inline int deviceType()
    {
        return SYNTH_MIDI;
    }

    virtual bool putMidiEvent(const MidiPlayEvent&)
    {
        return true;
    }

private:
    virtual QString open();
    virtual void close();
};
