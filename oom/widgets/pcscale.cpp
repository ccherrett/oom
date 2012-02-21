//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: mtscale.cpp,v 1.8.2.7 2009/05/03 04:14:01 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <values.h>

#include <QMouseEvent>
#include <QPainter>
#include <QMenu>
#include <QAction>

#include "pcscale.h"
#include "song.h"
#include "icons.h"
#include "gconfig.h"
#include "globals.h"
#include "PerformerCanvas.h"
#include "audio.h"
#include "midiport.h"
#include "conductor/Conductor.h"
#include "instrumentmenu.h"

//---------------------------------------------------------
//   PCScale
//    Midi Time Scale
//---------------------------------------------------------

PCScale::PCScale(int* r, QWidget* parent, Performer* editor, int xs, bool _mode)
: View(parent, xs, 1)
{
	//audio = 0;
	currentEditor = editor;
	waveMode = _mode;
	setToolTip(tr("bar pcscale"));
	barLocator = false;
	raster = r;
	m_clickpos = 0;
	if (waveMode)
	{
		pos[0] = tempomap.tick2frame(song->cpos());
		pos[1] = tempomap.tick2frame(song->lpos());
		pos[2] = tempomap.tick2frame(song->rpos());
	}
	else
	{
		pos[0] = song->cpos();
		pos[1] = song->lpos();
		pos[2] = song->rpos();
	}
	pos[3] = MAXINT; // do not show
	button = Qt::NoButton;
	setMouseTracking(true);
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), SLOT(setPos(int, unsigned, bool)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

	setFixedHeight(14);
	//setBg(QColor(110, 141, 152));
	setBg(QColor(18,18,18));
	_pc.state = doNothing;
	_pc.valid = false;
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void PCScale::songChanged(int type)
{
	if (type & (SC_SIG | SC_TEMPO))
	{
		if ((type & SC_TEMPO) && waveMode)
		{
			pos[0] = tempomap.tick2frame(song->cpos());
			pos[1] = tempomap.tick2frame(song->lpos());
			pos[2] = tempomap.tick2frame(song->rpos());
		}
		redraw();
	}
	redraw();
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void PCScale::setPos(int idx, unsigned val, bool)
{
	if (val == MAXINT)
	{
		if (idx == 3)
		{
			pos[3] = MAXINT;
			redraw(QRect(0, 0, width(), height()));
		}
		return;
	}
	if (waveMode)
		val = tempomap.tick2frame(val);
	if (val == pos[idx])
		return;
	//unsigned opos = mapx(pos[idx] == MAXINT ? val : pos[idx]);
	int opos = mapx(pos[idx] == MAXINT ? val : pos[idx]);
	pos[idx] = val;
	if (!isVisible())
		return;

	int tval = mapx(val);
	int x = -9;
	int w = 18;

	if (tval < 0)
	{ // tval<0 occurs whenever the window is scrolled left, so I switched to signed int (ml)
		//printf("PCScale::setPos - idx:%d val:%d tval:%d opos:%d w:%d h:%d\n", idx, val, tval, opos, width(), height());

		redraw(QRect(0, 0, width(), height()));
		return;
	}
	//if (opos > (unsigned int) tval) {	//prevent compiler warning: comparison signed/unsigned
	if (opos > tval)
	{
		w += opos - tval;
		x += tval;
	}
	else
	{
		w += tval - opos;
		x += opos;
	}
	//printf("PCScale::setPos idx:%d val:%d tval:%d opos:%d x:%d w:%d h:%d\n", idx, val, tval, opos, x, w, height());

	redraw(QRect(x, 0, w, height()));
}

void PCScale::deleteProgramChangeClicked(bool checked)
{
	if(!checked)
		return
	deleteProgramChange(_pc.event);/*{{{*/
	_pc.valid = false;

	song->startUndo();
	audio->msgDeleteEvent(_pc.event, _pc.part, true, true, false);
	song->endUndo(SC_EVENT_MODIFIED);
	if(currentEditor->isGlobalEdit() && !selectionList.isEmpty())
	{
		foreach(ProgramChangeObject pco, selectionList)
		{
			song->startUndo();
			audio->msgDeleteEvent(pco.event, pco.part, true, true, false);
			song->endUndo(SC_EVENT_MODIFIED);
		}
	}
	selectionList.clear();
	update();/*}}}*/
}

void PCScale::changeProgramChangeClicked(int patch, QString)
{
	Event nevent = _pc.event.clone();
	if(debugMsg)
		printf("Patch selected: %d - event.dataA(): %d - event.dataB(): %d\n", patch, nevent.dataA(), nevent.dataB());
	nevent.setB(patch);
	audio->msgChangeEvent(_pc.event, nevent, _pc.part, true, true, false);
	_pc.event = nevent;
	if(currentEditor->isGlobalEdit() && !selectionList.isEmpty())/*{{{*/
	{
		foreach(ProgramChangeObject pco, selectionList)
		{
			Event nevent1 = pco.event.clone();
			nevent1.setB(patch);
			audio->msgChangeEvent(pco.event, nevent1, pco.part, true, true, false);
			pco.event = nevent1;
		}
	}/*}}}*/
	_pc.valid = true;
	_pc.state = doNothing;
	selectionList.clear();
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void PCScale::viewMousePressEvent(QMouseEvent* event)
{
	button = event->button();
	//viewMouseMoveEvent(event);
	if (event->modifiers() & Qt::ShiftModifier)
		setCursor(QCursor(Qt::PointingHandCursor));
	else
		setCursor(QCursor(Qt::ArrowCursor));

	int x = event->x();
	if (x < 0)
	{
		x = 0;
	}
	m_clickpos = x;

	int i;
	switch (button)
	{
		case Qt::LeftButton:
			i = 0;
			break;
		case Qt::MidButton:
			i = 1;
			break;
		case Qt::RightButton:
			i = 2;
			break;
		default:
			return; // if no button is pressed the function returns here
	}
	Pos p(x, true);
	if (waveMode)
	{
		song->setPos(i, p);
		return;
	}

	if (i == 0 && (event->modifiers() & Qt::ShiftModifier))
	{ // If shift +LMB we add a marker
		//Add program change here
		song->setPos(i, p); // all other cases: relocating one of the locators
		unsigned utick = song->cpos() + currentEditor->rasterStep(song->cpos());
		if(currentEditor->isGlobalEdit())
		{
			for (iPart ip = currentEditor->parts()->begin(); ip != currentEditor->parts()->end(); ++ip)
			{
				Part* part = ip->second;
				emit addProgramChange(part, utick);
			}
		}
		else
		{
			emit addProgramChange(currentEditor->curCanvasPart(), utick);
		}
	}
	/*else if (i == 2 && (event->modifiers() & Qt::ShiftModifier))
	{ // If shift +RMB we remove a marker
		//Delete Program change here
		if (selectProgramChange(x))
		{
			deleteProgramChange(_pc.event);
			_pc.valid = false;

			song->startUndo();
			audio->msgDeleteEvent(_pc.event, _pc.part, true, true, false);
			song->endUndo(SC_EVENT_MODIFIED);
			update();
			return;
		}
	}*/
	else if(i == 2)
	{//popup a menu here to copy/paste PC
		if(_pc.valid && _pc.state == selectedController)
		{
			//update the song position before we start so we have a position for paste
			song->setPos(0, p);
			QMenu* menu = new QMenu(this);
			QAction *cp = menu->addAction(tr("Paste Program Change Here."));
			cp->setCheckable(true);
			connect(cp, SIGNAL(triggered(bool)), this, SLOT(copySelected(bool)));
			cp->setData(1);
			QAction *del = menu->addAction(tr("Delete Selected."));
			del->setCheckable(true);
			connect(del, SIGNAL(triggered(bool)), this, SLOT(deleteProgramChangeClicked(bool)));
			del->setData(2);
			int outPort = ((MidiTrack*)_pc.part->track())->outPort();
			MidiInstrument* instr = midiPorts[outPort].instrument();
			if(instr)
			{
				QMenu* menu2 = new QMenu(tr("Change Patch"), this);
				InstrumentMenu *imenu = new InstrumentMenu(menu2, instr);
				menu2->addAction(imenu);
				connect(imenu, SIGNAL(patchSelected(int, QString)), this, SLOT(changeProgramChangeClicked(int, QString)));
				menu->addMenu(menu2);
			}
			menu->exec(event->globalPos(), 0);
		}
	}
	else if (i == 0 && (event->modifiers() & Qt::ControlModifier))/*{{{*/
	{ // If LMB select the program change
		if (selectProgramChange(x))
		{
			return;
		}
		//We did not find a program change so just set song position
		song->setPos(i, p);
	}/*}}}*/
	else if (i == 0)/*{{{*/
	{ // If LMB select the program change
		if (selectProgramChange(x) && !_pc.event.empty())
		{
			Event nevent = _pc.event.clone();

			audio->msgDeleteEvent(_pc.event, _pc.part, false, true, false);
			update();

			_pc.event = nevent;
			_pc.event.setTick(x);

			_pc.state = movingController;
			_pc.valid = true;

			emit drawSelectedProgram(_pc.event.tick(), true);

			return;
		}
		//We did not find a program change so just set song position
		song->setPos(i, p);
	}/*}}}*/
	else
		song->setPos(i, p); // all other cases: relocating one of the locators

	update();
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void PCScale::viewMouseReleaseEvent(QMouseEvent* event)
{

	if(_pc.valid && _pc.state == movingController && _pc.part && button == Qt::LeftButton)
	{
		int x = event->x();
		x = AL::sigmap.raster(x, *raster);
		if (x < 0)
		{
			x = 0;
		}

		if((unsigned int)x >= _pc.part->tick())
		{
			int diff = _pc.event.tick() - _pc.part->lenTick();
			if (diff > 0)
			{// too short part? extend it
				int endTick = song->roundUpBar(_pc.part->lenTick() + diff);
				_pc.part->setLenTick(endTick);
				if(song->len() <= (unsigned int)endTick)
				{
					song->setLen((unsigned int)endTick);
				}
			}
			song->recordEvent((MidiPart*)_pc.part, _pc.event);
		}
		if(currentEditor->isGlobalEdit() && !selectionList.isEmpty())/*{{{*/
		{
			foreach(ProgramChangeObject pco, selectionList)
			{
				if((unsigned int)x < pco.part->tick())
					continue;
				Event nevent1 = pco.event.clone();
				nevent1.setTick(x);
				int diff1 = nevent1.tick() - pco.part->lenTick();
				if (diff1 > 0)
				{// too short part? extend it
					int pendTick = song->roundUpBar(pco.part->lenTick() + diff1);
					pco.part->setLenTick(pendTick);
				}
				audio->msgChangeEvent(pco.event, nevent1, pco.part, true, true, false);
				pco.event = nevent1;
			}
		}/*}}}*/
	}

	//Deselect the PC only if was a end of move
	if(button == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier))
	{
		if(_pc.valid && _pc.state == movingController)
		{
			//Move all other events on the other parts to the new location of the event
			selectionList.clear();
			_pc.state = doNothing;
			_pc.valid = false;
			emit drawSelectedProgram(-1, false);
		}
	}


	button = Qt::NoButton;
	update();
}

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void PCScale::viewMouseMoveEvent(QMouseEvent* event)/*{{{*/
{
	//printf("PCScale::viewMouseMoveEvent()\n");
	if(_pc.valid && _pc.state == movingController && _pc.part && button == Qt::LeftButton)
	{
		int x = event->x();
		x = AL::sigmap.raster(x, *raster);
		if (x < 0)
			x = 0;
		//int tick = pc.event.tick() + pc.part->tick();
		if((unsigned int)x < _pc.part->tick())
			return;
		_pc.event.setTick(x);
		int diff = _pc.event.tick() - _pc.part->lenTick();
		if (diff > 0)
		{// too short part? extend it
			int endTick = song->roundUpBar(_pc.part->lenTick() + diff);
			_pc.part->setLenTick(endTick);
			if(song->len() <= (unsigned int)endTick)
			{
				song->setLen((unsigned int)endTick);
			}
		}
		emit drawSelectedProgram(_pc.event.tick(), true);
		/*if(currentEditor->isGlobalEdit() && !selectionList.isEmpty())
		{
			foreach(ProgramChangeObject pco, selectionList)
			{
				if((unsigned int)x < pco.part->tick())
					continue;
				Event nevent1 = pco.event.clone();
				nevent1.setTick(x);
				int diff1 = nevent1.tick() - pco.part->lenTick();
				if (diff1 > 0)
				{// too short part? extend it
					int pendTick = song->roundUpBar(pco.part->lenTick() + diff1);
					pco.part->setLenTick(pendTick);
				}
				audio->msgChangeEvent(pco.event, nevent1, pco.part, true, true, false);
				pco.event = nevent1;
			}
		}*/
		update();
	}
}/*}}}*/

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PCScale::leaveEvent(QEvent*)
{
	//emit timeChanged(MAXINT);
}

void PCScale::setEditor(Performer* editor)
{
	currentEditor = editor;
}

/**
 * updateProgram
 * Called to redraw the view after a program change was added
 */
void PCScale::updateProgram()
{
	redraw();
}

/**
 * selectProgramChange
 * called the select the programchange at the current song position
 */
bool PCScale::selectProgramChange(int x)/*{{{*/
{
	if(currentEditor->isGlobalEdit())
	{
		Part* curpart = currentEditor->curCanvasPart();
		if (!curpart)
		{
			return false;
		}
		selectionList.clear();
		for (iPart ip = currentEditor->parts()->begin(); ip != currentEditor->parts()->end(); ++ip)
		{
			Part* part = ip->second;
		
			QList<Event> events;
		
			EventList* eventList = part->events();
			for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
			{
				//Get event type.
				Event pcevt = evt->second;
				if (!pcevt.isNote() && pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
				{
					int xp = pcevt.tick() + part->tick();
					int diff = abs(xp - x);
					if (diff < 300)
					{
						events.append(pcevt);
					}
				}
			}
		
			if (!events.size())
			{
				continue;
				//return false;
			}
		
			Event nearest;
		
			int minDiff = INT_MAX;
			foreach(Event event, events)
			{
				int diff = abs((event.tick() + part->tick()) - x);
				if (diff < minDiff) {
					minDiff = diff;
					nearest = event;
				}
			}
			if(part == curpart)
			{
				_pc.part = part;
				_pc.event = nearest;
				_pc.valid = true;
				_pc.state = selectedController;
			}
			else
			{
				ProgramChangeObject pco;
				pco.part = part;
				pco.event = nearest;
				pco.valid = true;
				selectionList.append(pco);
			}
		}
	}
	else
	{
		Part* part = currentEditor->curCanvasPart();/*{{{*/
		if (!part)
		{
			return false;
		}
	
		QList<Event> events;
	
		EventList* eventList = part->events();
		for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
		{
			//Get event type.
			Event pcevt = evt->second;
			if (!pcevt.isNote() && pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
			{
				int xp = pcevt.tick() + part->tick();
				int diff = abs(xp - x);
				if (diff < 300)
				{
					events.append(pcevt);
				}
			}
		}
	
		if (!events.size())
		{
			return false;
		}
	
		Event nearest;
	
		int minDiff = INT_MAX;
		foreach(Event event, events)
		{
			int diff = abs((event.tick() + part->tick()) - x);
			if (diff < minDiff) {
				minDiff = diff;
				nearest = event;
			}
		}
	
		_pc.part = part;
		_pc.event = nearest;
		_pc.valid = true;
		_pc.state = selectedController;/*}}}*/
		emit drawSelectedProgram(nearest.tick(), true);
	}

	update();

	return true;

}/*}}}*/

/**
 * deleteProgramChange
 * @param Event event object to compare against selected
 */
void PCScale::deleteProgramChange(Event evt)
{
	if(_pc.valid && (_pc.event == evt))
	{
		_pc.valid = false;
		_pc.state = doNothing;
		emit drawSelectedProgram(-1, false);
	}
}

/**
 * moveSelected
 * Move the selected program change left or right on the time scale
 * @param dir int value where <=0 == left, >0 == right
 */
void PCScale::moveSelected(int dir)/*{{{*/
{
	if(_pc.valid && _pc.state == selectedController && _pc.part)
	{
		int x = _pc.event.tick();
		if(dir > 0)
		{
			x += currentEditor->rasterStep(x) + _pc.part->tick();
		}
		else
		{
			x -= currentEditor->rasterStep(x) + _pc.part->tick();
		}
		//printf("PCScale::moveSelected(%d) tick; %d\n", dir, x);

		if (x < 0)
			x = 0;
		if((unsigned int)x > _pc.part->tick())
		{
			Event nevent = _pc.event.clone();
			nevent.setTick(x);
			int diff = nevent.tick() - _pc.part->lenTick();
			if (diff > 0)
			{// too short part? extend it
				int endTick = song->roundUpBar(_pc.part->lenTick() + diff);
				_pc.part->setLenTick(endTick);
			}
			audio->msgChangeEvent(_pc.event, nevent, _pc.part, true, true, false);
			_pc.event = nevent;
			emit drawSelectedProgram(_pc.event.tick(), true);
		}

		if(currentEditor->isGlobalEdit() && !selectionList.isEmpty())
		{
			foreach(ProgramChangeObject pco, selectionList)
			{
				int x1 = pco.event.tick();/*{{{*/
				if(dir > 0)
				{
					x1 += currentEditor->rasterStep(x1) + pco.part->tick();
				}
				else
				{
					x1 -= currentEditor->rasterStep(x1) + pco.part->tick();
				}
				//printf("PCScale::moveSelected(%d) tick; %d\n", dir, x);

				if (x1 < 0)
					x1 = 0;
				if((unsigned int)x1 < pco.part->tick())
					continue;
				Event nevent1 = pco.event.clone();
				nevent1.setTick(x1);
				int diff1 = nevent1.tick() - pco.part->lenTick();
				if (diff1 > 0)
				{// too short part? extend it
					int endTick = song->roundUpBar(pco.part->lenTick() + diff1);
					pco.part->setLenTick(endTick);
				}
				audio->msgChangeEvent(pco.event, nevent1, pco.part, true, true, false);
				pco.event = nevent1;/*}}}*/
			}
		}
	}
	update();
}/*}}}*/

/**
 * copySelected
 * Used to copy the selected program change to the current song position
 */
void PCScale::copySelected(bool checked)/*{{{*/
{
	if(!checked || _pc.event.empty())
		return;
	int x = m_clickpos;
	Event nevent = _pc.event.clone();
	nevent.setTick(x);
	if((unsigned int)x > _pc.part->tick())
	{
		int diff = nevent.tick() - _pc.part->lenTick();
		if (diff > 0)
		{// too short part? extend it
			int endTick = song->roundUpBar(_pc.part->lenTick() + diff);
			_pc.part->setLenTick(endTick);
		}
		song->recordEvent((MidiPart*)_pc.part, nevent);
	}
	if(currentEditor->isGlobalEdit() && !selectionList.isEmpty())
	{
		foreach(ProgramChangeObject pco, selectionList)
		{
			Event nevent1 = pco.event.clone();
			nevent1.setTick(x);
			if((unsigned int)x < pco.part->tick())
				continue;
			int diff1 = nevent1.tick() - pco.part->lenTick();
			if (diff1 > 0)
			{// too short part? extend it
				int pendTick = song->roundUpBar(pco.part->lenTick() + diff1);
				pco.part->setLenTick(pendTick);
			}
			song->recordEvent((MidiPart*)pco.part, nevent1);
		}
	}
	_pc.valid = true;
	_pc.state = doNothing;
	selectionList.clear();
	update();
}/*}}}*/

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PCScale::pdraw(QPainter& p, const QRect& r)/*{{{*/
{
	if (waveMode)
	{
		return;
	}

	Part* curPart = currentEditor->curCanvasPart();
	if (!curPart)
	{
		return;
	}

	int x = r.x();
	int w = r.width();

	x -= 20;
	w += 40; // wg. Text

	//---------------------------------------------------
	//    draw Flag
	//---------------------------------------------------

	int y = 12;
	p.setPen(Qt::black);
	p.setFont(config.fonts[4]);
	p.drawLine(r.x(), y + 1, r.x() + r.width(), y + 1);
	QRect tr(r);
	tr.setHeight(12);


	QList<Event> pcEvents;
	EventList* eventList = curPart->events();
	for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
	{
		//Get event type.
		Event pcevt = evt->second;
		if (!pcevt.isNote() && pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
		{
			pcEvents.append(pcevt);
		}
	}

	if (_pc.valid)
	{
		pcEvents.append(_pc.event);
	}

	foreach(Event pcevt, pcEvents)
	{
		int xp;
		// if the event == _pc.event then there is no need to add the part.tick() value
		// since it wasn't substracted yet.
		if (_pc.valid && pcevt == _pc.event)
		{
			xp = mapx(pcevt.tick());
		}
		else
		{
			xp = mapx(pcevt.tick() + curPart->tick());
		}
		if (xp > x + w)
		{
			//printf("Its dying from greater than bar size\n");
			break;
		}
		int xe = r.x() + r.width();
//		iEvent mm = evt;
//		++mm;

		QRect tr(xp, 0, xe - xp, 13);

		QRect wr = r.intersect(tr);
		if (!wr.isEmpty())
		{
			//printf("PCScale::pdraw marker %s xp:%d y:%d h:%d r.x:%d r.w:%d\n", "Test Debug", xp, height(), y, r.x(), r.width());

			// Must be reasonable about very low negative x values! With long songs > 15min
			//  and with high horizontal magnification, 'ghost' drawings appeared,
			//  apparently the result of truncation later (xp = -65006 caused ghosting
			//  at bar 245 with magnification at max.), even with correct clipping region
			//  applied to painter in View::paint(). Tim.  Apr 5 2009
			// Quote: "Warning: Note that QPainter does not attempt to work around
			//  coordinate limitations in the underlying window system. Some platforms may
			//  behave incorrectly with coordinates as small as +/-4000."
			if (xp >= -32)
			{
				if(_pc.valid && _pc.event == pcevt)
				{
					p.drawPixmap(xp, 0, *flagIconSPSel);
				}
				else
				{
					p.drawPixmap(xp, 0, *flagIconSP);
				}
			}
		}
	}
}/*}}}*/

