//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: part.h,v 1.5.2.4 2009/05/24 21:43:44 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PART_H__
#define __PART_H__

#include <map>

#include <uuid/uuid.h>
#include <QList>

#include "event.h"
#include "audioconvert.h"
#include "FadeCurve.h"

class QString;

class Track;
class MidiTrack;
class WaveTrack;
class Xml;
class Part;


struct ClonePart
{
    const Part* cp;
    int id;
    uuid_t uuid;
    ClonePart(const Part*, int i = -1);
};

typedef std::list<ClonePart> CloneList;
typedef CloneList::iterator iClone;

//---------------------------------------------------------
//   Part
//---------------------------------------------------------

class Part : public PosLen
{
    static int snGen;
    int _sn;

    QString _name;
    bool _selected;
    bool _mute;
    int _colorIndex;

protected:
    Track* _track;
    EventList* _events;
    Part* _prevClone;
    Part* _nextClone;
	int m_zIndex;
 //currently on used for wavetrack
	unsigned int m_leftClip;
	unsigned int m_rightClip;

public:
    Part(Track*);
    Part(Track*, EventList*);
    virtual ~Part();

    int sn()
    {
        return _sn;
    }

    void setSn(int n)
    {
        _sn = n;
    }

    int newSn()
    {
        return snGen++;
    }

	void setLeftClip(unsigned int clip)
	{
		m_leftClip = clip;
	}
	unsigned int leftClip()
	{
		return m_leftClip;
	}

	void setRightClip(unsigned int clip)
	{
		m_rightClip = clip;
	}
	unsigned int rightClip()
	{
		return m_rightClip;
	}

    virtual Part* clone() const = 0;

    const QString& name() const
    {
        return _name;
    }

    void setName(const QString& s)
    {
        _name = s;
    }

    bool selected() const
    {
        return _selected;
    }

	void setSelected(bool f);

    bool mute() const
    {
        return _mute;
    }

    void setMute(bool b)
    {
        _mute = b;
    }

    Track* track() const
    {
        return _track;
    }

    void setTrack(Track*t)
    {
        _track = t;
    }

    EventList* events() const
    {
        return _events;
    }

    const EventList* cevents() const
    {
        return _events;
    }

    int colorIndex() const
    {
        return _colorIndex;
    }

    void setColorIndex(int idx)
    {
        _colorIndex = idx;
    }

    Part* prevClone()
    {
        return _prevClone;
    }

    Part* nextClone()
    {
        return _nextClone;
    }

    void setPrevClone(Part* p)
    {
        _prevClone = p;
    }

    void setNextClone(Part* p)
    {
        _nextClone = p;
    }

	void setZIndex(int i);
	int getZIndex()
	{	
		return m_zIndex;
	}
	static bool smallerZValue(Part* first, Part* second);

    iEvent addEvent(Event& p);

    virtual void write(int, Xml&, bool isCopy = false, bool forceWavePaths = false) const;

    virtual void dump(int n = 0) const;

};

//---------------------------------------------------------
//   MidiPart
//---------------------------------------------------------

class MidiPart : public Part
{
public:

    MidiPart(MidiTrack* t) : Part((Track*) t)
    {
    }

    MidiPart(MidiTrack* t, EventList* ev) : Part((Track*) t, ev)
    {
    }
    MidiPart(const MidiPart& p);

    virtual ~MidiPart()
    {
    }
    virtual MidiPart* clone() const;

    MidiTrack* track() const
    {
        return (MidiTrack*) Part::track();
    }

    virtual void dump(int n = 0) const;
};

//---------------------------------------------------------
//   WavePart
//---------------------------------------------------------

class WavePart : public Part
{
	AudioConvertMap _converters;
	FadeCurve *m_fadeIn;
	FadeCurve *m_fadeOut;
	FadeCurve *m_crossFadeIn;
	FadeCurve *m_crossFadeOut;
	bool m_hasCrossFadeForPartialOverlapLeft;
	bool m_hasCrossFadeForPartialOverlapRight;

	void init();

public:
    WavePart(WaveTrack* t);
    WavePart(WaveTrack* t, EventList* ev);
    WavePart(const WavePart& p);

    virtual ~WavePart()
    {
    }
    virtual WavePart* clone() const;

    WaveTrack* track() const
    {
        return (WaveTrack*) Part::track();
    }
	FadeCurve* fadeIn()
	{
		return m_fadeIn;
	}
	FadeCurve* fadeOut()
	{
		return m_fadeOut;
	}
	FadeCurve* crossFadeIn() { return m_crossFadeIn;}
	FadeCurve* crossFadeOut() { return m_crossFadeOut;}

	float gain(unsigned);
	float getFadeOutValue(unsigned pos, QList<FadeCurve*> fades);
	float getFadeInValue(unsigned pos, QList<FadeCurve*> fades);
	void setHasCrossFadeForPartialOverlapLeft(bool crossFade) {m_hasCrossFadeForPartialOverlapLeft = crossFade;}
	void setHasCrossFadeForPartialOverlapRight(bool crossFade) {m_hasCrossFadeForPartialOverlapRight = crossFade;}
	bool hasCrossFadeForPartialOverlapLeft() const {return m_hasCrossFadeForPartialOverlapLeft;}
	bool hasCrossFadeForPartialOverlapRight() const {return m_hasCrossFadeForPartialOverlapRight;}


    virtual void dump(int n = 0) const;

};


//---------------------------------------------------------
//   PartList
//---------------------------------------------------------
class PartList;

struct PartMap {
	PartList* parts;
	Track* track;
};

typedef std::multimap<int, Part*, std::less<unsigned> >::iterator iPart;
typedef std::multimap<int, Part*, std::less<unsigned> >::const_iterator ciPart;

class PartList : public std::multimap<int, Part*, std::less<unsigned> >
{
public:
    iPart findPart(unsigned tick);
    iPart add(Part*);
    void remove(Part* part);
    int index(Part*);
    Part* find(int idx);
    Part* find(unsigned tick, int sn);
    Part* findAtTick(unsigned tick);
	PartMap partMap(Track*);
	QList<Track*> tracks();
};

extern void chainClone(Part* p);
extern void chainClone(Part* p1, Part* p2);
extern void unchainClone(Part* p);
extern void replaceClone(Part* p1, Part* p2);
extern void chainCheckErr(Part* p);
extern void unchainTrackParts(Track* t, bool decRefCount);
extern void chainTrackParts(Track* t, bool incRefCount);
extern void addPortCtrlEvents(Part* part, bool doClones);
extern void addPortCtrlEvents(Event& event, Part* part, bool doClones);
extern void removePortCtrlEvents(Part* part, bool doClones);
extern void removePortCtrlEvents(Event& event, Part* part, bool doClones);
extern CloneList cloneList;
extern Part* readXmlPart(Xml&, Track*, bool doClone = false, bool toTrack = true);

#endif

