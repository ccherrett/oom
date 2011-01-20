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
}

TrackView::~TrackView()
{
}

void TrackView::setDefaultName()/*{{{*/
{
	QString base = QString("Track View");
	base += " ";
	for (int i = 1; true; ++i)
	{
		QString n;
		n.setNum(i);
		QString s = base + n;
		TrackView* tv = song->findTrackView(s);
		if (tv == 0)
		{
			setViewName(s);
			break;
		}
	}
}/*}}}*/

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
				else if (tag == "vtrack")
				{
					Track* t = song->findTrack(xml.parse1());
					if (t)
					{
						printf("TrackView::read() Adding track\n");
						addTrack(t);
					}
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
