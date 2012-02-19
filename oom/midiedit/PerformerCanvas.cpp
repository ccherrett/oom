//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: PerformerCanvas.cpp,v 1.20.2.19 2009/11/16 11:29:33 lunar_shuttle Exp $
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include <QApplication>
#include <QtGui>

#include <values.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "xml.h"
#include "PerformerCanvas.h"
#include "midiport.h"
#include "minstrument.h"
#include "midictrl.h"
#include "midi.h"
#include "event.h"
#include "mpevent.h"
#include "globals.h"
#include "cmd.h"
#include "gatetime.h"
#include "velocity.h"
#include "song.h"
#include "audio.h"
#include "gconfig.h"
#include "traverso_shared/TConfig.h"
#include "tracklistview.h"

//---------------------------------------------------------
//   NEvent
//---------------------------------------------------------

NEvent::NEvent(Event& e, Part* p, int y) : CItem(e, p)
{
	y = y - KH / 4;
	unsigned tick = e.tick() + p->tick();
	setPos(QPoint(tick, y));
	setBBox(QRect(tick, y, e.lenTick(), KH / 2));
}

//---------------------------------------------------------
//   PerformerCanvas
//---------------------------------------------------------

PerformerCanvas::PerformerCanvas(AbstractMidiEditor* pr, QWidget* parent, int sx, int sy)
: EventCanvas(pr, parent, sx, sy)
{
	colorMode = 0;
	cmdRange = 0; // all Events
	playedPitch = -1;
	_octaveQwerty = 3;
	m_globalKey = false;
    _playMoveEvent = false;

	songChanged(SC_TRACK_INSERTED);
	connect(song, SIGNAL(midiNote(int, int)), SLOT(midiNote(int, int)));

	createQWertyToMidiBindings();
}

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void PerformerCanvas::addItem(Part* part, Event& event)
{
	if (signed(event.tick()) < 0)
	{
		printf("ERROR: trying to add event before current part!\n");
		return;
	}

	NEvent* ev = new NEvent(event, part, pitch2y(event.pitch()));
	_items.add(ev);

	int diff = event.endTick() - part->lenTick();
	if (diff > 0)
	{// too short part? extend it
		//printf("addItem - this code should not be run!\n");
		//Part* newPart = part->clone();
		//newPart->setLenTick(newPart->lenTick()+diff);
		//audio->msgChangePart(part, newPart,false);
		//part = newPart;
		int endTick = song->roundUpBar(part->lenTick() + diff);
		part->setLenTick(endTick + (editor->rasterStep(endTick)*2));
	}
}

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int PerformerCanvas::pitch2y(int pitch) const
{
	int tt[] = {
		5, 12, 19, 26, 33, 44, 51, 58, 64, 71, 78, 85
	};
	int y = (75 * KH) - (tt[pitch % 12] + (7 * KH) * (pitch / 12));
	if (y < 0)
		y = 0;
	return y;
}

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int PerformerCanvas::y2pitch(int y) const
{
	const int total = (10 * 7 + 5) * KH; // 75 Ganztonschritte
	y = total - y;
	int oct = (y / (7 * KH)) * 12;
	char kt[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
		1, 1, 1, 1, 1, 1, 1, // 13
		2, 2, 2, 2, 2, 2, // 19
		3, 3, 3, 3, 3, 3, 3, // 26
		4, 4, 4, 4, 4, 4, 4, 4, 4, // 34
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 43
		6, 6, 6, 6, 6, 6, 6, // 52
		7, 7, 7, 7, 7, 7, // 58
		8, 8, 8, 8, 8, 8, 8, // 65
		9, 9, 9, 9, 9, 9, // 71
		10, 10, 10, 10, 10, 10, 10, // 78
		11, 11, 11, 11, 11, 11, 11, 11, 11, 11 // 87
	};
	return kt[y % 91] + oct;
}

void PerformerCanvas::toggleComments(bool state)
{
	m_showcomments = state;
	update();
}

void PerformerCanvas::drawOverlay(QPainter&, const QRect&)
{
}
//---------------------------------------------------------
//   drawTopItem
//---------------------------------------------------------
void PerformerCanvas::drawTopItem(QPainter& p, const QRect& rect)
{
	int x = rect.x();
	if(_curPart && m_showcomments)/*{{{*/
	{
		int  cmag = (xmag*-1)*193;
		if(cmag <= 0)
			cmag = 1;
		if(cmag > 4000)
			cmag = 4000;
		//printf("PerformerCanvas::drawCanvas(): Current xMag: %f, cMag: %d\n", xmag, cmag);
		Track* track = _curPart->track();
		if(track && track->isMidiTrack())
		{
			//printf("PerformerCanvas::drawCanvas() track is MidiTrack\n");
			MidiTrack* mtrack = (MidiTrack*)track;
			int port = mtrack->outPort();
			int channel = mtrack->outChannel();
	  		MidiInstrument* instr = midiPorts[port].instrument();
			if(instr)
			{
				MidiPort* mport = &midiPorts[port];
				int program = mport->hwCtrlState(channel, CTRL_PROGRAM);
				Patch* patch = 0;
				if (program != CTRL_VAL_UNKNOWN && program != 0xffffff)
				{
					patch = instr->getPatch(channel, program, song->mtype(), mtrack->type() == Track::DRUM);
				}

				p.setPen(config.partColors[_curPart->colorIndex()]);
				QFont font("sans-serif", 5);
				font.setWeight(QFont::Light);
				font.setStretch(cmag);
				p.setFont(font);
				for(int key = 0; key < 128; ++key)
				{
					//printf("Found key: %d ", key);
	  				KeyMap* km = instr->keymap(key);
					QString text(km->comment);
					int offset = 2;
					QString label(" ");
					bool hasComment = false;
	  				if(!km->comment.isEmpty() && km->hasProgram)
					{
						label.append(km->pname+" : "+km->comment);
						hasComment = true;
					}
					else if(!km->comment.isEmpty() && !km->hasProgram)
					{
						label.append(km->comment);
						hasComment = true;
					}
					else if(km->comment.isEmpty() && km->hasProgram)
					{
						label.append(km->pname);
						hasComment = true;
						//p.drawText(x+10, pitch2y(key)+offset, text);
					}
					if(patch && patch->comments.contains(key))
					{
						if(hasComment)
							label.append(" : ");
						label.append(patch->comments.value(key));
					}
					p.drawText(x+10, pitch2y(key)+offset, label);
				}
			}
		}
	}/*}}}*/
}

//---------------------------------------------------------
//   drawEvent
//    draws a note
//---------------------------------------------------------

void PerformerCanvas::drawItem(QPainter& p, const CItem* item, const QRect& rect)/*{{{*/
{
	QRect r = item->bbox();
	if (!virt())
	{
		r.moveCenter(map(item->pos()));
	}
	r = r.intersected(rect);
	if (!r.isValid())
	{
		return;
	}

	struct Triple
	{
		int r, g, b;
	};

	static Triple myColors /*Qt::color1*/[12] = {// ddskrjp
		{ 0xff, 0x3d, 0x39},
		{ 0x39, 0xff, 0x39},
		{ 0x39, 0x3d, 0xff},
		{ 0xff, 0xff, 0x39},
		{ 0xff, 0x3d, 0xff},
		{ 0x39, 0xff, 0xff},
		{ 0xff, 0x7e, 0x7a},
		{ 0x7a, 0x7e, 0xff},
		{ 0x7a, 0xff, 0x7a},
		{ 0xff, 0x7e, 0xbf},
		{ 0x7a, 0xbf, 0xff},
		{ 0xff, 0xbf, 0x7a}
	};

	QPen mainPen(Qt::black);
	int alpha = 195;
	int ghostedAlpha = tconfig().get_property("PerformerEdit", "renderalpha", 50).toInt();
	if(noteAlphaAction && !noteAlphaAction->isChecked())
		ghostedAlpha = 0;

	QColor colMoving;
	colMoving.setRgb(220, 220, 120, alpha);

	QPen movingPen(Qt::darkGray);

	QColor colSelected;
	colSelected.setRgb(243, 206, 105, alpha);

	NEvent* nevent = (NEvent*) item;
	Event event = nevent->event();
	if (nevent->part() != _curPart)
	{
		if (item->isMoving())
		{
			//p.setPen(movingPen);
			//p.setBrush(colMoving);
			QColor fillColor;
			fillColor = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
			fillColor.setAlpha(20);
			mainPen.setColor(fillColor);
			p.setPen(mainPen);
			
			QColor color;
			color = QColor(config.partColors[nevent->part()->colorIndex()]);
			color.setAlpha(20);
			p.setBrush(color);
		}
		else if (item->isSelected())
		{
			QColor fillColor;
			fillColor = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
			fillColor.setAlpha(alpha);
			mainPen.setColor(fillColor);
			p.setPen(mainPen);
			
			QColor color;
			color = QColor(config.partColors[nevent->part()->colorIndex()]);
			color.setAlpha(alpha);
			p.setBrush(color);
		}
		else
		{
			QColor fillColor;
			fillColor = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
			fillColor.setAlpha(ghostedAlpha);
			mainPen.setColor(fillColor);
			p.setPen(mainPen);
			
			QColor color;
			color = QColor(config.partColors[nevent->part()->colorIndex()]);
			color.setAlpha(ghostedAlpha);
			p.setBrush(color);
		}
	}
	else
	{
		if (item->isMoving())
		{
			//p.setPen(movingPen);
			//p.setBrush(colMoving);
			QColor fillColor;
			fillColor = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
			fillColor.setAlpha(20);
			mainPen.setColor(fillColor);
			p.setPen(mainPen);
			
			QColor color;
			color = QColor(config.partColors[nevent->part()->colorIndex()]);
			color.setAlpha(20);
			p.setBrush(color);
		}
		else if (item->isSelected())
		{
			QColor fillColor;
			fillColor = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
			fillColor.setAlpha(alpha);
			mainPen.setColor(fillColor);
			p.setPen(mainPen);
			
			QColor color;
			color = QColor(config.partColors[nevent->part()->colorIndex()]);
			color.setAlpha(alpha);
			p.setBrush(color);
		}
		else
		{
			QColor fillColor;
			fillColor = QColor(config.partColors[nevent->part()->colorIndex()]);
			fillColor.setAlpha(alpha);
			mainPen.setColor(fillColor);
			
			QColor color;
			color = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
			color.setAlpha(alpha);
			switch (colorMode)
			{
				case 0:
				{
					break;
				}	
				case 1: // pitch
				{
					Triple* c = &myColors/*Qt::color1*/[event.pitch() % 12];
					color.setRgb(c->r, c->g, c->b, alpha);
				}
					break;
				case 2: // velocity
				{
					int velo = event.velo();
/*{{{
					if (velo <= 11)
						color.setRgb(147, 186, 195, alpha);
					else if (velo <= 22)
						color.setRgb(119, 169, 181, alpha);
					else if (velo <= 33)
						color.setRgb(85, 157, 175, alpha);
					else if (velo <= 44)
						color.setRgb(58, 152, 176, alpha);
					else if (velo <= 55)
						color.setRgb(33, 137, 163, alpha);
					else if (velo <= 66)
						color.setRgb(30, 136, 162, alpha);
					else if (velo <= 77)
						color.setRgb(13, 124, 151, alpha);
					else if (velo <= 88)
						color.setRgb(0, 110, 138, alpha);
					else if (velo <= 99)
						color.setRgb(0, 99, 124, alpha);
					else if (velo <= 110)
						color.setRgb(0, 77, 96, alpha);
					else if (velo <= 121)
						color.setRgb(0, 69, 86, alpha);
					else
						color.setRgb(0, 58, 72, alpha);
//}}}*/
					color = QColor(config.partWaveColors[nevent->part()->colorIndex()]);
					color.setAlpha(velo+125);

				}
					break;
			}
			p.setBrush(color);
			p.setPen(mainPen);
		}
	}
	p.drawRect(r);
}/*}}}*/


