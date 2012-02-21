//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: AbstractMidiEditor.cpp,v 1.2.2.2 2009/02/02 21:38:00 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include "AbstractMidiEditor.h"
#include "midiedit/EventCanvas.h"
#include "scrollscale.h"
#include "mtscale.h"
#include "xml.h"
#include "part.h"
#include "track.h"
#include "song.h"
#include "event.h"
#include "PerformerCanvas.h"

#include <QRect>
#include <QColor>
#include <QGridLayout>

//---------------------------------------------------------
//   AbstractMidiEditor
//---------------------------------------------------------

AbstractMidiEditor::AbstractMidiEditor(int q, int r, PartList* pl, QWidget* parent, const char* name)
: TopWin(parent, name)
{
	setAttribute(Qt::WA_DeleteOnClose);
	_pl = pl;
	if (_pl)
		for (iPart i = _pl->begin(); i != _pl->end(); ++i)
			_parts.push_back(i->second->sn());
	_quant = q;
	_raster = r;
	canvas = 0;
	_curDrumInstrument = -1;
	mainw = new QWidget(this);

	///mainGrid = new QGridLayout(mainw);
	mainGrid = new QGridLayout();
	//mainw->setLayout(mainGrid);

	mainGrid->setContentsMargins(0, 0, 0, 0);
	mainGrid->setSpacing(0);
	setCentralWidget(mainw);
}

//---------------------------------------------------------
//   genPartlist
//---------------------------------------------------------

void AbstractMidiEditor::genPartlist()/*{{{*/
{
	_pl->clear();
	for (std::list<int>::iterator i = _parts.begin(); i != _parts.end(); ++i)
	{
		TrackList* tl = song->tracks();
		for (iTrack it = tl->begin(); it != tl->end(); ++it)
		{
			PartList* pl = (*it)->parts();
			iPart ip;
			for (ip = pl->begin(); ip != pl->end(); ++ip)
			{
				if (ip->second->sn() == *i)
				{
					_pl->add(ip->second);
					break;
				}
			}
			if (ip != pl->end())
				break;
		}
	}
}/*}}}*/

bool AbstractMidiEditor::hasPart(int sn)/*{{{*/
{
	bool rv = false;
	for (std::list<int>::iterator i = _parts.begin(); i != _parts.end(); ++i)
	{
		if(sn == *i)
		{
			rv = true;
			break;
		}
	}
	return rv;
}/*}}}*/

//---------------------------------------------------------
//   AbstractMidiEditor
//---------------------------------------------------------

AbstractMidiEditor::~AbstractMidiEditor()
{
	if (_pl)
		delete _pl;
}

//---------------------------------------------------------
//   quantVal
//---------------------------------------------------------

