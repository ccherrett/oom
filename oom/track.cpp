//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: track.cpp,v 1.34.2.11 2009/11/30 05:05:49 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QStringList>
#include "track.h"
#include "event.h"
#include "utils.h"
#include "midi.h"
#include "midictrl.h"
#include "mididev.h"
#include "midiport.h"
#include "song.h"
#include "xml.h"
#include "plugin.h"
#include "drummap.h"
#include "audio.h"
#include "globaldefs.h"
#include "route.h"
#include "midimonitor.h"
#include "ccinfo.h"

unsigned int Track::_soloRefCnt = 0;
Track* Track::_tmpSoloChainTrack = 0;
bool Track::_tmpSoloChainDoIns = false;
bool Track::_tmpSoloChainNoDec = false;

const char* Track::_cname[] = {
	"Midi", "Drum", "Wave", "AudioOut", "AudioIn", "AudioBuss",
	"AudioAux", "AudioSynth"
};

//---------------------------------------------------------
//   addPortCtrlEvents
//---------------------------------------------------------

void addPortCtrlEvents(MidiTrack* t)
{
	const PartList* pl = t->cparts();
	for (ciPart ip = pl->begin(); ip != pl->end(); ++ip)
	{
		Part* part = ip->second;
		const EventList* el = part->cevents();
		unsigned len = part->lenTick();
		for (ciEvent ie = el->begin(); ie != el->end(); ++ie)
		{
			const Event& ev = ie->second;
			// Added by T356. Do not add events which are past the end of the part.
			if (ev.tick() >= len)
				break;

			if (ev.type() == Controller)
			{
				int tick = ev.tick() + part->tick();
				int cntrl = ev.dataA();
				int val = ev.dataB();
				int ch = t->outChannel();

				MidiPort* mp = &midiPorts[t->outPort()];
				// Is it a drum controller event, according to the track port's instrument?
				if (t->type() == Track::DRUM)
				{
					MidiController* mc = mp->drumController(cntrl);
					if (mc)
					{
						int note = cntrl & 0x7f;
						cntrl &= ~0xff;
						ch = drumMap[note].channel;
						mp = &midiPorts[drumMap[note].port];
						cntrl |= drumMap[note].anote;
					}
				}

				mp->setControllerVal(ch, tick, cntrl, val, part);
			}
		}
	}
}

//---------------------------------------------------------
//   removePortCtrlEvents
//---------------------------------------------------------

void removePortCtrlEvents(MidiTrack* t)
{
	const PartList* pl = t->cparts();
	for (ciPart ip = pl->begin(); ip != pl->end(); ++ip)
	{
		Part* part = ip->second;
		const EventList* el = part->cevents();
		//unsigned len = part->lenTick();
		for (ciEvent ie = el->begin(); ie != el->end(); ++ie)
		{
			const Event& ev = ie->second;
			// Added by T356. Do not remove events which are past the end of the part.
			// No, actually, do remove ALL of them belonging to the part.
			// Just in case there are stray values left after the part end.
			//if(ev.tick() >= len)
			//  break;

			if (ev.type() == Controller)
			{
				int tick = ev.tick() + part->tick();
				int cntrl = ev.dataA();
				int ch = t->outChannel();

				MidiPort* mp = &midiPorts[t->outPort()];
				// Is it a drum controller event, according to the track port's instrument?
				if (t->type() == Track::DRUM)
				{
					MidiController* mc = mp->drumController(cntrl);
					if (mc)
					{
						int note = cntrl & 0x7f;
						cntrl &= ~0xff;
						ch = drumMap[note].channel;
						mp = &midiPorts[drumMap[note].port];
						cntrl |= drumMap[note].anote;
					}
				}

				mp->deleteController(ch, tick, cntrl, part);
			}
		}
	}
}

//---------------------------------------------------------
//   y
//---------------------------------------------------------

int Track::y() const
{
	TrackList* tl = song->visibletracks();
	int yy = 0;
	for (ciTrack it = tl->begin(); it != tl->end(); ++it)
	{
		if (this == *it)
			return yy;
		yy += (*it)->height();
	}
	printf("Track::y(%s): track not in tracklist\n", name().toLatin1().constData());
	return -1;
}

//---------------------------------------------------------
//   Track::init
//---------------------------------------------------------

