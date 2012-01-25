//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: waveevent.cpp,v 1.9.2.6 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2000-2003 Werner Schweer (ws@seh.de)
//=========================================================

#include "audioconvert.h"
#include "globals.h"
#include "event.h"
#include "waveevent.h"
#include "xml.h"
#include "wave.h"
#include <iostream>
#include <math.h>

// Added by Tim. p3.3.18
//#define USE_SAMPLERATE
//
//#define WAVEEVENT_DEBUG
//#define WAVEEVENT_DEBUG_PRC

//---------------------------------------------------------
//   WaveEvent
//---------------------------------------------------------

WaveEventBase::WaveEventBase(EventType t)
: EventBase(t)
{
	deleted = false;
}

//---------------------------------------------------------
//   WaveEventBase::clone
//---------------------------------------------------------

EventBase* WaveEventBase::clone()
{
	return new WaveEventBase(*this);
}

//---------------------------------------------------------
//   WaveEvent::mid
//---------------------------------------------------------

EventBase* WaveEventBase::mid(unsigned b, unsigned e)
{
	WaveEventBase* ev = new WaveEventBase(*this);
	unsigned fr = frame();
	unsigned start = fr - b;
	if (b > fr)
	{
		start = 0;
		ev->setSpos(spos() + b - fr);
	}
	unsigned end = endFrame();

	if (e < end)
		end = e;

	ev->setFrame(start);
	ev->setLenFrame(end - b - start);
	return ev;
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void WaveEventBase::dump(int n) const
{
	EventBase::dump(n);
}

//---------------------------------------------------------
//   WaveEventBase::read
//---------------------------------------------------------

void WaveEventBase::read(Xml& xml)
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
			case Xml::Attribut:
				return;
			case Xml::TagStart:
				if (tag == "poslen")
					PosLen::read(xml, "poslen");
				else if (tag == "frame")
					_spos = xml.parseInt();
				else if(tag == "leftclip")
					m_leftclip = xml.parseInt();
				else if(tag == "rightclip")
					m_rightclip = xml.parseInt();
				else if (tag == "file")
				{
					SndFile* wf = getWave(xml.parse1(), true);
					if (wf)
					{
						f = SndFileR(wf);
					}
				}
				else
					xml.unknown("Event");
				break;
			case Xml::TagEnd:
				if (tag == "event")
				{
					Pos::setType(FRAMES); // DEBUG
					return;
				}
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveEventBase::write(int level, Xml& xml, const Pos& offset, bool forcePath) const
{
	if (f.isNull())
		return;
	xml.tag(level++, "event");
	PosLen wpos(*this);
	wpos += offset;
	wpos.write(level, xml, "poslen");
	xml.intTag(level, "frame", _spos); // offset in wave file
	xml.intTag(level, "leftclip", m_leftclip);
	xml.intTag(level, "rightclip", m_rightclip);

	//
	// waves in the project dirctory are stored
	// with relative path name, others with absolute path
	//
	QString path = f.dirPath();

	//if (path.contains(oomProject)) {
	if (!forcePath && path.contains(oomProject))
	{
		// extract oomProject.
		QString newName = f.path().remove(oomProject + "/");
		xml.strTag(level, "file", newName);
	}
	else
		xml.strTag(level, "file", f.path());
    xml.etag(--level, "event");
}

void WaveEventBase::readAudio(WavePart* part, unsigned offset, float** buffer, int channel, int n, bool doSeek, bool overwrite)
{
#ifdef WAVEEVENT_DEBUG_PRC
	printf("WaveEventBase::readAudio audConv:%p sfCurFrame:%ld offset:%u channel:%d n:%d\n", audConv, sfCurFrame, offset, channel, n);
#endif

#ifdef USE_SAMPLERATE

	// TODO:
	>> >> >> >> >> >+++++++++++++++++++++++++++++
	// If we have a valid audio converter then use it to do the processing. Otherwise just a normal seek + read.
	if (audConv)
		sfCurFrame = audConv->readAudio(f, sfCurFrame, offset, buffer, channel, n, doSeek, overwrite);
	else
	{
		if (!f.isNull())
		{
			sfCurFrame = f.seek(offset + _spos, 0);
			sfCurFrame += f.read(channel, buffer, n, offset, overwrite, part);
		}
	}
	return;

#else
	if (f.isNull())
		return;

	f.seek(offset + _spos, 0);
	f.read(channel, buffer, n, offset, overwrite, part);

	return;
#endif

}

