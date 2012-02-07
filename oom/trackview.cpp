//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#include "event.h"
#include "song.h"
#include "audio.h"
#include "midi.h"
#include "midictrl.h"
#include "xml.h"
#include "utils.h"
#include "globaldefs.h"
#include "trackview.h"
#include "track.h"
#include "TrackManager.h"

TrackView::TrackView(bool istemplate)
{
	m_id = create_id();
	_selected = false;
	_recState = false;
	m_template = istemplate;
}

TrackView::~TrackView()
{
}

void TrackView::setDefaultName()/*{{{*/
{
	QString base = QString("Track View");
	setViewName(getValidName(base));
}/*}}}*/


QString TrackView::getValidName(QString text)
{
	QString spc(" ");
	QString rv = text;
	TrackViewList* tv;
	if(m_template)
		tv = song->instrumentTemplates();
	else
		tv = song->trackviews();
	QHash<qint64, TrackView*>::const_iterator iter = tv->constBegin();
	int c = 1;
	while(iter != tv->constEnd())
	{
		QString n = QString::number(c);
		if(iter.value()->viewName() == rv)
		{
			rv = text + spc + n;
			iter = tv->constBegin();
			++c;
			continue;
		}
		++iter;
	}
	return rv;
}

//---------------------------------------------------------
//    addTrack
//---------------------------------------------------------

void TrackView::addTrack(qint64 id)/*{{{*/
{
	m_tracks[id] = new TrackView::TrackViewTrack(id);
	m_tracksIndex.append(id);
}/*}}}*/

void TrackView::addTrack(TrackView::TrackViewTrack* t)
{
	m_tracks[t->id] = t;
	m_tracksIndex.append(t->id);
}

void TrackView::removeTrack(qint64 id)
{
	if(m_tracks.contains(id))
	{
		m_tracks.erase(m_tracks.find(id));
		m_tracksIndex.removeAll(id);
	}
	//This needs to fire something so the gui gets updated
}

void TrackView::removeTrack(TrackView::TrackViewTrack *t)
{
	if(t && m_tracks.contains(t->id))
	{
		m_tracks.erase(m_tracks.find(t->id));
		m_tracksIndex.removeAll(t->id);
	}
	//This needs to fire something so the gui gets updated
}


void TrackView::addVirtualTrack(VirtualTrack* vt)
{
	if(vt)
		m_vtracks[vt->id] = vt;
}

qint64 TrackView::addVirtualTrackCopy(VirtualTrack* vt)
{
	VirtualTrack* v = new VirtualTrack;
//{{{
	v->type = vt->type;
	v->name = vt->name;
	v->instrumentName = vt->instrumentName;
	v->instrumentType = vt->instrumentType;
	v->autoCreateInstrument = vt->autoCreateInstrument;
	v->createMidiInputDevice = vt->createMidiInputDevice;
	v->createMidiOutputDevice = vt->createMidiOutputDevice;
	v->useInput = vt->useInput;
	v->useOutput = vt->useOutput;
	v->useMonitor = vt->useMonitor;
	v->useBuss = vt->useBuss;
	v->inputChannel = vt->inputChannel;
	v->outputChannel = vt->outputChannel;
	v->inputConfig = vt->inputConfig;
	v->outputConfig = vt->outputConfig;
	v->monitorConfig = vt->monitorConfig;
    v->monitorConfig2 = vt->monitorConfig2;
	v->bussConfig = vt->bussConfig;
//}}}
	m_vtracks[v->id] = v;
	return v->id;
}

void TrackView::removeVirtualTrack(qint64 tvid)
{
	m_vtracks.erase(m_vtracks.find(tvid));
}