void Track::init()
{
	_partDefaultColor = 1;
	_activity = 0;
	_lastActivity = 0;
	_recordFlag = false;
	_mute = false;
	_solo = false;
	_internalSolo = 0;
	_off = false;
	_channels = 0; // 1 - mono, 2 - stereo
	_reminder1 = false;
	_reminder2 = false;
	_reminder3 = false;
	_collapsed = true;
	_mixerTab = 0;
	m_maxZIndex = 0;

	_volumeEnCtrl = true;
	_volumeEn2Ctrl = true;
	_panEnCtrl = true;
	_panEn2Ctrl = true;
	m_chainMaster = 0;
	m_masterFlag = false;
    _wantsAutomation = false;

	_selected = false;
	_height = DEFAULT_TRACKHEIGHT;
	_locked = false;
	for (int i = 0; i < MAX_CHANNELS; ++i)
	{
		_meter[i] = 0.0;
		_peak[i] = 0.0;
	}
	m_midiassign.enabled = false;
	m_midiassign.port = 0;
	m_midiassign.preset = 0;
	m_midiassign.channel = 0;
	m_midiassign.track = this;
	m_midiassign.midimap.clear();
	switch(_type)
	{
		case AUDIO_INPUT:
		case AUDIO_BUSS:
			m_midiassign.midimap.insert(CTRL_AUX1, new CCInfo(this, 0, 0, CTRL_AUX1, -1));
			m_midiassign.midimap.insert(CTRL_AUX2, new CCInfo(this, 0, 0, CTRL_AUX2, -1));
		break;
		case AUDIO_SOFTSYNTH:
		case AUDIO_AUX:
		break;
		case Track::WAVE:
			m_midiassign.midimap.insert(CTRL_AUX1, new CCInfo(this, 0, 0, CTRL_AUX1, -1));
			m_midiassign.midimap.insert(CTRL_AUX2, new CCInfo(this, 0, 0, CTRL_AUX2, -1));
			m_midiassign.midimap.insert(CTRL_RECORD, new CCInfo(this, 0, 0, CTRL_RECORD, -1));
		break;
		default:
			m_midiassign.midimap.insert(CTRL_RECORD, new CCInfo(this, 0, 0, CTRL_RECORD, -1));
		break;
	}
	m_midiassign.midimap.insert(CTRL_MUTE, new CCInfo(this, 0, 0, CTRL_MUTE, -1));
	m_midiassign.midimap.insert(CTRL_SOLO, new CCInfo(this, 0, 0, CTRL_SOLO, -1));
	m_midiassign.midimap.insert(CTRL_VOLUME, new CCInfo(this, 0, 0, CTRL_VOLUME, -1));
	m_midiassign.midimap.insert(CTRL_PANPOT, new CCInfo(this, 0, 0, CTRL_PANPOT, -1));
	/*if(isMidiTrack())
	{
		m_midiassign.midimap.insert(CTRL_REVERB_SEND, new CCInfo(this, 0, 0, CTRL_REVERB_SEND, -1));
		m_midiassign.midimap.insert(CTRL_CHORUS_SEND, new CCInfo(this, 0, 0, CTRL_CHORUS_SEND, -1));
		m_midiassign.midimap.insert(CTRL_VARIATION_SEND, new CCInfo(this, 0, 0, CTRL_VARIATION_SEND, -1));
	}*/
}

Track::Track(Track::TrackType t)
{
	_type = t;
	m_id = create_id();
	init();
}

Track::Track(const Track& t, bool cloneParts)
{
	m_id = t.m_id;
	m_chainMaster = t.m_chainMaster;
	m_masterFlag = t.m_masterFlag;
	m_audioChain = t.m_audioChain;
	_partDefaultColor = t._partDefaultColor;
	_activity = t._activity;
	_lastActivity = t._lastActivity;
	_recordFlag = t._recordFlag;
	_mute = t._mute;
	_solo = t._solo;
	_internalSolo = t._internalSolo;
	_off = t._off;
	_channels = t._channels;

	_volumeEnCtrl = t._volumeEnCtrl;
	_volumeEn2Ctrl = t._volumeEn2Ctrl;
	_panEnCtrl = t._panEnCtrl;
	_panEn2Ctrl = t._panEn2Ctrl;

	_selected = t.selected();
	_y = t._y;
	_height = t._height;
	_comment = t.comment();
	_name = t.name();
	_type = t.type();
	_locked = t.locked();
	_mixerTab = t._mixerTab;
	m_midiassign = t.m_midiassign;
	m_maxZIndex = t.m_maxZIndex;
	_collapsed = t._collapsed;
	_reminder1 = t._reminder1;
	_reminder2 = t._reminder2;
	_reminder3 = t._reminder3;
    _wantsAutomation = t._wantsAutomation;

	if (cloneParts)
	{
		const PartList* pl = t.cparts();
		for (ciPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			Part* newPart = ip->second->clone();
			newPart->setTrack(this);
			_parts.add(newPart);
		}
	}
	else
	{
		_parts = *(t.cparts());
		// NOTE: We can't do this because of the way clipboard, cloneList, and undoOp::ModifyTrack, work.
		// A couple of schemes were conceived to deal with cloneList being invalid, but the best way is
		//  to not alter the part list here. It's a big headache because: Either the parts in the cloneList
		//  need to be reliably looked up replaced with the new ones, or the clipboard and cloneList must be cleared.
		// Fortunately the ONLY part of oom using this function is track rename (in TrackList and TrackInfo).
		// So we can get away with leaving this out:
		//for (iPart ip = _parts.begin(); ip != _parts.end(); ++ip)
		//      ip->second->setTrack(this);
	}

	for (int i = 0; i < MAX_CHANNELS; ++i)
	{
		//_meter[i] = 0;
		//_peak[i]  = 0;
		_meter[i] = 0.0;
		_peak[i] = 0.0;
	}
}

//---------------------------------------------------------
//   operator =
//   Added by Tim. Parts' track members MUST point to this track, 
//    not some other track, so simple assignment operator won't do!
//---------------------------------------------------------

Track& Track::operator=(const Track& t)
{
	m_id = t.m_id;
	m_chainMaster = t.m_chainMaster;
	m_masterFlag = t.m_masterFlag;
	m_audioChain = t.m_audioChain;
	_partDefaultColor = t._partDefaultColor;
	_activity = t._activity;
	_lastActivity = t._lastActivity;
	_recordFlag = t._recordFlag;
	_mute = t._mute;
	_solo = t._solo;
	_internalSolo = t._internalSolo;
	_off = t._off;
	_channels = t._channels;

	_volumeEnCtrl = t._volumeEnCtrl;
	_volumeEn2Ctrl = t._volumeEn2Ctrl;
	_panEnCtrl = t._panEnCtrl;
	_panEn2Ctrl = t._panEn2Ctrl;

	_selected = t.selected();
	_y = t._y;
	_height = t._height;
	_comment = t.comment();
	_name = t.name();
	_type = t.type();
	_locked = t.locked();
	_collapsed = t._collapsed;
	m_maxZIndex = t.m_maxZIndex;
    _wantsAutomation = t._wantsAutomation;

	_parts = *(t.cparts());

	for (int i = 0; i < MAX_CHANNELS; ++i)
	{
		_meter[i] = t._meter[i];
		_peak[i] = t._peak[i];
	}
	return *this;
}

