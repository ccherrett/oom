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
#include "globaldefs.h"
#include "trackview.h"
#include "track.h"

TrackView::TrackView()
{
	_selected = false;
	_recState = false;
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
	TrackView* tv = song->findTrackView(text);
	int c = 1;
	while(tv)
	{
		QString n = QString::number(c);
		rv = text + spc + n;
		tv = song->findTrackView(rv);
		++c;
	}
	return rv;
}

//---------------------------------------------------------
//    addTrack
//---------------------------------------------------------

void TrackView::addTrack(Track* t)/*{{{*/
{
	_tracks.push_back(t);
}/*}}}*/

void TrackView::removeTrack(Track* t)
{
	_tracks.erase(t);
	//This needs to fire something so the gui gets updated
}

void TrackView::setSelected(bool f)
{
	_selected = f;
	if(f)
	{
		for(iTrack it = tracks()->begin(); it != tracks()->end(); ++it)/*{{{*/
		{
			if((*it)->isMidiTrack() && hasSettings((*it)->name()))
			{
				MidiTrack* track = (MidiTrack*) (*it);;
				TrackSettings* tset = _tSettings[(*it)->name()];
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
			//(*it)->setRecordFlag1(false);
			//(*it)->setRecordFlag2(false);
			//(*it)->setSelected(false);
		}/*}}}*/
	}
	else
	{
		for(iTrack it = tracks()->begin(); it != tracks()->end(); ++it)/*{{{*/
		{
			if((*it)->isMidiTrack() && hasSettings((*it)->name()))
			{
				MidiTrack* track = (MidiTrack*) (*it);;
				TrackSettings* tset = _tSettings[(*it)->name()];
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
			(*it)->setRecordFlag1(false);
			(*it)->setRecordFlag2(false);
			(*it)->setSelected(false);
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
				if(tag == "name")
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
					Track* t = song->findTrack(xml.parse1());
					if (t)
					{
						//printf("TrackView::read() Adding track\n");
						addTrack(t);
					}
				}
				else if(tag == "tracksettings")
				{
					TrackSettings *ts = new TrackSettings;
					ts->valid = true;
					ts->read(xml);
					if(ts->track)
						_tSettings[ts->track->name()] = ts;
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