//---------------------------------------------------------
//   drawMoving
//    draws moving items
//---------------------------------------------------------

void PerformerCanvas::drawMoving(QPainter& p, const CItem* item, const QRect& rect)/*{{{*/
{
	//if(((NEvent*)item)->part() != curPart)
	//  return;
	//if(!item->isMoving())
	//  return;
	QRect mr = QRect(item->mp().x(), item->mp().y() - item->height() / 2, item->width(), item->height());
	mr = mr.intersected(rect);
	if (!mr.isValid())
		return;
	//p.setPen(Qt::black);
	//p.setBrush(Qt::NoBrush);
		
	QColor fillColor;
	fillColor = QColor(config.partWaveColors[item->part()->colorIndex()]);
	fillColor.setAlpha(127);
	QPen mainPen;
	mainPen.setColor(fillColor);
	p.setPen(mainPen);
	
	QColor color;
	color = QColor(config.partColors[item->part()->colorIndex()]);
	color.setAlpha(127);
	p.setBrush(color);
	p.drawRect(mr);
}/*}}}*/

//---------------------------------------------------------
//   viewMouseDoubleClickEvent
//---------------------------------------------------------

void PerformerCanvas::viewMouseDoubleClickEvent(QMouseEvent* event)
{
	if ((_tool != PointerTool) && (event->button() != Qt::LeftButton))
	{
		mousePress(event);
		return;
	}
}

//---------------------------------------------------------
//   moveCanvasItems
//---------------------------------------------------------

void PerformerCanvas::moveCanvasItems(CItemList& items, int dp, int dx, DragType dtype, int* pflags)/*{{{*/
{
	if (editor->parts()->empty())
		return;

	PartsToChangeMap parts2change;

	int modified = 0;
	for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
	{
		Part* part = ip->second;
		if (!part)
			continue;

		int npartoffset = 0;
		for (iCItem ici = items.begin(); ici != items.end(); ++ici)
		{
			CItem* ci = ici->second;
			if (ci->part() != part)
				continue;

			int x = ci->pos().x() + dx;
			int y = pitch2y(y2pitch(ci->pos().y()) + dp);
			QPoint newpos = raster(QPoint(x, y));

			// Test moving the item...
			NEvent* nevent = (NEvent*) ci;
			Event event = nevent->event();
			x = newpos.x();
			if (x < 0)
				x = 0;
			int ntick = editor->rasterVal(x) - part->tick();
			if (ntick < 0)
				ntick = 0;
			int diff = ntick + event.lenTick() - part->lenTick();

			// If moving the item would require a new part size...
			if (diff > npartoffset)
				npartoffset = diff;
		}

		if (npartoffset > 0)
		{
			iPartToChange ip2c = parts2change.find(part);
			if (ip2c == parts2change.end())
			{
				PartToChange p2c = {0, npartoffset};
				parts2change.insert(std::pair<Part*, PartToChange > (part, p2c));
			}
			else
				ip2c->second.xdiff = npartoffset;
		}
	}

	for (iPartToChange ip2c = parts2change.begin(); ip2c != parts2change.end(); ++ip2c)
	{
		Part* opart = ip2c->first;
		int diff = ip2c->second.xdiff;

		Part* newPart = opart->clone();

		newPart->setLenTick(newPart->lenTick() + diff);

		modified = SC_PART_MODIFIED;

		// BUG FIX: #1650953
		// Added by T356.
		// Fixes posted "select and drag past end of part - crashing" bug
		for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
		{
			if (ip->second == opart)
			{
				editor->parts()->erase(ip);
				break;
			}
		}

		editor->parts()->add(newPart);
		// Indicate no undo, and do port controller values but not clone parts.
		audio->msgChangePart(opart, newPart, false, true, false);

		ip2c->second.npart = newPart;

	}

	iPartToChange icp = parts2change.find(_curPart);
	if (icp != parts2change.end())
	{
		_curPart = icp->second.npart;
		_curPartId = _curPart->sn();
		updateCItemsZValues();
	}

	std::vector< CItem* > doneList;
	typedef std::vector< CItem* >::iterator iDoneList;

    int i = 0, count = items.size();
	_playMoveEvent = (items.selectionCount() == 1);
	for (iCItem ici = items.begin(); ici != items.end(); ++ici, ++i)
	{
		CItem* ci = ici->second;
        //_playMoveEvent = (i == 0);

		// If this item's part is in the parts2change list, change the item's part to the new part.
		Part* pt = ci->part();
		iPartToChange ip2c = parts2change.find(pt);
		if (ip2c != parts2change.end())
			ci->setPart(ip2c->second.npart);

		int x = ci->pos().x();
		int y = ci->pos().y();
		int nx = x + dx;
		int ny = pitch2y(y2pitch(y) + dp);
		QPoint newpos = raster(QPoint(nx, ny));
		selectItem(ci, true);

		iDoneList idl;
		for (idl = doneList.begin(); idl != doneList.end(); ++idl)
			// This compares EventBase pointers to see if they're the same...
			if ((*idl)->event() == ci->event())
				break;

		// Do not process if the event has already been processed (meaning it's an event in a clone part)...
		//if(moveItem(ci, newpos, dtype))
		if (idl != doneList.end())
			// Just move the canvas item.
			ci->move(newpos);
		else
		{
			//Populate the multiselect list for this citem
			if(editor->isGlobalEdit())
				populateMultiSelect(ci);
			// Currently moveItem always returns true.
			if (moveItem(ci, newpos, dtype))
			{
				// Add the canvas item to the list of done items.
				doneList.push_back(ci);
				// Move the canvas item.
				ci->move(newpos);
			}
		}

		if (_moving.size() == 1 || i == count-1)
			itemReleased(_curItem, newpos);
		if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
			selectItem(ci, false);
	}
    _playMoveEvent = false;

	if (pflags)
		*pflags = modified;
}/*}}}*/

//---------------------------------------------------------
//   moveItem
//    called after moving an object
//---------------------------------------------------------

// Changed by T356.
//bool PerformerCanvas::moveItem(CItem* item, const QPoint& pos, DragType dtype, int* pflags)