void TrackView::setSelected(bool f)
{//TODO: Add code to deal with selection when virtual tracks exists
	_selected = f;
	if(f)
	{
		foreach(qint64 id, m_tracksIndex)/*{{{*/
		{
			Track* it = song->findTrackById(id);
			TrackViewTrack* tvt = m_tracks.value(id);
			if(it)
			{
				if(it->isMidiTrack() && tvt->hasSettings() && !tvt->is_virtual)
				{
					MidiTrack* track = (MidiTrack*)it;
					TrackSettings* tset = tvt->settings;
					if(tset)
					{
						if(tset->transpose != 0)
						{
							track->transposition = tset->transpose;
							track->transpose = true;
						}
						if(tset->program >= 0)
						{
							int channel = track->outChannel();
							int port = track->outPort();

							MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, tset->program, (Track*)track);
							audio->msgPlayMidiEvent(&ev);
						}
					}
				}
			}
			//(*it)->setRecordFlag1(false);
			//(*it)->setRecordFlag2(false);
			//(*it)->setSelected(false);
		}/*}}}*/
	}
	else
	{
		foreach(qint64 id, m_tracksIndex)/*{{{*/
		{
			Track* it = song->findTrackById(id);
			TrackViewTrack* tvt = m_tracks.value(id);
			if(it)
			{
				if(it->isMidiTrack() && tvt->hasSettings() && !tvt->is_virtual)
				{
					MidiTrack* track = (MidiTrack*) it;
					TrackSettings* tset = tvt->settings;
					if(tset)
					{
						if(tset->transpose != 0)
						{
							track->transposition = 0;
							track->transpose = false;
						}
					}
				}
				//Remove record arm so we dont have hidded tracks armed
				//As requested by Wendy Cherrett
				it->setRecordFlag1(false);
				it->setRecordFlag2(false);
				it->setSelected(false);
			}
		}/*}}}*/
	}
}

void TrackView::clear()
{
	m_tracks.clear();
	m_tracksIndex.clear();
	m_vtracks.clear();
}

//---------------------------------------------------------
//   TrackView::read
//---------------------------------------------------------

