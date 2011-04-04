//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#include "event.h"
#include "song.h"
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
	if(!f)
	{
		for(iTrack it = tracks()->begin(); it != tracks()->end(); ++it)
		{
			(*it)->setRecordFlag1(false);
			(*it)->setRecordFlag2(false);
			(*it)->setSelected(false);
		}
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
					TrackSettings ts;
					ts.valid = true;
					ts.read(xml);
					_tSettings[ts.name] = ts;
				}
				break;
			case Xml::Attribut:
				break;
			case Xml::TagEnd:
				if(tag == "trackview")
					return;
			default:
				break;
		}
	}
}/*}}}*/