bool PerformerCanvas::moveItem(CItem* item, const QPoint& pos, DragType dtype)/*{{{*/
{
	NEvent* nevent = (NEvent*) item;
	Event event = nevent->event();
	int npitch = y2pitch(pos.y());
	int pitchdiff = npitch - event.pitch();
	//printf("moveItem: pitch; %d, npitch: %d, diff: %d\n", event.pitch(), npitch, pitchdiff);
	Event newEvent = event.clone();
	int x = pos.x();
	if (x < 0)
		x = 0;

    // replace note if needed
	if (playedPitch != npitch && _playEvents && _playMoveEvent)
	{
		int port = track()->outPort();
		int channel = track()->outChannel();
		// release note:
		MidiPlayEvent ev1(0, port, channel, 0x90, playedPitch, 0, (Track*)track());
		audio->msgPlayMidiEvent(&ev1);
		MidiPlayEvent ev2(0, port, channel, 0x90, npitch + track()->getTransposition(), event.velo(), (Track*)track());
		audio->msgPlayMidiEvent(&ev2);
        playedPitch = npitch + track()->getTransposition();
	}

	Part* part = nevent->part(); 

	newEvent.setPitch(npitch);
	int ntick = editor->rasterVal(x) - part->tick();
	if (ntick < 0)
		ntick = 0;
	newEvent.setTick(ntick);
	newEvent.setLenTick(event.lenTick());

	// Added by T356.
	// msgAddEvent and msgChangeEvent (below) will set these, but set them here first?
	item->setEvent(newEvent);

	// Added by T356.
	if (((int) newEvent.endTick() - (int) part->lenTick()) > 0)
		printf("PerformerCanvas::moveItem Error! New event end:%d exceeds length:%d of part:%s\n", newEvent.endTick(), part->lenTick(), part->name().toLatin1().constData());

	song->startUndo();
	if (dtype == MOVE_COPY || dtype == MOVE_CLONE)
	{
		// Indicate no undo, and do not do port controller values and clone parts.
		audio->msgAddEvent(newEvent, part, false, false, false);
		if(editor->isGlobalEdit() && !m_multiSelect.empty())/*{{{*/
		{
			for(iCItem i = m_multiSelect.begin(); i != m_multiSelect.end(); ++i)
			{
				NEvent* nevent2 = (NEvent*) i->second;
				Event event2 = nevent2->event();
				Event cloneEv = event2.clone();
				Part* evpart = nevent2->part(); 
				int cloneTick = editor->rasterVal(x) - evpart->tick();
				if (cloneTick < 0)
					cloneTick = 0;
				cloneEv.setTick(cloneTick);
				cloneEv.setPitch(event2.pitch()+pitchdiff);
				audio->msgAddEvent(cloneEv, nevent2->part(), false, false, false);
			}
		}/*}}}*/
	}
	else
	{
		// Indicate no undo, and do not do port controller values and clone parts.
		audio->msgChangeEvent(event, newEvent, part, false, false, false);
		if(editor->isGlobalEdit() && !m_multiSelect.empty())/*{{{*/
		{
			for(iCItem i = m_multiSelect.begin(); i != m_multiSelect.end(); ++i)
			{
				NEvent* nevent2 = (NEvent*) i->second;
				Event event2 = nevent2->event();
				Event cloneEv = event2.clone();
				Part* evpart = nevent2->part(); 
				int cloneTick = editor->rasterVal(x) - evpart->tick();
				if (cloneTick < 0)
					cloneTick = 0;
				cloneEv.setTick(cloneTick);
				//TODO: Error check here to make sure we are not above 127 or -126
				cloneEv.setPitch(event2.pitch()+pitchdiff);
				audio->msgChangeEvent(event2, cloneEv, nevent2->part(), false, false, false);
			}
		}/*}}}*/
	}
	song->endUndo(0);
	emit pitchChanged(newEvent.pitch());

	return true;
}/*}}}*/

//---------------------------------------------------------
//   newItem(p, state)
//---------------------------------------------------------

CItem* PerformerCanvas::newItem(const QPoint& p, int)/*{{{*/
{
	//printf("newItem point\n");
	int pitch = y2pitch(p.y());
	int tick = editor->rasterVal1(p.x());
	int len = p.x() - tick;
	int etick = tick - _curPart->tick();
	if (etick < 0)
		etick = 0;
	Event e = Event(Note);
	e.setTick(etick);
	e.setPitch(pitch);
	e.setVelo(curVelo);
	e.setLenTick(len);
	int transp = ((MidiTrack*)_curPart->track())->getTransposition();
	//Populate epic mode showdow notes
	if(editor->isGlobalEdit())
	{
		//printf("Populating list for new Items\n");
		PartList* pl = editor->parts();
		m_multiSelect.clear();
		for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			Part* part = ip->second;
			if(part == _curPart)
				continue;
			MidiTrack* track = (MidiTrack*)part->track();
			int evpitch = (pitch + track->getTransposition())-transp;
			int evtick = tick - part->tick();
			Event ev = Event(Note);
			ev.setTick(evtick);
			ev.setPitch(evpitch);
			ev.setVelo(curVelo);
			ev.setLenTick(len);
			m_multiSelect.add(new NEvent(ev, part, pitch2y(evpitch)));
		}
	}
    return new NEvent(e, _curPart, pitch2y(pitch));
}/*}}}*/

void PerformerCanvas::newItem(CItem* item, bool noSnap)/*{{{*/
{
	//printf("newItem citem\n");
	NEvent* nevent = (NEvent*) item;
	Event event = nevent->event();
	int x = item->x();
	if (x < 0)
		x = 0;
	int w = item->width();

	if (!noSnap)
	{
		x = editor->rasterVal1(x); //round down
		w = editor->rasterVal(x + w) - x;
		if (w == 0)
			w = editor->raster();
	}
	Part* part = nevent->part();
	event.setTick(x - part->tick());
	event.setLenTick(w);
	event.setPitch(y2pitch(item->y()));

	song->startUndo();
	int modified = SC_EVENT_MODIFIED;
	int diff = event.endTick() - part->lenTick();
	if (diff > 0)
	{// too short part? extend it
		Part* newPart = part->clone();
		newPart->setLenTick(newPart->lenTick() + diff);
		// Indicate no undo, and do port controller values but not clone parts.
		audio->msgChangePart(part, newPart, false, true, false);
		modified = modified | SC_PART_MODIFIED;
		part = newPart; 
	}
	// Indicate no undo, and do not do port controller values and clone parts.
	audio->msgAddEvent(event, part, false, false, false);
	//Add in the duplicate events
	if(editor->isGlobalEdit() && !m_multiSelect.empty())/*{{{*/
	{
		//printf("Adding multipart events");
		for(iCItem i = m_multiSelect.begin(); i != m_multiSelect.end(); ++i)
		{
			CItem* ni = i->second;
			ni->setWidth(item->width());
			NEvent* nevent2 = (NEvent*) ni;
			Part* npart = nevent2->part();
			Event ev = nevent2->event();
			ev.setTick(x - npart->tick());
			ev.setLenTick(w);
			ev.setPitch(y2pitch(ni->y()));


			int diff2 = ev.endTick() - npart->lenTick();
			if (diff2 > 0)
			{// too short part? extend it
				Part* newPart2 = npart->clone();
				newPart2->setLenTick(newPart2->lenTick() + diff2);
				audio->msgChangePart(npart, newPart2, false, true, false);
				if(!modified & SC_PART_MODIFIED)
					modified = modified | SC_PART_MODIFIED;
				npart = newPart2; 
			}

			audio->msgAddEvent(ev, npart, false, false, false);
			_items.add(ni);
		}
	}/*}}}*/
	emit pitchChanged(event.pitch());
	song->endUndo(modified);
}/*}}}*/

//---------------------------------------------------------
//   resizeItem
//---------------------------------------------------------

void PerformerCanvas::resizeItem(CItem* item, bool noSnap) // experimental changes to try dynamically extending parts/*{{{*/
{
	//printf("ComposerCanvas::resizeItem!\n");
	NEvent* nevent = (NEvent*) item;
	Event event = nevent->event();
	Event newEvent = event.clone();
	int len;

	Part* part = nevent->part();

	if (noSnap)
		len = nevent->width();
	else
	{
		unsigned tick = event.tick() + part->tick();
		len = editor->rasterVal(tick + nevent->width()) - tick;
		if (len <= 0)
			len = editor->raster();
	}
	song->startUndo();
	int modified = SC_EVENT_MODIFIED;
	//printf("event.tick()=%d len=%d part->lenTick()=%d\n",event.endTick(),len,part->lenTick());
	int diff = event.tick() + len - part->lenTick();
	if (diff > 0)
	{// too short part? extend it
		Part* newPart = part->clone();
		newPart->setLenTick(newPart->lenTick() + diff);
		audio->msgChangePart(part, newPart, false, true, false);
		modified = modified | SC_PART_MODIFIED;
		part = newPart; 
	}

	newEvent.setLenTick(len);
	// Indicate no undo, and do not do port controller values and clone parts.
	audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);

	if(editor->isGlobalEdit() && !m_multiSelect.empty())/*{{{*/
	{
		for(iCItem i = m_multiSelect.begin(); i != m_multiSelect.end(); ++i)
		{
			NEvent* nevent2 = (NEvent*) i->second;
			Event event2 = nevent2->event();
			Event newEvent2 = event2.clone();

			Part* npart = nevent2->part();

			int diff2 = event2.tick() + len - npart->lenTick();
			if (diff2 > 0)
			{// too short part? extend it
				Part* newPart2 = npart->clone();
				newPart2->setLenTick(newPart2->lenTick() + diff2);
				audio->msgChangePart(npart, newPart2, false, true, false);
				if(!modified & SC_PART_MODIFIED)
					modified = modified | SC_PART_MODIFIED;
				npart = newPart2; 
			}

			newEvent2.setLenTick(len);
			audio->msgChangeEvent(event2, newEvent2, nevent2->part(), false, false, false);
		}
	}/*}}}*/
	song->endUndo(modified);
}/*}}}*/

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

bool PerformerCanvas::deleteItem(CItem* item)/*{{{*/
{
	NEvent* nevent = (NEvent*) item;
	//if (nevent->part() == _curPart)
	//{
		Event ev = nevent->event();
		song->startUndo();
		// Indicate do undo, and do not do port controller values and clone parts.
		//audio->msgDeleteEvent(ev, curPart);
		audio->msgDeleteEvent(ev, nevent->part(), false, false, false);
		
		if(editor->isGlobalEdit() && !m_multiSelect.empty())/*{{{*/
		{
			for(iCItem i = m_multiSelect.begin(); i != m_multiSelect.end(); ++i)
			{
				NEvent* nevent2 = (NEvent*) i->second;
				Event event2 = nevent2->event();
				audio->msgDeleteEvent(event2, nevent2->part(), false, false, false);
			}
			//FIXME: We should probably clear the list after this operation
		}/*}}}*/
		song->endUndo(0);
		return true;
	//}
	//return false;
}/*}}}*/