//---------------------------------------------------------
//   setDefaultName
//    generate unique name for track
//---------------------------------------------------------

void Track::setDefaultName()
{
	QString base;
	switch (_type)/*{{{*/
	{
		case MIDI:
			base = QString("Midi");
			break;
		case DRUM:
			base = QString("Drum");
			break;
		case WAVE:
			base = QString("Audio");
			break;
		case AUDIO_OUTPUT:
			base = QString("Out");
			break;
		case AUDIO_BUSS:
			base = QString("Buss");
			break;
		case AUDIO_AUX:
			base = QString("Aux");
			break;
		case AUDIO_INPUT:
			base = QString("Input");
			break;
		case AUDIO_SOFTSYNTH:
			base = QString("Synth");
			break;
	};/*}}}*/
	setName(getValidName(base, true));
}

QString Track::getValidName(QString base, bool dname)
{
	QString rv;
	if(!dname)
	{
		Track* track = song->findTrack(base);
		if(!track)
			rv = base;
	}
	else
	{
		base += " ";
	}
	if(rv != base)
	{
		for (int i = 1; true; ++i)
		{
			QString n;
			n.setNum(i);
			QString s = base + n;
			Track* track = song->findTrack(s);
			if (track == 0)
			{
				rv = s;
				break;
			}
		}
	}
	return rv;
}

Track* Track::inputTrack()
{
	Track* in = 0;
	foreach(qint64 i, m_audioChain)
	{
		in = song->findTrackByIdAndType(i, AUDIO_INPUT);
		if(in)
		{
			break;
		}
	}
	return in;
}

void Track::deselectParts()
{
	for (iPart ip = parts()->begin(); ip != parts()->end(); ++ip)
	{
		Part* p = ip->second;
		p->setSelected(false);
	}
}

void Track::setSelected(bool sel)
{
	_selected = sel;
	if(sel && m_midiassign.enabled && m_midiassign.preset && midiMonitor->isFeedbackEnabled())
	{
		MidiPort* mport = &midiPorts[m_midiassign.port];
		if(mport)
		{
			//const unsigned char* preset = stringToSysex(mport->preset(m_midiassign.preset));
			const char* src = mport->preset(m_midiassign.preset).toLatin1().constData();

			int len;
			int status;
			unsigned char* sysex = (unsigned char*) hex2string(src, len, status);
			if(sysex && !status)
			{
				//Send the event from the sequencer thread
				MidiPlayEvent ev(0, m_midiassign.port, ME_SYSEX, sysex, len, this);
				audio->msgPlayMidiEvent(&ev);

				//Send adjustments to all the mixer based dials assigned to this track
				QHashIterator<int, CCInfo*> iter(m_midiassign.midimap);
				while(iter.hasNext())
				{
					iter.next();
					CCInfo* info = iter.value();
					if(info && info->assignedControl() >= 0)
					{
						switch(iter.key())
						{
							case CTRL_RECORD:
								midiMonitor->msgSendMidiOutputEvent((Track*)this, CTRL_RECORD, _recordFlag ? 127 : 0);
							break;
							case CTRL_MUTE:
								midiMonitor->msgSendMidiOutputEvent((Track*)this, CTRL_MUTE, mute() ? 127 : 0);
							break;
							case CTRL_SOLO:
								midiMonitor->msgSendMidiOutputEvent((Track*)this, CTRL_SOLO, solo() ? 127 : 0);
							break;
							case CTRL_VOLUME:
							{
								if(isMidiTrack())
								{
									MidiPort* mp = &midiPorts[((MidiTrack*)this)->outPort()];
									if(mp)
									{
										MidiController* mc = mp->midiController(CTRL_VOLUME);
										if(mc)
										{
											int chan = ((MidiTrack*)this)->outChannel();
											//int mn = mc->minVal();
											//int max = mc->maxVal();
											int v = mp->hwCtrlState(chan, CTRL_VOLUME);
											if (v == CTRL_VAL_UNKNOWN)
											{
												int lastv = mp->lastValidHWCtrlState(chan, CTRL_VOLUME);
												if (lastv == CTRL_VAL_UNKNOWN)
												{
													if (mc->initVal() == CTRL_VAL_UNKNOWN)
														v = 0;
													else
														v = mc->initVal();
												}
												else
													v = lastv;
											}
											midiMonitor->msgSendMidiOutputEvent((Track*)this, iter.key(), v);
										}
									}
								}
								else
								{
									midiMonitor->msgSendAudioOutputEvent((Track*)this, iter.key(), ((AudioTrack*)this)->volume());
								}
							}
							break;
							case CTRL_PANPOT:
							{
								if(isMidiTrack())
								{
									MidiPort* mp = &midiPorts[((MidiTrack*)this)->outPort()];
									if(mp)
									{
										MidiController* mc = mp->midiController(CTRL_PANPOT);
										if(mc)
										{
											int chan = ((MidiTrack*)this)->outChannel();
											//int mn = mc->minVal();
											//int max = mc->maxVal();
											int v = mp->hwCtrlState(chan, CTRL_PANPOT);
											if (v == CTRL_VAL_UNKNOWN)
											{
												int lastv = mp->lastValidHWCtrlState(chan, CTRL_PANPOT);
												if (lastv == CTRL_VAL_UNKNOWN)
												{
													//Center it
													v = 64;
												}
											}
											midiMonitor->msgSendMidiOutputEvent((Track*)this, iter.key(), v);
										}
									}
								}
								else
								{
									midiMonitor->msgSendAudioOutputEvent((Track*)this, iter.key(), ((AudioTrack*)this)->pan());
								}
							}
							break;
						}
					}
				}
			}
		}
	}
}

