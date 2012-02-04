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

TrackView::TrackView()
{
	_selected = false;
	_recState = false;
	m_id = create_id();
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
	TrackViewList* tv = song->trackviews();
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
	m_tracks.append(id);
}/*}}}*/

void TrackView::removeTrack(qint64 id)
{
	if(!m_tracks.isEmpty() && m_tracks.contains(id))
		m_tracks.removeAll(id); //There should NEVER be duplicate ids in the system on anything
	//This needs to fire something so the gui gets updated
}


void TrackView::addVirtualTrack(VirtualTrack* vt)
{
	if(vt)
		m_vtracks[vt->id] = vt;
}

void TrackView::removeVirtualTrack(qint64 tvid)
{
	m_vtracks.erase(m_vtracks.find(tvid));
}

void TrackView::setSelected(bool f)
{
	_selected = f;
	if(f)
	{
		foreach(qint64 id, m_tracks)/*{{{*/
		{
			Track* it = song->findTrackById(id);
			if(it)
			{
				if(it->isMidiTrack() && hasSettings(id))
				{
					MidiTrack* track = (MidiTrack*)it;
					TrackSettings* tset = _tSettings[id];
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
		foreach(qint64 id, m_tracks)/*{{{*/
		{
			Track* it = song->findTrackById(id);
			if(it)
			{
				if(it->isMidiTrack() && hasSettings(id))
				{
					MidiTrack* track = (MidiTrack*) it;
					TrackSettings* tset = _tSettings[id];
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
				if (tag == "id")
				{
					m_id = xml.parseLongLong();
				}
				else if(tag == "name")
				{
					_name = xml.parse1();
				}
				else if(tag == "comment")
				{
					_comment = xml.parse1();
				}
				else if(tag == "selected")
				{
					_selected = (bool)xml.parseInt();
				}
				else if(tag == "record")
				{
					_recState = (bool)xml.parseInt();
				}
				else if (tag == "vtrack")
				{
					//Deal with old style
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
				{
					TrackSettings *ts = new TrackSettings;
					ts->valid = true;
					ts->read(xml);
					_tSettings[ts->tid] = ts;
				}
				break;
			case Xml::Attribut:
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

	xml.put(level, "<%s>", tag.c_str());//, _name.toStdString().c_str(), _selected, _type);
	level++;
	xml.strTag(level, "name", _name);
	xml.intTag(level, "selected", _selected);
	xml.intTag(level, "record", _recState);
	if(!_comment.isEmpty())
		xml.strTag(level, "comment", Xml::xmlString(_comment).toUtf8().constData());

	//for(iTrack* t = _tracks.begin(); t != _tracks.end(); ++t)
	foreach(qint64 id, m_tracks)
	{
		xml.qint64Tag(level, "vtrack", id);
	}
	QMapIterator<qint64, TrackSettings*> ts(_tSettings);
	while(ts.hasNext())
	{
		ts.next();
		ts.value()->write(level, xml);
	}
    xml.put(--level, "</%s>", tag.c_str());
}/*}}}*/

void TrackSettings::write(int level, Xml& xml) const/*{{{*/
{
	std::string tag = "tracksettings";
	xml.put(level, "<%s>", tag.c_str());
	xml.qint64Tag(level, "trackId", tid);
	xml.strTag(level, "pname", pname.toUtf8().constData());
	xml.intTag(level, "program", program);
	xml.intTag(level, "rec", rec);
	xml.intTag(level, "transpose", transpose);
    xml.put(--level, "</%s>", tag.c_str());
}/*}}}*/

void TrackSettings::read(Xml& xml)/*{{{*/
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
				if(tag == "pname")
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
					tid = xml.parse1().toLongLong();
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
				}
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if(tag == "tracksettings")
					return;
			default:
				break;
		}
	}
}/*}}}*/