//---------------------------------------------------------
//   pianoCmd
//---------------------------------------------------------

void PerformerCanvas::pianoCmd(int cmd)/*{{{*/
{
	switch (cmd)
	{
		case CMD_LEFT:
		{
			int spos = _pos[0];
			if (spos > 0)
			{
				spos -= 1; // Nudge by -1, then snap down with raster1.
				spos = AL::sigmap.raster1(spos, editor->rasterStep(_pos[0]));
			}
			if (spos < 0)
				spos = 0;
			Pos p(spos, true);
			song->setPos(0, p, true, true, true);
		}
			break;
		case CMD_RIGHT:
		{
			int spos = AL::sigmap.raster2(_pos[0] + 1, editor->rasterStep(_pos[0])); // Nudge by +1, then snap up with raster2.
			Pos p(spos, true);
			song->setPos(0, p, true, true, true);
		}
			break;
		case CMD_LEFT_NOSNAP:
		{
			int spos = _pos[0] - editor->rasterStep(_pos[0]);
			if (spos < 0)
				spos = 0;
			Pos p(spos, true);
			song->setPos(0, p, true, true, true); //CDW
		}
			break;
		case CMD_RIGHT_NOSNAP:
		{
			Pos p(_pos[0] + editor->rasterStep(_pos[0]), true);
			//if (p > part->tick())
			//      p = part->tick();
			song->setPos(0, p, true, true, true); //CDW
		}
			break;
		case CMD_INSERT:
		{
			if (_pos[0] < start() || _pos[0] >= end())
				break;
			MidiPart* part = (MidiPart*) _curPart;

			if (part == 0)
				break;
			song->startUndo();
			EventList* el = part->events();

			std::list <Event> elist;
			for (iEvent e = el->lower_bound(_pos[0] - part->tick()); e != el->end(); ++e)
				elist.push_back((Event) e->second);
			for (std::list<Event>::iterator i = elist.begin(); i != elist.end(); ++i)
			{
				Event event = *i;
				Event newEvent = event.clone();
				newEvent.setTick(event.tick() + editor->raster()); // - part->tick());
				// Indicate no undo, and do not do port controller values and clone parts.
				//audio->msgChangeEvent(event, newEvent, part, false);
				audio->msgChangeEvent(event, newEvent, part, false, false, false);
			}
			song->endUndo(SC_EVENT_MODIFIED);
			Pos p(editor->rasterVal(_pos[0] + editor->rasterStep(_pos[0])), true);
			song->setPos(0, p, true, false, true);
		}
			return;
		case CMD_DELETE:
			if (_pos[0] < start() || _pos[0] >= end())
				break;
		{
			MidiPart* part = (MidiPart*) _curPart;
			if (part == 0)
				break;
			song->startUndo();
			EventList* el = part->events();

			std::list<Event> elist;
			for (iEvent e = el->lower_bound(_pos[0]); e != el->end(); ++e)
				elist.push_back((Event) e->second);
			for (std::list<Event>::iterator i = elist.begin(); i != elist.end(); ++i)
			{
				Event event = *i;
				Event newEvent = event.clone();
				newEvent.setTick(event.tick() - editor->raster() - part->tick());
				// Indicate no undo, and do not do port controller values and clone parts.
				//audio->msgChangeEvent(event, newEvent, part, false);
				audio->msgChangeEvent(event, newEvent, part, false, false, false);
			}
			song->endUndo(SC_EVENT_MODIFIED);
			Pos p(editor->rasterVal(_pos[0] - editor->rasterStep(_pos[0])), true);
			song->setPos(0, p, true, false, true);
		}
			break;
	}
}/*}}}*/

//---------------------------------------------------------
//   pianoPressed
//---------------------------------------------------------

void PerformerCanvas::pianoPressed(int pitch, int velocity, bool shift)/*{{{*/
{
	int port = track()->outPort();
	int channel = track()->outChannel();
	int ipitch = pitch;
	ipitch += track()->getTransposition();

	// play note:
	MidiPlayEvent e(0, port, channel, 0x90, ipitch, velocity, (Track*)track());
	audio->msgPlayMidiEvent(&e);

    if (_steprec && _pos[0] >= start_tick && _pos[0] < end_tick)
	{
		if (_curPart == 0)
			return;
		int len = editor->raster();
		unsigned tick = _pos[0];// - _curPart->tick(); //CDW
		if (shift)
			tick -= editor->rasterStep(tick);
		if(m_globalKey)
		{
			PartList* pl = editor->parts();
			for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
			{
				Part* part = ip->second;
				MidiTrack* ctrack = (MidiTrack*)part->track();
				int tpitch = pitch;
				tpitch += ctrack->getTransposition();
				Event e(Note);
				e.setTick(tick);
				e.setPitch(tpitch);
				e.setVelo(127);
				e.setLenTick(len);
				// Indicate do undo, and do not do port controller values and clone parts.
				audio->msgAddEvent(e, part, true, false, false);
			}
		}
		else
		{
			pitch += track()->getTransposition();

			Event e(Note);
			e.setTick(tick);
			e.setPitch(pitch);
			e.setVelo(127);
			e.setLenTick(len);
			// Indicate do undo, and do not do port controller values and clone parts.
			audio->msgAddEvent(e, _curPart, true, false, false);
		}
		tick += editor->rasterStep(tick) + _curPart->tick();
		unsigned int t = tick;
		//t += (editor->rasterStep(t)/2); <-I think this is a mistake
		t += (editor->rasterStep(t)*2);
		if(song->len() <= (tick + editor->rasterStep(tick)))
		{
			t += editor->rasterStep(t);
			song->setLen(t);
		}
		if (tick != song->cpos())
		{
			Pos p(tick, true);
			song->setPos(0, p, true, false, true);
		}
	}

}/*}}}*/

//---------------------------------------------------------
//   pianoReleased
//---------------------------------------------------------

void PerformerCanvas::pianoReleased(int pitch, bool)/*{{{*/
{
	if(m_globalKey)
	{
		PartList* pl = editor->parts();
		for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			Part* part = ip->second;
			MidiTrack* ctrack = (MidiTrack*)part->track();
			int tpitch = pitch;
			tpitch += ctrack->getTransposition();
			int port = ctrack->outPort();
			int channel = ctrack->outChannel();

			MidiPlayEvent e(0, port, channel, 0x90, tpitch, 0, (Track*)track());
			audio->msgPlayMidiEvent(&e);
		}
	}
	else
	{
		int port = track()->outPort();
		int channel = track()->outChannel();
		pitch += track()->getTransposition();

	// release key:
		MidiPlayEvent e(0, port, channel, 0x90, pitch, 0, (Track*)track());
		audio->msgPlayMidiEvent(&e);
	}
}/*}}}*/

//---------------------------------------------------------
//   drawTickRaster
//---------------------------------------------------------

