//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: waveevent.h,v 1.6.2.4 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __WAVE_EVENT_H__
#define __WAVE_EVENT_H__

//#include <samplerate.h>
#include <sys/types.h>

#include "eventbase.h"

class AudioConverter;
class WavePart;

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

class WaveEventBase : public EventBase
{
    QString _name;
    SndFileR f;
    int _spos; // start sample position in WaveFile
    bool deleted;

    // p3.3.31
    //virtual EventBase* clone() { return new WaveEventBase(*this); }
    virtual EventBase* clone();

public:
    WaveEventBase(EventType t);

    virtual ~WaveEventBase()
    {
    }

    virtual void read(Xml&);
    //virtual void write(int, Xml&, const Pos& offset) const;
    virtual void write(int, Xml&, const Pos& offset, bool forcePath = false) const;
    virtual EventBase* mid(unsigned, unsigned);

    virtual void dump(int n = 0) const;

    virtual const QString name() const
    {
        return _name;
    }

    virtual void setName(const QString& s)
    {
        _name = s;
    }

    virtual int spos() const
    {
        return _spos;
    }

    virtual void setSpos(int s)
    {
        _spos = s;
    }

	virtual int rightClip() const
	{
		return m_rightclip;
	}
	virtual void setRightClip(int clip)
	{
		m_rightclip = clip;
	}
	virtual int leftClip() const
	{
		return m_leftclip;
	}
	virtual void setLeftClip(int clip)
	{
		m_leftclip = clip;
	}

    virtual SndFileR sndFile() const
    {
        return f;
    }

    virtual void setSndFile(SndFileR& sf)
    {
        f = sf;
    }

    virtual void readAudio(WavePart* /*part*/, unsigned /*offset*/,
            float** /*bpp*/, int /*channels*/, int /*nn*/, bool /*doSeek*/, bool /*overwrite*/);
};

#endif