//---------------------------------------------------------
//   clearRecAutomation
//---------------------------------------------------------

void Track::clearRecAutomation(bool clearList)
{
	_volumeEnCtrl = true;
	_volumeEn2Ctrl = true;
	_panEnCtrl = true;
	_panEn2Ctrl = true;

	if (isMidiTrack())
		return;

	AudioTrack *t = (AudioTrack*)this;
	Pipeline *pl = t->efxPipe();
	BasePlugin *p;
	for (iPluginI i = pl->begin(); i != pl->end(); ++i)
	{
		p = *i;
		if (!p)
			continue;
		p->enableAllControllers(true);
	}

	if (clearList)
		t->recEvents()->clear();
}

//---------------------------------------------------------
//   dump
//---------------------------------------------------------

void Track::dump() const
{
	printf("Track <%s>: typ %d, parts %zd sel %d\n",
			_name.toLatin1().constData(), _type, _parts.size(), _selected);
}

//---------------------------------------------------------
//   MidiTrack
//---------------------------------------------------------

MidiTrack::MidiTrack()
: Track(MIDI)
{
	init();
	_events = new EventList;
	_mpevents = new MPEventList;
}

//MidiTrack::MidiTrack(const MidiTrack& mt)
//   : Track(mt)

MidiTrack::MidiTrack(const MidiTrack& mt, bool cloneParts)
: Track(mt, cloneParts)
{
	_outPort = mt.outPort();
	_outChannel = mt.outChannel();
	///_inPortMask    = mt.inPortMask();
	///_inChannelMask = mt.inChannelMask();
	m_samplerData = mt.m_samplerData;
	_events = new EventList;
	_mpevents = new MPEventList;
	transposition = mt.transposition;
	transpose = mt.transpose;
	velocity = mt.velocity;
	delay = mt.delay;
	len = mt.len;
	compression = mt.compression;
	_recEcho = mt.recEcho();
}

MidiTrack::~MidiTrack()
{
	delete _events;
	delete _mpevents;
	if(_wantsAutomation)
	{
    	if (_outPort >= 0 && _outPort < MIDI_PORTS)
    	{
    	    if (midiPorts[_outPort].device() && midiPorts[_outPort].device()->isSynthPlugin())
    	    {
    	        SynthPluginDevice* dev = (SynthPluginDevice*)midiPorts[_outPort].device();
				dev->close();
    	    }
			midiPorts[_outPort].inRoutes()->clear();
			midiPorts[_outPort].outRoutes()->clear();
			midiPorts[_outPort].patchSequences()->clear();
			midiPorts[_outPort].setFoundInSongFile(false);
			midiPorts[_outPort].setMidiDevice(0);
    	}
	}
}

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void MidiTrack::init()
{
	_outPort = 0;
	_outChannel = 0;

	transposition = 0;
	velocity = 0;
	delay = 0;
	len = 100; // percent
	compression = 100; // percent
	_recEcho = true;
	transpose = false;
	m_samplerData = 0;
	m_masterFlag = true; //Midi tracks should always be master

	m_midiassign.enabled = false;
	m_midiassign.port = 0;
	m_midiassign.preset = 0;
	m_midiassign.channel = 0;
	m_midiassign.track = this;
	m_midiassign.midimap.clear();

	m_midiassign.midimap.insert(CTRL_RECORD, new CCInfo((Track*)this, 0, 0, CTRL_RECORD, -1));
	m_midiassign.midimap.insert(CTRL_MUTE, new CCInfo((Track*)this, 0, 0, CTRL_MUTE, -1));
	m_midiassign.midimap.insert(CTRL_SOLO, new CCInfo((Track*)this, 0, 0, CTRL_SOLO, -1));
	m_midiassign.midimap.insert(CTRL_VOLUME, new CCInfo((Track*)this, 0, 0, CTRL_VOLUME, -1));
	m_midiassign.midimap.insert(CTRL_PANPOT, new CCInfo((Track*)this, 0, 0, CTRL_PANPOT, -1));
	//m_midiassign.midimap.insert(CTRL_REVERB_SEND, new CCInfo((Track*)this, 0, 0, CTRL_REVERB_SEND, -1));
	//m_midiassign.midimap.insert(CTRL_CHORUS_SEND, new CCInfo((Track*)this, 0, 0, CTRL_CHORUS_SEND, -1));
	//m_midiassign.midimap.insert(CTRL_VARIATION_SEND, new CCInfo((Track*)this, 0, 0, CTRL_VARIATION_SEND, -1));
}

int MidiTrack::getTransposition()
{
	if(transpose)
	{
		return transposition;
	}
	return 0;
}

//---------------------------------------------------------
//   setOutPort
//---------------------------------------------------------