void drawTickRaster(QPainter& p, int x, int y, int w, int h, int raster, bool ctrl)/*{{{*/
{

	QColor colBeat;
	QColor colBar1;
	QColor colBar2;
	if(ctrl)
	{
		colBar1.setRgb(94,96,97);
		colBar2.setRgb(82,83,84);
		colBeat.setRgb(72,73,74);
	}
	else
	{
		colBar1.setRgb(94,96,97);
		colBar2.setRgb(82,83,84);
		colBeat.setRgb(69,70,71);
		
		/*colBar1.setRgb(104,106,107);
		colBar2.setRgb(89,91,92);
		colBeat.setRgb(83,86,87);
		*/
		//colBeat.setRgb(210, 216, 219);
		//colBar1.setRgb(82, 85, 87);
		//colBar2.setRgb(150, 160, 167);
	}


	int bar1, bar2, beat;
	unsigned tick;
	AL::sigmap.tickValues(x, &bar1, &beat, &tick);
	AL::sigmap.tickValues(x + w, &bar2, &beat, &tick);
	++bar2;
	int y2 = y + h;
	for (int bar = bar1; bar < bar2; ++bar)
	{
		unsigned x = AL::sigmap.bar2tick(bar, 0, 0);
		p.setPen(colBar1);
		p.drawLine(x, y, x, y2);
		int z, n;
		AL::sigmap.timesig(x, z, n);
		///int q = p.xForm(QPoint(raster, 0)).x() - p.xForm(QPoint(0, 0)).x();
		int q = p.combinedTransform().map(QPoint(raster, 0)).x() - p.combinedTransform().map(QPoint(0, 0)).x();
		int qq = raster;
		if (q < 8) // grid too dense
			qq *= 2;
		//switch (quant) {
		//      case 32:
		//      case 48:
		//      case 64:
		//      case 96:
		//      case 192:         // 8tel
		//      case 128:         // 8tel Triolen
		//      case 288:
		p.setPen(colBeat);
		if (raster >= 4)
		{
			int xx = x + qq;
			int xxx = AL::sigmap.bar2tick(bar, z, 0);
			while (xx <= xxx)
			{
				p.drawLine(xx, y, xx, y2);
				xx += qq;
			}
			xx = xxx;
		}
		//            break;
		//      default:
		//            break;
		// }
		p.setPen(colBar2);
		for (int beat = 1; beat < z; beat++)
		{
			int xx = AL::sigmap.bar2tick(bar, beat, 0);
			p.drawLine(xx, y, xx, y2);
		}

	}
}/*}}}*/

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PerformerCanvas::drawCanvas(QPainter& p, const QRect& rect)/*{{{*/
{
	int x = rect.x();
	int y = rect.y();
	int w = rect.width();
	int h = rect.height();

	//---------------------------------------------------
	//  horizontal lines
	//---------------------------------------------------

	int yy = ((y - 1) / KH) * KH + KH;
	int key = 75 - (yy / KH);
	QColor blackNoteColor = QColor(56,56,56);
	for (; yy < y + h; yy += KH)
	{
		//printf("Drawing lines\n");
		switch (key % 7)
		{
			case 0:
			case 3:
				p.setPen(blackNoteColor);
				p.drawLine(x, yy, x + w, yy);
				break;
			default:
				//p.setPen(lightGray);
				//p.fillRect(x, yy-3, w, 6, QBrush(QColor(230,230,230)));
				p.fillRect(x, yy - 3, w, 6, QBrush(blackNoteColor));
				//p.drawLine(x, yy, x + w, yy);
				break;
		}
		--key;
	}

	//---------------------------------------------------
	// vertical lines
	//---------------------------------------------------

	drawTickRaster(p, x, y, w, h, editor->raster(), false);
}/*}}}*/

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void PerformerCanvas::cmd(int cmd, int quantStrength, int quantLimit, bool quantLen, int range)/*{{{*/
{
	cmdRange = range;
	//printf("PerformerCanvas cmd called with command: %d\n\n", cmd);
	switch (cmd)
	{
		case CMD_CUT:
			copy();
			song->startUndo();
			for (iCItem i = _items.begin(); i != _items.end(); ++i)
			{
				if (!(i->second->isSelected()))
					continue;
				NEvent* e = (NEvent*) (i->second);
				Event ev = e->event();
				// Indicate no undo, and do not do port controller values and clone parts.
				audio->msgDeleteEvent(ev, e->part(), false, false, false);
			}
			song->endUndo(SC_EVENT_REMOVED);
			break;
		case CMD_COPY:
			copy();
			break;
		case CMD_PASTE:
			paste();
			break;
		case CMD_DEL:
			if (selectionSize())
			{
				//song->startUndo();
				if(!_items.empty())
				{
					QList<CItem*> toBeDeleted;
					for (iCItem i = _items.begin(); i != _items.end(); ++i)
					{
						if (!i->second->isSelected())
							continue;
						CItem* item = i->second;
						toBeDeleted.append(item);
						//populateMultiSelect(item);
						//deleteItem(item);
						//Event ev = i->second->event();
						// Indicate no undo, and do not do port controller values and clone parts.
						//audio->msgDeleteEvent(ev, i->second->part(), false, false, false);
					}
					while(toBeDeleted.size())
					{
						CItem* item = toBeDeleted.takeFirst();
						populateMultiSelect(item);
						deleteItem(item);
					}
				}
				//song->endUndo(SC_EVENT_REMOVED);
			}
			return;
		case CMD_OVER_QUANTIZE: // over quantize
			quantize(100, 1, quantLen);
			break;
		case CMD_ON_QUANTIZE: // note on quantize
			quantize(50, 1, false);
			break;
		case CMD_ONOFF_QUANTIZE: // note on/off quantize
			quantize(50, 1, true);
			break;
		case CMD_ITERATIVE_QUANTIZE: // Iterative Quantize
			quantize(quantStrength, quantLimit, quantLen);
			break;
		case CMD_SELECT_ALL: // select all
		{
			// get a list of items that belong to the current part
			// since (if) multiple parts have populated the _items list
			// we need to filter on the actual current Part!
			CItemList list = _items;
			if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
				list = getItemlistForCurrentPart();
			
			for (iCItem k = list.begin(); k != list.end(); ++k)
			{
				if (!k->second->isSelected())
					selectItem(k->second, true);
			}
		}
	     break;
		case CMD_SELECT_NONE: // select none
			deselectAll();
			break;
		case CMD_SELECT_INVERT: // invert selection
	    	for (iCItem k = _items.begin(); k != _items.end(); ++k)
			{
				selectItem(k->second, !k->second->isSelected());
			}
			break;
		case CMD_SELECT_ILOOP: // select inside loop
	    	for (iCItem k = _items.begin(); k != _items.end(); ++k)
			{
				NEvent* nevent = (NEvent*) (k->second);
				Part* part = nevent->part();
				Event event = nevent->event();
				unsigned tick = event.tick() + part->tick();
				if (tick < song->lpos() || tick >= song->rpos())
					selectItem(k->second, false);
				else
					selectItem(k->second, true);
			}
			break;
		case CMD_SELECT_OLOOP: // select outside loop
	    	for (iCItem k = _items.begin(); k != _items.end(); ++k)
			{
				NEvent* nevent = (NEvent*) (k->second);
				Part* part = nevent->part();
				Event event = nevent->event();
				unsigned tick = event.tick() + part->tick();
				if (tick < song->lpos() || tick >= song->rpos())
					selectItem(k->second, true);
				else
					selectItem(k->second, false);
			}
			break;
		case CMD_SELECT_PREV_PART: // select previous part
		{
			Part* pt = editor->curCanvasPart();
			Track* cTrack = pt->track();
			Part* newpt = pt;
			PartList* pl = editor->parts();
			QList<Track*> editorTracks = pl->tracks();
			int listSize = (editorTracks.size() - 1);
			int trackIndex = editorTracks.indexOf(pt->track());
			PartMap partMap = pl->partMap(cTrack);
			PartList* plist = partMap.parts;
			for (iPart ip = plist->begin(); ip != plist->end(); ++ip)
			{
				if (ip->second == pt)
				{
					if (ip == plist->begin() && !listSize)
					{
						ip = plist->end();
					}
					else if(ip == plist->begin())
					{
						int next = listSize;
						if(trackIndex > 0)
						{
							next = (trackIndex - 1);
						}
						PartMap map = pl->partMap(editorTracks.at(next));
						if(map.parts)
						{
							iPart ip2 = map.parts->end();
							--ip2;
							newpt = ip2->second;
							break;
						}
					}
					--ip;
					newpt = ip->second;
					break;
				}
			}
			if (newpt != pt)
			{
				// turn of record flag for the currents part track
				if (_curPart)
				{
					song->setRecordFlag(track(), false);
				}
				PartList *pl = editor->parts();
				if(pl && !pl->empty())
				{
					for(iPart p = pl->begin(); p != pl->end(); ++p)
					{
						Track* t = p->second->track();
						if(t != track() && t != newpt->track())
							song->setRecordFlag(t, false);
					}
				}
				editor->setCurCanvasPart(newpt);
				// and turn it on for the new parts track
				song->setRecordFlag(track(), true);
				song->deselectTracks();
				song->deselectAllParts();
				track()->setSelected(true);
				newpt->setSelected(true);
				song->update(SC_SELECTION);
				Song::movePlaybackToPart(newpt);
			}
		}
			break;
		case CMD_SELECT_NEXT_PART: // select next part
		{
			Part* pt = editor->curCanvasPart();
			Track* cTrack = pt->track();
			Part* newpt = pt;
			PartList* pl = editor->parts();
			QList<Track*> editorTracks = pl->tracks();
			int listSize = (editorTracks.size() - 1);
			int trackIndex = editorTracks.indexOf(pt->track());
			PartMap partMap = pl->partMap(cTrack);
			PartList* plist = partMap.parts;
			for (iPart ip = plist->begin(); ip != plist->end(); ++ip)
			{
				if (ip->second == pt)
				{
					++ip;
					if (ip == plist->end() && !listSize)
					{
						ip = plist->begin();
					}
					else if(ip == plist->end())
					{
						int next = 0;
						if(trackIndex < listSize)
						{
							next = (trackIndex + 1);
						}
						PartMap map = pl->partMap(editorTracks.at(next));
						if(map.parts)
						{
							iPart ip2 = map.parts->begin();
							newpt = ip2->second;
							break;
						}
					}
					newpt = ip->second;
					break;
				}
			}
			if (newpt != pt)
			{
				// turn of record flag for the currents part track
				if (_curPart)
				{
					song->setRecordFlag(track(), false);
				}
				PartList *pl = editor->parts();
				if(pl && !pl->empty())
				{
					for(iPart p = pl->begin(); p != pl->end(); ++p)
					{
						Track* t = p->second->track();
						if(t != track() && t != newpt->track())
							song->setRecordFlag(t, false);
					}
				}
				editor->setCurCanvasPart(newpt);
				// and turn it on for the new parts track
				song->setRecordFlag(track(), true);
				song->deselectTracks();
				song->deselectAllParts();
				track()->setSelected(true);
				newpt->setSelected(true);
				song->update(SC_SELECTION);
				Song::movePlaybackToPart(newpt);
			}
		}
			break;
		case CMD_MODIFY_GATE_TIME:
		{
			GateTime w(this);
			w.setRange(range);
			if (!w.exec())
				break;
			int range = w.range(); // all, selected, looped, sel+loop
			int rate = w.rateVal();
			int offset = w.offsetVal();

			song->startUndo();
			for (iCItem k = _items.begin(); k != _items.end(); ++k)
			{
				NEvent* nevent = (NEvent*) (k->second);
				Event event = nevent->event();
				if (event.type() != Note)
					continue;
				unsigned tick = event.tick();
				bool selected = k->second->isSelected();
				bool inLoop = (tick >= song->lpos()) && (tick < song->rpos());

				if ((range == 0)
						|| (range == 1 && selected)
						|| (range == 2 && inLoop)
						|| (range == 3 && selected && inLoop))
				{
					unsigned int len = event.lenTick(); //prevent compiler warning: comparison singed/unsigned

					len = rate ? (len * 100) / rate : 1;
					len += offset;
					if (len < 1)
						len = 1;

					if (event.lenTick() != len)
					{
						Event newEvent = event.clone();
						newEvent.setLenTick(len);
						// Indicate no undo, and do not do port controller values and clone parts.
						//audio->msgChangeEvent(event, newEvent, nevent->part(), false);
						audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
					}
				}
			}
			song->endUndo(SC_EVENT_MODIFIED);
		}
			break;

		case CMD_MODIFY_VELOCITY:
		{
			Velocity w;
			w.setRange(range);
			if (!w.exec())
				break;
			int range = w.range(); // all, selected, looped, sel+loop
			int rate = w.rateVal();
			int offset = w.offsetVal();

			song->startUndo();
	    	for (iCItem k = _items.begin(); k != _items.end(); ++k)
			{
				NEvent* nevent = (NEvent*) (k->second);
				Event event = nevent->event();
				if (event.type() != Note)
					continue;
				unsigned tick = event.tick();
				bool selected = k->second->isSelected();
				bool inLoop = (tick >= song->lpos()) && (tick < song->rpos());

				if ((range == 0)
						|| (range == 1 && selected)
						|| (range == 2 && inLoop)
						|| (range == 3 && selected && inLoop))
				{
					int velo = event.velo();

					//velo = rate ? (velo * 100) / rate : 64;
					velo = (velo * rate) / 100;
					velo += offset;

					if (velo <= 0)
						velo = 1;
					if (velo > 127)
						velo = 127;
					if (event.velo() != velo)
					{
						Event newEvent = event.clone();
						newEvent.setVelo(velo);
						// Indicate no undo, and do not do port controller values and clone parts.
						//audio->msgChangeEvent(event, newEvent, nevent->part(), false);
						audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
					}
				}
			}
			song->endUndo(SC_EVENT_MODIFIED);
		}
			break;

		case CMD_FIXED_LEN: //Set notes to the length specified in the drummap
			if (!selectionSize())
				break;
			song->startUndo();
			for (iCItem k = _items.begin(); k != _items.end(); ++k)
			{
				if (k->second->isSelected())
				{
					NEvent* nevent = (NEvent*) (k->second);
					Event event = nevent->event();
					Event newEvent = event.clone();
					newEvent.setLenTick(editor->raster());
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgChangeEvent(event, newEvent, nevent->part() , false);
					audio->msgChangeEvent(event, newEvent, nevent->part(), false, false, false);
				}
			}
			song->endUndo(SC_EVENT_MODIFIED);
			break;

		case CMD_DELETE_OVERLAPS:
			if (!selectionSize())
				break;

			song->startUndo();
	    	for (iCItem k = _items.begin(); k != _items.end(); k++)
			{
				if (k->second->isSelected() == false)
					continue;

				NEvent* e1 = (NEvent*) (k->second); // first note
				NEvent* e2 = NULL; // ptr to next selected note (which will be checked for overlap)
				Event ce1 = e1->event();
				Event ce2;

				if (ce1.type() != Note)
					continue;

				// Find next selected item on the same pitch
				iCItem l = k;
				l++;
				for (; l != _items.end(); l++)
				{
					if (l->second->isSelected() == false)
						continue;

					e2 = (NEvent*) l->second;
					ce2 = e2->event();

					// Same pitch?
					if (ce1.dataA() == ce2.dataA())
						break;

					// If the note has the same len and place we treat it as a duplicate note and not a following note
					// The best thing to do would probably be to delete the duplicate note, we just want to avoid
					// matching against the same note
					if (ce1.tick() + e1->part()->tick() == ce2.tick() + e2->part()->tick()
							&& ce1.lenTick() + e1->part()->tick() == ce2.lenTick() + e2->part()->tick())
					{
						e2 = NULL; // this wasn't what we were looking for
						continue;
					}

				}

				if (e2 == NULL) // None found
					break;

				Part* part1 = e1->part();
				Part* part2 = e2->part();
				if (ce2.type() != Note)
					continue;


				unsigned event1pos = ce1.tick() + part1->tick();
				unsigned event1end = event1pos + ce1.lenTick();
				unsigned event2pos = ce2.tick() + part2->tick();

				//printf("event1pos %u event1end %u event2pos %u\n", event1pos, event1end, event2pos);
				if (event1end > event2pos)
				{
					Event newEvent = ce1.clone();
					unsigned newlen = ce1.lenTick() - (event1end - event2pos);
					//printf("newlen: %u\n", newlen);
					newEvent.setLenTick(newlen);
					// Indicate no undo, and do not do port controller values and clone parts.
					//audio->msgChangeEvent(ce1, newEvent, e1->part(), false);
					audio->msgChangeEvent(ce1, newEvent, e1->part(), false, false, false);
				}
			}
			song->endUndo(SC_EVENT_MODIFIED);
			break;


		case CMD_CRESCENDO:
		case CMD_TRANSPOSE:
		case CMD_THIN_OUT:
		case CMD_ERASE_EVENT:
		case CMD_NOTE_SHIFT:
		case CMD_MOVE_CLOCK:
		case CMD_COPY_MEASURE:
		case CMD_ERASE_MEASURE:
		case CMD_DELETE_MEASURE:
		case CMD_CREATE_MEASURE:
			break;
		default:
			//                  printf("unknown EventCanvas cmd %d\n", cmd);
			break;
	}
	updateSelection();
	redraw();
}/*}}}*/