void TrackView::read(Xml& xml)/*{{{*/
{
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
				if (tag == "id")/*{{{*/
				{
					m_id = xml.parseLongLong();
				}
				else if(tag == "selected")
				{
					_selected = (bool)xml.parseInt();
				}
				else if(tag == "template")
				{
					m_template = xml.parseInt();
				}
				else if(tag == "record")
				{
					_recState = (bool)xml.parseInt();
				}/*}}}*/
				else if(tag == "name")
				{
					_name = xml.parse1();
				}
				else if(tag == "comment")
				{
					_comment = xml.parse1();
				}
				else if(tag == "virtualTrack")
				{//TODO: Virtual track read code;
					//skip for now
					VirtualTrack* vt = new VirtualTrack;
					vt->read(xml);
					m_vtracks[vt->id] = vt;
				}
				else if(tag == "trackViewTrack")
				{
					TrackView::TrackViewTrack* tvt = new TrackView::TrackViewTrack;
					tvt->read(xml);
					addTrack(tvt);
				}
				else if (tag == "vtrack")
				{//vtrack is being phased out
					//Deal with old style, we still have an addTrack that will create a new object from id
					//Later we add the settings to that object
					bool ok;
					QString stid = xml.parse1();
					qint64 tid = stid.toLongLong(&ok);
					Track* t = 0;
					if(ok)
					{
						addTrack(tid);
					}
					else
					{
						t = song->findTrack(xml.parse1());
						if (t)
						{
							//printf("TrackView::read() Adding track\n");
							addTrack(t->id());
						}
					}
				}
				else if(tag == "tracksettings")
				{//This tag should ONLY exist at this level in old files, so we convert them silently - Andrew
					TrackSettings *ts = new TrackSettings;
					ts->valid = true;
					ts->read(xml);
					foreach(TrackViewTrack *t, m_tracks)
					{
						if(t->id == ts->tid)
						{
							t->settings = ts;
							break;
						}
					}
					//_tSettings[ts->tid] = ts;
				}
				break;
			case Xml::Attribut:
				if (tag == "id")
				{
					m_id = xml.s2().toLongLong();
				}
				else if(tag == "selected")
				{
					_selected = (bool)xml.s2().toInt();
				}
				else if(tag == "template")
				{
					m_template = xml.s2().toInt();
				}
				else if(tag == "record")
				{
					_recState = (bool)xml.s2().toInt();
				}
				break;
			case Xml::TagEnd:
				if(tag == "trackview")
				{
					//This calls an update on the track states that are a part of this view
					setSelected(_selected);
					return;
				}
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   TrackView::write
//---------------------------------------------------------

void TrackView::write(int level, Xml& xml) const /*{{{*/
{
	std::string tag = "trackview";

	xml.put(level, "<%s id=\"%lld\" record=\"%d\" selected=\"%d\" template=\"%d\">", tag.c_str(), m_id, _recState, _selected, m_template);
	level++;
	xml.strTag(level, "name", _name);
	if(!_comment.isEmpty())
		xml.strTag(level, "comment", Xml::xmlString(_comment).toUtf8().constData());

	QMapIterator<qint64, VirtualTrack*> ts(m_vtracks);
	while(ts.hasNext())
	{
		ts.next();
		ts.value()->write(level, xml);
	}
	foreach(TrackView::TrackViewTrack *t, m_tracks)
	{
		t->write(level, xml);
		//xml.qint64Tag(level, "vtrack", id);
	}
    xml.put(--level, "</%s>", tag.c_str());
}/*}}}*/

void TrackView::TrackSettings::write(int level, Xml& xml) const/*{{{*/
{
	std::string tag = "trackSettings";
	xml.put(level, "<%s pname=\"%s\" program=\"%d\" rec=\"%d\" transpose=\"%d\" />",
			tag.c_str(), Xml:xmlString(pname).toUtf8().constData(), program, rec, transpose);
}/*}}}*/

void TrackView::TrackSettings::read(Xml& xml)/*{{{*/
{
	program = -1;
	rec = 0;
	pname = QString("");
	transpose = 0;
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
				//Backwards compat
				if(tag == "pname")/*{{{*/
				{
					pname = xml.parse1();
				}
				else if(tag == "trackname")
				{//Old style update compat
					Track *t = song->findTrack(xml.parse1());
					if(t)
						tid = t->id();
					//TODO: Set the upgrade flag
				}
				else if(tag == "trackId")
				{
					tid = xml.parseLongLong();
				}
				else if(tag == "rec")
				{
					rec = (bool)xml.parseInt();
				}
				else if(tag == "program")
				{
					program = (bool)xml.parseInt();
				}
				else if(tag == "transpose")
				{
					transpose = xml.parseInt();
				}/*}}}*/
				break;
			case Xml::Attribut:
				if(tag == "pname")/*{{{*/
				{
					pname = xml.s2();
				}
				else if(tag == "rec")
				{
					rec = (bool)xml.s2().toInt();
				}
				else if(tag == "program")
				{
					program = (bool)xml.s2().toInt();
				}
				else if(tag == "transpose")
				{
					transpose = xml.s2().toInt();
				}/*}}}*/
				break;
			case Xml::TagEnd:
				if(tag == "trackSettings" || tag == "tracksettings")
					return;
			default:
				break;
		}
	}
}/*}}}*/

void TrackView::TrackViewTrack::setSettingsCopy(TrackView::TrackSettings* ts)
{
	if(ts)
	{
		settings = new TrackView::TrackSettings;
		settings->pname = ts->pname;
		settings->program = ts->program;
		settings->rec = ts->rec;
		settings->transpose = ts->transpose;
	}
}

void TrackView::TrackViewTrack::write(int level, Xml& xml) const
{
	std::string tag = "trackViewTrack";
	xml.put(level, "<%s trackId=\"%lld\" virtual=\"%d\">", tag.c_str(), id, is_virtual);
	level++;
	if(settings)
		settings->write(level, xml);
    xml.put(--level, "</%s>", tag.c_str());
}

void TrackView::TrackViewTrack::read(Xml& xml)
{
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
				if(tag == "trackSettings")
				{
					settings = new TrackSettings;
					settings->read(xml);
				}
				break;
			case Xml::Attribut:
				if(tag == "trackId")
				{
					id = xml.s2().toLongLong();
				}
				else if(tag == "virtual")
				{
					is_virtual = xml.s2().toInt();
				}
				break;
			case Xml::TagEnd:
				if(tag == "trackViewTrack")
					return;
			default:
				break;
		}
	}
}