void MidiTrack::setOutPort(int i)
{
    _outPort = i;

	MidiPort* mp = &midiPorts[i];
	if(mp)
		_outPortId = mp->id();
    if (i >= 0 && i < MIDI_PORTS)
    {
        _wantsAutomation = (midiPorts[i].device() && midiPorts[i].device()->isSynthPlugin());
        if (_wantsAutomation)
            ((SynthPluginDevice*)midiPorts[i].device())->setTrackId(m_id);
    }
}

void MidiTrack::setOutPortId(qint64 i)
{
    _outPortId = i;

    if (oomMidiPorts.contains(i))
	{
		MidiPort* mp = oomMidiPorts.value(i);
		_outPort = mp->portno();
		_wantsAutomation = (mp->device() && mp->device()->isSynthPlugin());
        if (_wantsAutomation)
            ((SynthPluginDevice*)mp->device())->setTrackId(m_id);
	}
}

//---------------------------------------------------------
//   setOutChanAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutChanAndUpdate(int i)
{
	if (_outChannel == i)
		return;

	removePortCtrlEvents(this);
	_outChannel = i;
	addPortCtrlEvents(this);
}

//---------------------------------------------------------
//   setOutPortAndUpdate
//---------------------------------------------------------

void MidiTrack::setOutPortAndUpdate(int i)/*{{{*/
{
    if (i >= 0 && i < MIDI_PORTS)
    {
        _wantsAutomation = (midiPorts[i].device() && midiPorts[i].device()->isSynthPlugin());
        if (_wantsAutomation)
            ((SynthPluginDevice*)midiPorts[i].device())->setTrackId(m_id);
    }

	MidiPort* mp = &midiPorts[i];
	if(mp)
		_outPortId = mp->id();
	if (_outPort == i)
		return;

	removePortCtrlEvents(this);
	_outPort = i;
	addPortCtrlEvents(this);
}/*}}}*/

void MidiTrack::setOutPortIdAndUpdate(qint64 i)/*{{{*/
{
    if (oomMidiPorts.contains(i))
	{
		MidiPort* mp = oomMidiPorts.value(i);
		_outPort = mp->portno();
		_wantsAutomation = (mp->device() && mp->device()->isSynthPlugin());
	}

	if (_outPortId == i)
		return;

	removePortCtrlEvents(this);
	_outPortId = i;
	addPortCtrlEvents(this);
}/*}}}*/

//---------------------------------------------------------
//   setInPortAndChannelMask
//   For old song files with port mask (max 32 ports) and channel mask (16 channels), 
//    before midi routing was added (the iR button). p3.3.48
//---------------------------------------------------------