//---------------------------------------------------------
//   quantize
//---------------------------------------------------------

void PerformerCanvas::quantize(int strength, int limit, bool quantLen)/*{{{*/
{
	song->startUndo();
    for (iCItem k = _items.begin(); k != _items.end(); ++k)
	{
		NEvent* nevent = (NEvent*) (k->second);
		Event event = nevent->event();
		Part* part = nevent->part();
		if (event.type() != Note)
			continue;

		if ((cmdRange & CMD_RANGE_SELECTED) && !k->second->isSelected())
			continue;

		unsigned tick = event.tick() + part->tick();

		if ((cmdRange & CMD_RANGE_LOOP)
				&& ((tick < song->lpos() || tick >= song->rpos())))
			continue;

		unsigned int len = event.lenTick(); //prevent compiler warning: comparison singed/unsigned
		int tick2 = tick + len;

		// quant start position
		int diff = AL::sigmap.raster(tick, editor->quant()) - tick;
		if (abs(diff) > limit)
			tick += ((diff * strength) / 100);

		// quant len
		diff = AL::sigmap.raster(tick2, editor->quant()) - tick2;
		if (quantLen && (abs(diff) > limit))
			len += ((diff * strength) / 100);

		// something changed?
		if (((event.tick() + part->tick()) != tick) || (event.lenTick() != len))
		{
			Event newEvent = event.clone();
			newEvent.setTick(tick - part->tick());
			newEvent.setLenTick(len);
			// Indicate no undo, and do not do port controller values and clone parts.
			//audio->msgChangeEvent(event, newEvent, part, false);
			audio->msgChangeEvent(event, newEvent, part, false, false, false);
		}
	}
	song->endUndo(SC_EVENT_MODIFIED);
}/*}}}*/


void PerformerCanvas::createQWertyToMidiBindings()/*{{{*/
{
	_qwertyToMidiMap.clear();

	/* Lower keyboard row - "zxcvbnm". */
	bindQwertyKeyToMidiValue("z", 12);	/* C-1 */
	bindQwertyKeyToMidiValue("s", 13);
	bindQwertyKeyToMidiValue("x", 14);
	bindQwertyKeyToMidiValue("d", 15);
	bindQwertyKeyToMidiValue("c", 16);
	bindQwertyKeyToMidiValue("v", 17);
	bindQwertyKeyToMidiValue("g", 18);
	bindQwertyKeyToMidiValue("b", 19);
	bindQwertyKeyToMidiValue("h", 20);
	bindQwertyKeyToMidiValue("n", 21);
	bindQwertyKeyToMidiValue("j", 22);
	bindQwertyKeyToMidiValue("m", 23);

	/* Upper keyboard row, first octave - "qwertyu". */
	bindQwertyKeyToMidiValue("q", 24);
	bindQwertyKeyToMidiValue("2", 25);
	bindQwertyKeyToMidiValue("w", 26);
	bindQwertyKeyToMidiValue("3", 27);
	bindQwertyKeyToMidiValue("e", 28);
	bindQwertyKeyToMidiValue("r", 29);
	bindQwertyKeyToMidiValue("5", 30);
	bindQwertyKeyToMidiValue("t", 31);
	bindQwertyKeyToMidiValue("6", 32);
	bindQwertyKeyToMidiValue("y", 33);
	bindQwertyKeyToMidiValue("7", 34);
	bindQwertyKeyToMidiValue("u", 35);
}/*}}}*/

void PerformerCanvas::setOctaveQwerty(int octave)
{
	_octaveQwerty = octave + 1;
}

void PerformerCanvas::bindQwertyKeyToMidiValue(const char* key, int note)
{
	_qwertyToMidiMap.insert(key, note);
}


int PerformerCanvas::stepInputQwerty(QKeyEvent *event)
{
	//const char* key = event->text().toAscii().data();
	int pitch = _qwertyToMidiMap.value(event->text(), -1);

	if (pitch == -1)
	{
		return 0;
	}

	pitch += _octaveQwerty * 12;

	midiNote(pitch, 100);

	return 1;
}


//---------------------------------------------------------
//   midiNote
//---------------------------------------------------------