int AbstractMidiEditor::quantVal(int v) const/*{{{*/
{
	int val = ((v + _quant / 2) / _quant) * _quant;
	if (val == 0)
		val = _quant;
	return val;
}/*}}}*/

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void AbstractMidiEditor::readStatus(Xml& xml)/*{{{*/
{
	if (_pl == 0)
		_pl = new PartList;

	for (;;)
	{
		Xml::Token token = xml.parse();
		QString tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "quant")
					_quant = xml.parseInt();
				else if (tag == "raster")
					_raster = xml.parseInt();
				else if (tag == "topwin")
					TopWin::readStatus(xml);
				else
					xml.unknown("AbstractMidiEditor");
				break;
			case Xml::TagEnd:
				if (tag == "midieditor")
					return;
			default:
				break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   writePartList
//---------------------------------------------------------

void AbstractMidiEditor::writePartList(int level, Xml& xml) const/*{{{*/
{
	for (ciPart p = _pl->begin(); p != _pl->end(); ++p)
	{
		Part* part = p->second;
		Track* track = part->track();
		int trkIdx = song->artracks()->index(track);
		int partIdx = track->parts()->index(part);

		if ((trkIdx == -1) || (partIdx == -1))
			printf("AbstractMidiEditor::writePartList error: trkIdx:%d partIdx:%d\n", trkIdx, partIdx);

		xml.put(level, "<part>%d:%d</part>", trkIdx, partIdx);
	}
}/*}}}*/

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void AbstractMidiEditor::writeStatus(int level, Xml& xml) const/*{{{*/
{
	xml.tag(level++, "midieditor");
	TopWin::writeStatus(level, xml);
	xml.intTag(level, "quant", _quant);
	xml.intTag(level, "raster", _raster);
	xml.tag(level, "/midieditor");
}/*}}}*/

void AbstractMidiEditor::addParts(PartList* pl)/*{{{*/
{
	if(pl)
	{
		for(iPart i = pl->begin(); i != pl->end(); ++i)
		{
			if(!hasPart(i->second->sn()))
				_parts.push_back(i->second->sn());
		}
		songChanged(SC_PART_INSERTED);
		//setCurCanvasPart(p);
	}
}/*}}}*/

void AbstractMidiEditor::addPart(Part* p)/*{{{*/
{
	if(p)
	{
		_parts.push_back(p->sn());
		songChanged(SC_PART_INSERTED);
		//setCurCanvasPart(p);
	}
}/*}}}*/

void AbstractMidiEditor::removeParts(PartList* pl)/*{{{*/
{
	if(pl)
	{
		for(iPart i = pl->begin(); i != pl->end(); ++i)
		{
			if(hasPart(i->second->sn()) && _parts.size() >= 2)
				_parts.remove(i->second->sn());
		}
		songChanged(SC_PART_REMOVED);
	}
}/*}}}*/

void AbstractMidiEditor::removePart(int sn)/*{{{*/
{
	if(_parts.size() >= 2)
	{
		_parts.remove(sn);
	}
	songChanged(SC_PART_REMOVED);
}/*}}}*/

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void AbstractMidiEditor::songChanged(int type)/*{{{*/
{
	if (type)
	{
		if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
				| SC_PART_INSERTED | SC_TRACK_REMOVED))
		{
			genPartlist();
			// close window if editor has no parts anymore
			if (parts()->empty())
			{
				close();
				return;
			}
		}
		if (canvas)
			canvas->songChanged(type);

		if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
				| SC_PART_INSERTED | SC_TRACK_REMOVED))
		{

			updateHScrollRange();
			if (canvas)
				setWindowTitle(canvas->getCaption());
			if (type & SC_SIG)
				time->update();

		}

        if (type & SC_SELECTION)
        {
                CItemList list = canvas->getSelectedItemsForCurrentPart();

                //Get the rightmost selected note (if any)
                iCItem i, iRightmost;
                CItem* rightmost = NULL;

                i = list.begin();
                while (i != list.end())
                {
                        if (i->second->isSelected())
                        {
                                iRightmost = i;
                                rightmost = i->second;
                        }

                        ++i;
                }

                if (rightmost)
                {
                        int pos = rightmost->pos().x();
                        pos = canvas->mapx(pos) + hscroll->offset();
                        int s = hscroll->offset();
                        int e = s + canvas->width();

                        if (pos > e)
                                hscroll->setOffset(rightmost->pos().x());
                        if (pos < s)
                                hscroll->setOffset(rightmost->pos().x());
                }
        }

	}
}/*}}}*/

//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void AbstractMidiEditor::setCurDrumInstrument(int instr)/*{{{*/
{
	_curDrumInstrument = instr;
	emit curDrumInstrumentChanged(_curDrumInstrument);
}/*}}}*/

//---------------------------------------------------------
//   curCanvasPart
//---------------------------------------------------------

Part* AbstractMidiEditor::curCanvasPart()/*{{{*/
{
	if (canvas)
		return canvas->part();
	else
		return 0;
}/*}}}*/

QList<Event> AbstractMidiEditor::getSelectedEvents()/*{{{*/
{
	QList<Event> rv;
	if(canvas)
	{
		CItemList list = canvas->getSelectedItemsForCurrentPart();
	
		for (iCItem k = list.begin(); k != list.end(); ++k)
		{
			NEvent* nevent = (NEvent*) (k->second);
			Event event = nevent->event();
			if (event.type() != Note)
				continue;
			
			rv.append(event);
		}
	}
	return rv;
}/*}}}*/

bool AbstractMidiEditor::isEventSelected(Event e)/*{{{*/
{
	bool rv = false;
	if(canvas)
	{
		rv = canvas->isEventSelected(e);
	}
	return rv;
}/*}}}*/

//---------------------------------------------------------
//   curWavePart
//---------------------------------------------------------

WavePart* AbstractMidiEditor::curWavePart()/*{{{*/
{
	return 0;
}/*}}}*/

//---------------------------------------------------------
//   setCurCanvasPart
//---------------------------------------------------------

void AbstractMidiEditor::setCurCanvasPart(Part* part)/*{{{*/
{
	if (canvas)
	{
		//printf("AbstractMidiEditor::setCurCanvasPart\n");
		canvas->setCurrentPart(part);
	}
}/*}}}*/