void MidiTrack::setInPortAndChannelMask(unsigned int portmask, int chanmask)
{
	bool changed = false;

	for (int port = 0; port < 32; ++port) // 32 is the old maximum number of ports.
	{
		// p3.3.50 If the port was not used in the song file to begin with, just ignore it.
		// This saves from having all of the first 32 ports' channels connected.
		if (!midiPorts[port].foundInSongFile())
			continue;

		Route aRoute(port, chanmask); // p3.3.50
		Route bRoute(this, chanmask);

		if (portmask & (1 << port)) // p3.3.50
		{
			audio->msgAddRoute(aRoute, bRoute);
			changed = true;
		}
		else
		{
			audio->msgRemoveRoute(aRoute, bRoute);
			changed = true;
		}
	}

	if (changed)
	{
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
}

//---------------------------------------------------------
//   getAutomationTrack
//---------------------------------------------------------

AudioTrack* MidiTrack::getAutomationTrack()
{
    if (_outPort >= 0 && _outPort < MIDI_PORTS)
    {
        if (midiPorts[_outPort].device() && midiPorts[_outPort].device()->isSynthPlugin())
        {
            SynthPluginDevice* dev = (SynthPluginDevice*)midiPorts[_outPort].device();
            return dev->audioTrack();
        }
    }
    return 0;
}

//---------------------------------------------------------
//   addPart
//---------------------------------------------------------

iPart Track::addPart(Part* p)
{
	p->setTrack(this);
	return _parts.add(p);
}

//---------------------------------------------------------
//   findPart
//---------------------------------------------------------

Part* Track::findPart(unsigned tick)
{
	for (iPart i = _parts.begin(); i != _parts.end(); ++i)
	{
		Part* part = i->second;
		if (tick >= part->tick() && tick < (part->tick() + part->lenTick()))
			return part;
	}
	return 0;
}

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* MidiTrack::newPart(Part*p, bool clone)
{
	MidiPart* part = clone ? new MidiPart(this, p->events()) : new MidiPart(this);
	if (p)
	{
		part->setName(p->name());
		part->setColorIndex(getDefaultPartColor());

		*(PosLen*) part = *(PosLen*) p;
		part->setMute(p->mute());
	}

	if (clone)
	{
		//p->chainClone(part);
		part->setColorIndex(p->colorIndex());
		part->setZIndex(p->getZIndex());
		chainClone(p, part);
	}

	return part;
}

//---------------------------------------------------------
//   automationType
//---------------------------------------------------------

AutomationType MidiTrack::automationType() const
{
	MidiPort* port = &midiPorts[outPort()];
	return port->automationType(outChannel());
}

//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void MidiTrack::setAutomationType(AutomationType t)
{
    if (_wantsAutomation)
    {
        AudioTrack* atrack = getAutomationTrack();
        if (atrack)
            atrack->setAutomationType(t);
    }
    else
    {
        MidiPort* port = &midiPorts[outPort()];
        port->setAutomationType(outChannel(), t);
    }
}

bool MidiTrack::setRecordFlag1(bool f, bool monitor)
{
    _recordFlag = f;
	if(!monitor)
	{
		//Call the monitor here if it was not called from the monitor
		midiMonitor->msgSendMidiOutputEvent((Track*)this, CTRL_RECORD, f ? 127 : 0);
	}
    return true;
}

void MidiTrack::setRecordFlag2(bool, bool)
{
}

//---------------------------------------------------------
//   Track::writeProperties
//---------------------------------------------------------

void Track::writeProperties(int level, Xml& xml) const/*{{{*/
{
	xml.strTag(level, "name", _name);
	xml.qint64Tag(level, "trackId", m_id);
	if (!_comment.isEmpty())
		xml.strTag(level, "comment", _comment);
	xml.intTag(level, "record", _recordFlag);
	xml.intTag(level, "mute", mute());
	xml.intTag(level, "solo", solo());
	xml.intTag(level, "off", off());
	xml.intTag(level, "channels", _channels);
	xml.intTag(level, "height", _height);
	xml.intTag(level, "locked", _locked);
	xml.intTag(level, "reminder1", _reminder1);
	xml.intTag(level, "reminder2", _reminder2);
	xml.intTag(level, "reminder3", _reminder3);
	xml.intTag(level, "collapsed", _collapsed);
	xml.intTag(level, "mixertab", _mixerTab);
	xml.intTag(level, "partcolor", _partDefaultColor);
	if (_selected)
		xml.intTag(level, "selected", _selected);
	xml.nput(level, "<MidiAssign port=\"%d\"", m_midiassign.port);
	xml.nput(" channel=\"%d\"", m_midiassign.channel);
	xml.nput(" enabled=\"%d\"", (int)m_midiassign.enabled);
	xml.nput(" preset=\"%d\"", (int)m_midiassign.preset);
	QString assign;
	QHashIterator<int, CCInfo*> iter(m_midiassign.midimap);
	while(iter.hasNext())
	{
		iter.next();
		CCInfo* info = iter.value();
		assign.append(QString::number(info->port())).append(":")
			.append(QString::number(info->channel())).append(":")
			.append(QString::number(info->controller())).append(":")
			.append(QString::number(info->assignedControl())).append(":")
			.append(QString::number((int)info->recordOnly())).append(":")
			.append(QString::number((int)info->fakeToggle())).append(" ");
		//assign.append(QString::number(iter.key())).append(":").append(QString::number(iter.value())).append(" ");
	}
	xml.nput(" midimap=\"%s\"", assign.toUtf8().constData());
	xml.put(" />");
	xml.qint64Tag(level, "chainMaster", m_chainMaster);
	xml.intTag(level, "masterFlag", m_masterFlag);
	QStringList ac;
	if(m_audioChain.size())
	{
		foreach(qint64 id, m_audioChain)
		{
			ac.append(QString::number(id));
		}
		xml.strTag(level, "audioChain", ac.join(","));
	}
}/*}}}*/

//---------------------------------------------------------
//   Track::readProperties
//---------------------------------------------------------

bool Track::readProperties(Xml& xml, const QString& tag)/*{{{*/
{
	m_audioChain.clear();
	if (tag == "name")
		_name = xml.parse1();
	else if(tag == "trackId")
		m_id = xml.parseLongLong();
	else if (tag == "comment")
		_comment = xml.parse1();
	else if (tag == "record")
	{
		bool recordFlag = xml.parseInt();
		setRecordFlag1(recordFlag);
		setRecordFlag2(recordFlag);
	}
	else if (tag == "mute")
		_mute = xml.parseInt();
	else if (tag == "solo")
		_solo = xml.parseInt();
	else if (tag == "off")
		_off = xml.parseInt();
	else if (tag == "height")
		_height = xml.parseInt();
	else if (tag == "channels")
	{
		_channels = xml.parseInt();
		if (_channels > MAX_CHANNELS)
			_channels = MAX_CHANNELS;
	}
	else if (tag == "locked")
		_locked = xml.parseInt();
	else if (tag == "selected")
		_selected = xml.parseInt();
	else if (tag == "reminder1")
		_reminder1 = (bool)xml.parseInt();
	else if (tag == "reminder2")
		_reminder2 = (bool)xml.parseInt();
	else if (tag == "reminder3")
		_reminder3 = (bool)xml.parseInt();
	else if (tag == "collapsed")
		_collapsed = (bool)xml.parseInt();
	else if(tag == "mixertab")
		_mixerTab = xml.parseInt();
	else if(tag == "partcolor")
		_partDefaultColor = xml.parseInt();
	else if(tag == "MidiAssign")
		m_midiassign.read(xml, (Track*)this);
	else if(tag == "chainMaster")
		m_chainMaster = xml.parseLongLong();
	else if(tag == "masterFlag")
		m_masterFlag = xml.parseInt();
	else if(tag == "audioChain")
	{
		QString ids = xml.parse1();
		QStringList ac = ids.split(",");
		foreach(QString id, ac)
		{
			qint64 tid = id.toLongLong();
			m_audioChain.append(tid);
		}
	}
	else
	{
		return true;
	}
	return false;
}/*}}}*/

//---------------------------------------------------------
//   writeRouting
//---------------------------------------------------------

void Track::writeRouting(int level, Xml& xml) const/*{{{*/
{
	QString s;

	if (type() == Track::AUDIO_INPUT)
	{
		const RouteList* rl = &_inRoutes;
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			if (!r->name().isEmpty())
			{
				s = QT_TRANSLATE_NOOP("@default", "Route");
				if (r->channel != -1)
					s += QString(QT_TRANSLATE_NOOP("@default", " channel=\"%1\"")).arg(r->channel);

				xml.tag(level++, s.toAscii().constData());

				s = QT_TRANSLATE_NOOP("@default", "source");
				if (r->type != Route::TRACK_ROUTE)
					s += QString(QT_TRANSLATE_NOOP("@default", " type=\"%1\"")).arg(r->type);
				else
				{
					s += QString(QT_TRANSLATE_NOOP("@default", " trackId=\"%1\"")).arg(r->track->id());
				}
				s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(Xml::xmlString(r->name()));
				xml.tag(level, s.toAscii().constData());

				xml.tag(level, "dest name=\"%s\" trackId=\"%lld\"/", Xml::xmlString(name()).toLatin1().constData(), m_id);

                xml.etag(--level, "Route");
			}
		}
	}

	const RouteList* rl = &_outRoutes;
	for (ciRoute r = rl->begin(); r != rl->end(); ++r)
	{
		if (r->midiPort != -1 || !r->name().isEmpty()) // p3.3.49
		{
			s = QT_TRANSLATE_NOOP("@default", "Route");
			if (r->type == Route::MIDI_PORT_ROUTE) // p3.3.50
			{
				if (r->channel != -1 && r->channel != 0)
					s += QString(QT_TRANSLATE_NOOP("@default", " channelMask=\"%1\"")).arg(r->channel); // Use new channel mask.
			}
			else
			{
				if (r->channel != -1)
					s += QString(QT_TRANSLATE_NOOP("@default", " channel=\"%1\"")).arg(r->channel);
			}
			if (r->channels != -1)
				s += QString(QT_TRANSLATE_NOOP("@default", " channels=\"%1\"")).arg(r->channels);
			if (r->remoteChannel != -1)
				s += QString(QT_TRANSLATE_NOOP("@default", " remch=\"%1\"")).arg(r->remoteChannel);

			xml.tag(level++, s.toAscii().constData());

			xml.tag(level, "source name=\"%s\" trackId=\"%lld\"/", Xml::xmlString(name()).toLatin1().constData(), m_id);

			s = QT_TRANSLATE_NOOP("@default", "dest");

			if (r->type != Route::TRACK_ROUTE && r->type != Route::MIDI_PORT_ROUTE)
				s += QString(QT_TRANSLATE_NOOP("@default", " type=\"%1\"")).arg(r->type);
			else if(r->type == Route::TRACK_ROUTE)
				s += QString(QT_TRANSLATE_NOOP("@default", " trackId=\"%1\"")).arg(r->track->id());

			if (r->type == Route::MIDI_PORT_ROUTE) 
				s += QString(QT_TRANSLATE_NOOP("@default", " mport=\"%1\"/")).arg(r->midiPort);
			else
				s += QString(QT_TRANSLATE_NOOP("@default", " name=\"%1\"/")).arg(Xml::xmlString(r->name()));

			xml.tag(level, s.toAscii().constData());

            xml.etag(--level, "Route");
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   MidiTrack::write
//---------------------------------------------------------

void MidiTrack::write(int level, Xml& xml) const/*{{{*/
{
	const char* tag;

	if (type() == DRUM)
		tag = "drumtrack";
	else
		tag = "miditrack";
	xml.tag(level++, tag);
	Track::writeProperties(level, xml);

	//Make sure we have the proper id but using the id of the port currently set
	MidiPort* mp = &midiPorts[outPort()];
	if(mp)
	{ 	
		if(mp->id() == outPortId())
			xml.qint64Tag(level, "deviceId", outPortId());
		else
		{
			xml.qint64Tag(level, "deviceId", mp->id());
		}
	}
	/*else
	{//Its probably unconfgured just use the index
		xml.intTag(level, "device", outPort());
	}*/
	xml.intTag(level, "channel", outChannel());
	xml.intTag(level, "locked", _locked);
	xml.intTag(level, "echo", _recEcho);

	xml.intTag(level, "transposition", transposition);
	xml.intTag(level, "transpose", transpose);
	xml.intTag(level, "velocity", velocity);
	xml.intTag(level, "delay", delay);
	xml.intTag(level, "len", len);
	xml.intTag(level, "compression", compression);
    
    if (_wantsAutomation)
    {
        int atype = 0;

        // FIXME - cannot use 'getAutomationTrack()' here
        if (_outPort >= 0 && _outPort < MIDI_PORTS)
        {
            if (midiPorts[_outPort].device() && midiPorts[_outPort].device()->isSynthPlugin())
            {
                SynthPluginDevice* dev = (SynthPluginDevice*)midiPorts[_outPort].device();
                AudioTrack* atrack = dev->audioTrack();
                if (atrack)
                    atype = atrack->automationType();
            }
        }

        xml.intTag(level, "automation", atype);
    }
    else
        xml.intTag(level, "automation", int(automationType()));

	const PartList* pl = cparts();
	for (ciPart p = pl->begin(); p != pl->end(); ++p)
		p->second->write(level, xml);
    xml.etag(--level, tag);
}/*}}}*/

//---------------------------------------------------------
//   MidiTrack::read
//---------------------------------------------------------

void MidiTrack::read(Xml& xml)/*{{{*/
{
	unsigned int portmask = 0;
	int chanmask = 0;

	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "transposition")
					transposition = xml.parseInt();
				else if (tag == "transpose")
					transpose = (bool)xml.parseInt();
				else if (tag == "velocity")
					velocity = xml.parseInt();
				else if (tag == "delay")
					delay = xml.parseInt();
				else if (tag == "len")
					len = xml.parseInt();
				else if (tag == "compression")
					compression = xml.parseInt();
				else if (tag == "part")
				{
					Part* p = 0;
					p = readXmlPart(xml, this);
					if (p)
						parts()->add(p);
				}
				else if (tag == "device")
				{//Check if global inputs has bumped the port order and increment
					setOutPort(xml.parseInt());
				}
				else if (tag == "deviceId")
					setOutPortId(xml.parseLongLong());
				else if (tag == "channel")
					setOutChannel(xml.parseInt());
				else if (tag == "inportMap")
					portmask = xml.parseUInt(); // p3.3.48: Support old files.
				else if (tag == "inchannelMap")
					chanmask = xml.parseInt(); // p3.3.48: Support old files.
				else if (tag == "locked")
					_locked = xml.parseInt();
				else if (tag == "echo")
					_recEcho = xml.parseInt();
				else if (tag == "automation")
					setAutomationType(AutomationType(xml.parseInt()));
				else if (Track::readProperties(xml, tag))
				{
					// version 1.0 compatibility:
					if (tag == "track" && xml.majorVersion() == 1 && xml.minorVersion() == 0)
						break;
					xml.unknown("MidiTrack");
				}
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if (tag == "miditrack" || tag == "drumtrack")
				{
					setInPortAndChannelMask(portmask, chanmask); // p3.3.48: Support old files.
					return;
				}
			default:
				break;
		}
	}
}/*}}}*/