void PerformerCanvas::midiNote(int pitch, int velo)/*{{{*/
{
	unsigned songtick = song->cpos();
	//Filter the noteoff events/*{{{*/
	if(velo)
	{
		if(m_globalKey)
		{
			//Handle any keyswitching at this stage
			PartList* pl = editor->parts();
			for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
			{
				Part* part = ip->second;
				processKeySwitches(part, pitch, songtick);
			}
		}
		else
		{
			processKeySwitches(_curPart, pitch, songtick);
		}
	}/*}}}*/
	//printf("PerformerCanvas::midiNote(pitch:%d, velo:%d) \n", pitch, velo);
    if (_midiin && _steprec && _curPart && !audio->isPlaying() && velo && _pos[0] >= start_tick && !(globalKeyState & Qt::AltModifier))
	{
		//MidiTrack* ctrack = (MidiTrack*)_curPart->track();
		//int tpitch = pitch;
		//if(ctrack)
		//	tpitch = tpitch + ctrack->transposition;

		unsigned tick = _pos[0]; //CDW
		//Process all parts if in EPIC mode
		if(m_globalKey)
		{
			PartList* pl = editor->parts();
			for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
			{
				Part* part = ip->second;
				MidiTrack* ctrack = (MidiTrack*)part->track();
				int tpitch = pitch;
				if(ctrack)
					tpitch = tpitch + ctrack->transposition;
				stepInputNote(part, songtick, tpitch, velo);
			}
		}
		else
		{
			stepInputNote(_curPart, songtick, pitch, velo);
		}
		/*unsigned int len = editor->quant(); //prevent compiler warning: comparison singed/unsigned//{{{
		unsigned starttick = tick;
		if (globalKeyState & Qt::ShiftModifier)
			tick -= editor->rasterStep(tick);

		//
		// extend len of last note?
		//
		EventList* events = _curPart->events();
		if (globalKeyState & Qt::ControlModifier)
		{
			for (iEvent i = events->begin(); i != events->end(); ++i)
			{
				Event ev = i->second;
				if (!ev.isNote())
					continue;
				if (ev.pitch() == pitch && ((ev.tick() + ev.lenTick()) == starttick))
				{
					Event e = ev.clone();
					e.setLenTick(ev.lenTick() + editor->rasterStep(starttick));
					// Indicate do undo, and do not do port controller values and clone parts.
					//audio->msgChangeEvent(ev, e, curPart);
				    audio->msgChangeEvent(ev, e, _curPart, true, false, false);
					itemPressed(new CItem(e, _curPart));
					//TODO: emit a signal to flash keyboard here
					emit pitchChanged(pitch);
					tick += editor->rasterStep(tick);
					if (tick != song->cpos())
					{
						Pos p(tick, true);
						song->setPos(0, p, true, false, true);
					}
					return;
				}
			}
		}

		// if we already entered the note, delete it
		//
		EventRange range = events->equal_range(tick);
		for (iEvent i = range.first; i != range.second; ++i)
		{
			Event ev = i->second;
			if (ev.isNote() && ev.pitch() == pitch)
			{
				// Indicate do undo, and do not do port controller values and clone parts.
				//audio->msgDeleteEvent(ev, curPart);
				audio->msgDeleteEvent(ev, _curPart, true, false, false);
				if (globalKeyState & Qt::ShiftModifier)
					tick += editor->rasterStep(tick);
				return;
			}
		}
		Event e(Note);
		e.setTick(tick - _curPart->tick());
		e.setPitch(pitch);
		e.setVelo(velo);
		e.setLenTick(len);
		// Indicate do undo, and do not do port controller values and clone parts.
		//audio->msgAddEvent(e, curPart);
		audio->msgAddEvent(e, _curPart, true, false, false);
		itemPressed(new CItem(e, _curPart));//}}}*/
		tick += editor->rasterStep(tick);
		//printf("Song length %d current tick %d\n", song->len(), tick);
		unsigned int t = tick;
		t += (editor->rasterStep(t)*2);
		if(song->len() <= (tick + editor->rasterStep(tick)))
		{
			t += editor->rasterStep(t);
			song->setLen(t);
		}
		if (tick != song->cpos())
		{
			Pos p(tick, true);
			song->setPos(0, p, true, false, true);
		}
	}

	update();
	
	//TODO: emit a signal to flash keyboard here
	emit pitchChanged(pitch);
}/*}}}*/

void PerformerCanvas::stepInputNote(Part* part, unsigned tick, int pitch, int velo)
{
	unsigned int len = editor->quant(); //prevent compiler warning: comparison singed/unsigned/*{{{*/
	//unsigned tick = _pos[0]; //CDW
	unsigned starttick = tick;
	if (globalKeyState & Qt::ShiftModifier)
		tick -= editor->rasterStep(tick);

	//
	// extend len of last note?
	//
	EventList* events = part->events();
	if (globalKeyState & Qt::ControlModifier)
	{
		for (iEvent i = events->begin(); i != events->end(); ++i)
		{
			Event ev = i->second;
			if (!ev.isNote())
				continue;
			if (ev.pitch() == pitch && ((ev.tick() + ev.lenTick()) == /*(int)*/starttick))
			{
				Event e = ev.clone();
				e.setLenTick(ev.lenTick() + editor->rasterStep(starttick));
				// Indicate do undo, and do not do port controller values and clone parts.
				//audio->msgChangeEvent(ev, e, curPart);
			    audio->msgChangeEvent(ev, e, part, true, false, false);
				//itemPressed(new CItem(e, part));
				//TODO: emit a signal to flash keyboard here
				emit pitchChanged(pitch);
				/*tick += editor->rasterStep(tick);
				if (tick != song->cpos())
				{
					Pos p(tick, true);
					song->setPos(0, p, true, false, true);
				}*/
				return;
			}
		}
	}/*}}}*/
	///*{{{*/
	// if we already entered the note, delete it
	//
	EventRange range = events->equal_range(tick);
	for (iEvent i = range.first; i != range.second; ++i)
	{
		Event ev = i->second;
		if (ev.isNote() && ev.pitch() == pitch)
		{
			// Indicate do undo, and do not do port controller values and clone parts.
			//audio->msgDeleteEvent(ev, curPart);
			audio->msgDeleteEvent(ev, part, true, false, false);
			if (globalKeyState & Qt::ShiftModifier)
				tick += editor->rasterStep(tick);
			return;
		}
	}
	Event e(Note);
	e.setTick(tick - part->tick());
	e.setPitch(pitch);
	e.setVelo(velo);
	e.setLenTick(len);
	// Indicate do undo, and do not do port controller values and clone parts.
	audio->msgAddEvent(e, part, true, false, false);
	//itemPressed(new CItem(e, part));/*}}}*/
}

void PerformerCanvas::recordArmAll()
{
	PartList* pl = editor->parts();
	for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
	{
		Part* part = ip->second;
		Track* track = part->track();
		track->setRecordFlag1(true);
		track->setRecordFlag2(true);
		//printf("Record arm: %s\n", track->name().toUtf8().constData());
	}
	song->update(SC_RECFLAG);
}

void PerformerCanvas::globalTransposeClicked(bool state)/*{{{*/
{
	PartList* pl = editor->parts();
	for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
	{
		Part* part = ip->second;
		Track* track = part->track();
		MidiTrack* mtrack = (MidiTrack*)track;
		mtrack->transpose = state;
		//printf("global transpose track: %s - tp: %d \n", mtrack->name().toUtf8().constData(), state);
	}
	song->update(SC_MIDI_TRACK_PROP);
}/*}}}*/