void MidiAssignData::read(Xml& xml, Track* t)/*{{{*/
{
	enabled = false;
	port = 0;
	preset = 0;
	channel = 0;
	track = t;
	midimap.clear();
	switch(t->type())
	{
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
			midimap.insert(CTRL_AUX1, new CCInfo(t, 0, 0, CTRL_AUX1, -1));
			midimap.insert(CTRL_AUX2, new CCInfo(t, 0, 0, CTRL_AUX2, -1));
		break;
		case Track::AUDIO_SOFTSYNTH:
		case Track::AUDIO_AUX:
		break;
		case Track::WAVE:
			midimap.insert(CTRL_AUX1, new CCInfo(t, 0, 0, CTRL_AUX1, -1));
			midimap.insert(CTRL_AUX2, new CCInfo(t, 0, 0, CTRL_AUX2, -1));
			midimap.insert(CTRL_RECORD, new CCInfo(t, 0, 0, CTRL_RECORD, -1));
		break;
		default:
			midimap.insert(CTRL_RECORD, new CCInfo(t, 0, 0, CTRL_RECORD, -1));
		break;
	}
	midimap.insert(CTRL_MUTE, new CCInfo(t, 0, 0, CTRL_MUTE, -1));
	midimap.insert(CTRL_SOLO, new CCInfo(t, 0, 0, CTRL_SOLO, -1));
	midimap.insert(CTRL_VOLUME, new CCInfo(t, 0, 0, CTRL_VOLUME, -1));
	midimap.insert(CTRL_PANPOT, new CCInfo(t, 0, 0, CTRL_PANPOT, -1));
	/*if(t->isMidiTrack())
	{
		midimap.insert(CTRL_REVERB_SEND, new CCInfo(t, 0, 0, CTRL_REVERB_SEND, -1));
		midimap.insert(CTRL_CHORUS_SEND, new CCInfo(t, 0, 0, CTRL_CHORUS_SEND, -1));
		midimap.insert(CTRL_VARIATION_SEND, new CCInfo(t, 0, 0, CTRL_VARIATION_SEND, -1));
	}*/
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::Attribut:
			{
				QString s = xml.s2();
				if (tag == "port")
					port = xml.s2().toInt();
				else if (tag == "channel")
					channel = xml.s2().toInt();
				else if(tag == "enabled")
					enabled = (bool)xml.s2().toInt();
				else if(tag == "preset")
					preset = xml.s2().toInt();
				else if (tag == "midimap")
				{
					QStringList vals = xml.s2().split(" ", QString::SkipEmptyParts);
					foreach(QString ccpair, vals)
					{
						QStringList cclist = ccpair.split(":", QString::SkipEmptyParts);
						if(cclist.size() == 2)
						{//First old implementation, just here to update templates
							//midimap[cclist[0].toInt()] = cclist[1].toInt();
							midimap.insert(cclist[0].toInt(), new CCInfo(t, port, channel, cclist[0].toInt(), cclist[1].toInt()));
						}
						else if(cclist.size() == 4)
						{ //Added change and port to assign data
							midimap.insert(cclist[2].toInt(), new CCInfo(t, cclist[0].toInt(), cclist[1].toInt(), cclist[2].toInt(), cclist[3].toInt()));
						}
						else if(cclist.size() == 5)
						{ //Added record only
							midimap.insert(cclist[2].toInt(), new CCInfo(t, cclist[0].toInt(), cclist[1].toInt(), cclist[2].toInt(), cclist[3].toInt(), cclist[4].toInt()));
						}
						else if(cclist.size() == 6)
						{ //Added fakeToggle
							midimap.insert(cclist[2].toInt(), new CCInfo(t, cclist[0].toInt(), cclist[1].toInt(), cclist[2].toInt(), cclist[3].toInt(), cclist[4].toInt(), cclist[5].toInt()));
						}
					}
				}
			}
				break;
			case Xml::TagStart:
				xml.unknown("MidiAssign");
				break;
			case Xml::TagEnd:
				if (tag == "MidiAssign")
				{
					return;
				}
			default:
				break;
		}
	}
}/*}}}*/