void PerformerCanvas::processKeySwitches(Part* part, int pitch, int songtick)/*{{{*/
{
	MidiTrack* track = (MidiTrack*)part->track();
	int port = track->outPort();
	int channel = track->outChannel();
	MidiInstrument* instr = midiPorts[port].instrument();
	if(instr)
	{
		if(instr->hasMapping(pitch))
		{
			KeyMap *km = instr->keymap(pitch);
			//printf("Recieved pianoPressed found mapping, program: %d, pr: %d, test: %d\n", km->program, pr, 0xff);
			if (km->hasProgram)
			{
				int diff = songtick - part->lenTick();
				if (diff > 0)
				{
					int endTick = song->roundUpBar(part->lenTick() + diff);
					part->setLenTick(endTick);
				}
				
				//printf("Should be adding the program now\n");
				MidiPlayEvent ev(0, port, channel, ME_CONTROLLER, CTRL_PROGRAM, km->program, (Track*)track);
				audio->msgPlayMidiEvent(&ev);

				MidiPort* mport = &midiPorts[port];
				int program = mport->hwCtrlState(channel, CTRL_PROGRAM);
				//Check it again to make sure it was processed
				if (program != CTRL_VAL_UNKNOWN && program != 0xff)
				{
					Event a(Controller);
					a.setTick(songtick);
					a.setA(CTRL_PROGRAM);
					a.setB(program);

					//song->recordEvent(track, a);
					song->recordEvent((MidiPart*)part, a);
				}
			}
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void PerformerCanvas::copy()
{
	//QDrag* drag = getTextDrag();
	QMimeData* drag = getTextDrag();

	if (drag)
		QApplication::clipboard()->setMimeData(drag, QClipboard::Clipboard);
}

//---------------------------------------------------------
//   paste
//    paste events
//---------------------------------------------------------

void PerformerCanvas::paste()
{
	QString stype("x-oom-eventlist");

	QString s = QApplication::clipboard()->text(stype, QClipboard::Clipboard); // TODO CHECK Tim.

	pasteAt(s, song->cpos());
}

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void PerformerCanvas::startDrag(CItem* /* item*/, bool copymode)
{
	QMimeData* md = getTextDrag();

	if (md)
	{
		// "Note that setMimeData() assigns ownership of the QMimeData object to the QDrag object.
		//  The QDrag must be constructed on the heap with a parent QWidget to ensure that Qt can
		//  clean up after the drag and drop operation has been completed. "
		QDrag* drag = new QDrag(this);
		drag->setMimeData(md);

		if (copymode)
			drag->exec(Qt::CopyAction);
		else
			drag->exec(Qt::MoveAction);
	}
}

//---------------------------------------------------------
//   dragEnterEvent
//---------------------------------------------------------

void PerformerCanvas::dragEnterEvent(QDragEnterEvent* event)
{
	///event->accept(Q3TextDrag::canDecode(event));
	event->acceptProposedAction(); // TODO CHECK Tim.
}

//---------------------------------------------------------
//   dragMoveEvent
//---------------------------------------------------------

void PerformerCanvas::dragMoveEvent(QDragMoveEvent*)
{
	//printf("drag move %x\n", this);
	//event->acceptProposedAction();
}

//---------------------------------------------------------
//   dragLeaveEvent
//---------------------------------------------------------

void PerformerCanvas::dragLeaveEvent(QDragLeaveEvent*)
{
	//printf("drag leave\n");
	//event->acceptProposedAction();
}

void PerformerCanvas::selectLasso(bool toggle)
{
	CItemList curPartItems = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		curPartItems = getItemlistForCurrentPart();

	int n = 0;
	if (virt())
	{
		for (iCItem i = curPartItems.begin(); i != curPartItems.end(); ++i)
		{
			if (i->second->intersects(_lasso))
			{
				selectItem(i->second, !(toggle && i->second->isSelected()));
				++n;
			}
		}
	}
	else
	{
		for (iCItem i = curPartItems.begin(); i != curPartItems.end(); ++i)
		{
			QRect box = i->second->bbox();
			int x = rmapxDev(box.x());
			int y = rmapyDev(box.y());
			int w = rmapxDev(box.width());
			int h = rmapyDev(box.height());
			QRect r(x, y, w, h);
			///r.moveBy(i->second->pos().x(), i->second->pos().y());
			r.translate(i->second->pos().x(), i->second->pos().y());
			if (r.intersects(_lasso))
			{
				selectItem(i->second, !(toggle && i->second->isSelected()));
				++n;
			}
		}
	}



	if (n)
	{
		updateSelection();
		redraw();
	}
}


/*
//---------------------------------------------------------
//   dropEvent
//---------------------------------------------------------

void PerformerCanvas::viewDropEvent(QDropEvent* event)
	  {
	  QString text;
	  if (event->source() == this) {
			printf("local DROP\n");   // REMOVE Tim
			//event->acceptProposedAction();
			//event->ignore();                     // TODO CHECK Tim.
			return;
			}
	  ///if (Q3TextDrag::decode(event, text)) {
	  //if (event->mimeData()->hasText()) {
	  if (event->mimeData()->hasFormat("text/x-oom-eventlist")) {

			//text = event->mimeData()->text();
			text = QString(event->mimeData()->data("text/x-oom-eventlist"));

			int x = editor->rasterVal(event->pos().x());
			if (x < 0)
				  x = 0;
			pasteAt(text, x);
			//event->accept();  // TODO
			}
	  else {
			printf("cannot decode drop\n");
			//event->acceptProposedAction();
			//event->ignore();                     // TODO CHECK Tim.
			}
	  }
 */

//---------------------------------------------------------
//   itemPressed
//---------------------------------------------------------

void PerformerCanvas::itemPressed(const CItem* item)
{
	if (!_playEvents)
		return;

	int port = track()->outPort();
	int channel = track()->outChannel();
	NEvent* nevent = (NEvent*) item;
	Event event = nevent->event();
	playedPitch = event.pitch(); + track()->getTransposition();
	int velo = event.velo();

	// play note:
	MidiPlayEvent e(0, port, channel, 0x90, playedPitch, velo, (Track*)track());
	audio->msgPlayMidiEvent(&e);
}

//---------------------------------------------------------
//   itemReleased
//---------------------------------------------------------

void PerformerCanvas::itemReleased(const CItem*, const QPoint&)
{
	if (!_playEvents)
		return;
	int port = track()->outPort();
	int channel = track()->outChannel();

	// release note:
	MidiPlayEvent ev(0, port, channel, 0x90, playedPitch, 0, (Track*)track());
	audio->msgPlayMidiEvent(&ev);
	playedPitch = -1;
}

//---------------------------------------------------------
//   itemMoved
//---------------------------------------------------------

void PerformerCanvas::itemMoved(const CItem* item, const QPoint& pos)
{
    if (!_playEvents || (_moving.size() > 0 && _moving.begin()->second != item))
		return;
    int npitch = y2pitch(pos.y());
	if ((playedPitch != -1) && (playedPitch != npitch))
	{
		int port = track()->outPort();
		int channel = track()->outChannel();
		NEvent* nevent = (NEvent*) item;
		Event event = nevent->event();

		// release note:
		MidiPlayEvent ev1(0, port, channel, 0x90, playedPitch, 0, (Track*)track());
		audio->msgPlayMidiEvent(&ev1);
		// play note:
		MidiPlayEvent e2(0, port, channel, 0x90, npitch + track()->getTransposition(), event.velo(), (Track*)track());
		audio->msgPlayMidiEvent(&e2);
		playedPitch = npitch + track()->getTransposition();
	}
}

//---------------------------------------------------------
//   curPartChanged
//---------------------------------------------------------

void PerformerCanvas::curPartChanged()
{
	editor->setWindowTitle("The Performer:     "+getCaption());
	emit partChanged(editor->curCanvasPart());
}

//---------------------------------------------------------
// doModify
//---------------------------------------------------------

void PerformerCanvas::doModify(NoteInfo::ValType type, int delta, CItem* item, bool play)/*{{{*/
{
    if(item)
	{
		NEvent* e = (NEvent*) (item);
		Event event = e->event();
		if (event.type() != Note)
			return;

		MidiPart* part = (MidiPart*) (e->part());
		Event newEvent = event.clone();

		switch (type)
		{
			case NoteInfo::VAL_TIME:
			{
				int newTime = event.tick() + delta;
				if (newTime < 0)
					newTime = 0;
				newEvent.setTick(newTime);
			}
				break;
			case NoteInfo::VAL_LEN:
			{
				int len = event.lenTick() + delta;
				if (len < 1)
					len = 1;
				newEvent.setLenTick(len);
			}
				break;
			case NoteInfo::VAL_VELON:
			{
				int velo = event.velo() + delta;
				if (velo > 127)
					velo = 127;
				else if (velo < 0)
					velo = 0;
				newEvent.setVelo(velo);
			}
				break;
			case NoteInfo::VAL_VELOFF:
			{
				int velo = event.veloOff() + delta;
				if (velo > 127)
					velo = 127;
				else if (velo < 0)
					velo = 0;
				newEvent.setVeloOff(velo);
			}
				break;
			case NoteInfo::VAL_PITCH:
			{
				int pitch = event.pitch() + delta;
				if (pitch > 127)
					pitch = 127;
				else if (pitch < 0)
					pitch = 0;
				newEvent.setPitch(pitch);
			}
				break;
		}
		int epitch = event.pitch();
		song->changeEvent(event, newEvent, part);
		emit pitchChanged(newEvent.pitch());
		//This is a vertical movement
		if(_playEvents && newEvent.pitch() != epitch && play)
		{
			int port = track()->outPort();
			int channel = track()->outChannel();

			// release key:
			MidiPlayEvent pe2(0, port, channel, 0x90, epitch, 0, (Track*)track());
			audio->msgPlayMidiEvent(&pe2);
			MidiPlayEvent pe(0, port, channel, 0x90, newEvent.pitch(), newEvent.velo(), (Track*)track());
			audio->msgPlayMidiEvent(&pe);
			MidiPlayEvent pe3(0, port, channel, 0x90, newEvent.pitch(), 0, (Track*)track());
			audio->msgPlayMidiEvent(&pe3);
		}
		// Indicate do not do port controller values and clone parts.
		//song->undoOp(UndoOp::ModifyEvent, newEvent, event, part);
		song->undoOp(UndoOp::ModifyEvent, newEvent, event, part, false, false);
	}
}/*}}}*/

//---------------------------------------------------------
//   modifySelected
//---------------------------------------------------------

void PerformerCanvas::modifySelected(NoteInfo::ValType type, int delta, bool strict)/*{{{*/
{
	audio->msgIdle(true);
	song->startUndo();
	//i use this to make sure I only sound a note once if you are
	//moving multiple notes at once
	CItemList list = _items;
	if(strict)
		list = getSelectedItemsForCurrentPart();
	int count = list.selectionCount();
    for (iCItem i = list.begin(); i != list.end(); ++i)
	{
		if (!(i->second->isSelected()))
			continue;
		CItem* item = i->second;
		if(editor->isGlobalEdit())
			populateMultiSelect(item);
		doModify(type, delta, item, (count == 1));
		
		if(editor->isGlobalEdit())
		{
			for(iCItem ci = m_multiSelect.begin(); ci != m_multiSelect.end(); ++ci)
			{
				CItem* citem = ci->second;
				doModify(type, delta, citem, false);
			}
		}
	}
	song->endUndo(SC_EVENT_MODIFIED);
	audio->msgIdle(false);
}/*}}}*/

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void PerformerCanvas::resizeEvent(QResizeEvent* ev)
{
	if (ev->size().width() != ev->oldSize().width())
		emit newWidth(ev->size().width());
	EventCanvas::resizeEvent(ev);
}

bool PerformerCanvas::isEventSelected(Event e)/*{{{*/
{
	bool rv = false;
	CItemList list = getSelectedItemsForCurrentPart();

	for (iCItem k = list.begin(); k != list.end(); ++k)
	{
		NEvent* nevent = (NEvent*) (k->second);
		Event event = nevent->event();
		if (event.type() != Note)
			continue;

		if (event == e)
		{
			rv = true;
			break;
		}
	}
	return rv;
}/*}}}*/
