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
#include "prcanvas.h"
#include "audio.h"

//---------------------------------------------------------
//   PCScale
//    Midi Time Scale
//---------------------------------------------------------

PCScale::PCScale(int* r, QWidget* parent, PianoRoll* editor, int xs, bool _mode)
: View(parent, xs, 1)
{
	//audio = 0;
	currentEditor = editor;
	waveMode = _mode;
	setToolTip(tr("bar pcscale"));
	barLocator = false;
	raster = r;
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
	//connect(song, SIGNAL(markerChanged(int)), SLOT(redraw()));

	setFixedHeight(14);
	setBg(QColor(110, 141, 152));
	pc.state = doNothing;
	pc.moving = false;
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

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void PCScale::viewMousePressEvent(QMouseEvent* event)
{
	button = event->button();
	//viewMouseMoveEvent(event);
	if (event->modifiers() & Qt::ShiftModifier)/*{{{*/
		setCursor(QCursor(Qt::PointingHandCursor));
	else
		setCursor(QCursor(Qt::ArrowCursor));

	int x = event->x();
	x = AL::sigmap.raster(x, *raster);
	if (x < 0)
		x = 0;
	//printf("PCScale::viewMouseMoveEvent\n");
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
		//emit selectInstrument();
		emit addProgramChange();
	}
	else if (i == 2 && (event->modifiers() & Qt::ShiftModifier))/*{{{*/
	{ // If shift +RMB we remove a marker
		//Delete Program change here
		Track* track = song->findTrack(currentEditor->curCanvasPart());
		PartList* parts = track->parts();
		for (iPart pt = parts->begin(); pt != parts->end(); ++pt)
		{
			Part* mprt = pt->second;
			EventList* eventList = mprt->events();
			for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
			{
				//Get event type.
				Event pcevt = evt->second;
				if (!pcevt.isNote())
				{
					if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
					{
						int xp = pcevt.tick() + mprt->tick();
						if (xp >= x && xp <= (x + 50))
						{
							if (audio)
							{
								//song->startUndo();
								//audio->msgDeleteEvent(evt->second, p->second, true, true, true);
								//song->endUndo(SC_EVENT_MODIFIED);
								deleteProgramChange(pcevt);
								song->deleteEvent(pcevt, mprt); //hack
							}
						}
					}
				}
			}
			//redraw();
		}
	}/*}}}*/
	else if(i == 2)
	{//popup a menu here to copy/paste PC
		if(pc.moving && pc.state == selectedController)
		{
			//update the song position before we start so we have a position for paste
			song->setPos(0, p);
			QMenu* menu = new QMenu(this);
			QAction *cp = menu->addAction(tr("Copy Selected Here."));
			cp->setData(1);
			//QAction *mv = menu->addAction(tr("Paste"));
			//mv->setData(2);
			QAction *act = menu->exec(event->globalPos(), 0);
			if(act)
			{
				int id = act->data().toInt();
				if(id == 1)
				{
					Event nevent = pc.event.clone();/*{{{*/
					nevent.setTick(x);
					if((unsigned int)x < pc.part->tick())
						return;
					int diff = nevent.tick() - pc.part->lenTick();
					if (diff > 0)
					{// too short part? extend it
						int endTick = song->roundUpBar(pc.part->lenTick() + diff);
						pc.part->setLenTick(endTick);
					}
					pc.part->addEvent(nevent);/*}}}*/
				}
			}
		}
	}
	else if (i == 0 && (event->modifiers() & Qt::ControlModifier))/*{{{*/
	{ // If LMB select the program change
		Track* track = song->findTrack(currentEditor->curCanvasPart());
		PartList* parts = track->parts();
		bool stop = false;
		for (iPart pt = parts->begin(); pt != parts->end() && !stop; ++pt)
		{
			Part* mprt = pt->second;
			EventList* eventList = mprt->events();
			for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
			{
				//Get event type.
				Event pcevt = evt->second;
				if (!pcevt.isNote())
				{
					if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
					{
						int xp = pcevt.tick() + mprt->tick();
						if (xp >= x && xp <= (x + 50))
						{
							//Select the event and break
							if(pc.moving)
							{
								pc.moving = false;
								pc.state = doNothing;
								emit drawSelectedProgram(-1, false);
							}
							else
							{
								pc.track = track;
								pc.part = mprt;
								pc.event = pcevt;
								pc.moving = true;
								pc.state = selectedController;
								emit drawSelectedProgram(pcevt.tick(), true);
								stop = true;
								break;
							}
						}
					}
				}
			}
		}
		if(!stop)
		{
			//We did not find a program change so just set song position
			song->setPos(i, p);
		}
	}/*}}}*/
	else if (i == 0)/*{{{*/
	{ // If LMB select the program change
		Track* track = song->findTrack(currentEditor->curCanvasPart());
		PartList* parts = track->parts();
		bool stop = false;
		for (iPart pt = parts->begin(); pt != parts->end() && !stop; ++pt)
		{
			Part* mprt = pt->second;
			EventList* eventList = mprt->events();
			for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
			{
				//Get event type.
				Event pcevt = evt->second;
				if (!pcevt.isNote())
				{
					if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
					{
						int xp = pcevt.tick() + mprt->tick();
						if (xp >= x && xp <= (x + 50))
						{
							//Select the event and break
							pc.track = track;
							pc.part = mprt;
							pc.event = pcevt;
							pc.moving = true;
							pc.state = movingController;
							emit drawSelectedProgram(pcevt.tick(), true);
							stop = true;
							break;
						}
					}
				}
			}
		}
		if(!stop)
		{
			//We did not find a program change so just set song position
			song->setPos(i, p);
		}
	}/*}}}*/
	else
		song->setPos(i, p); // all other cases: relocating one of the locators
	
	update();/*}}}*/
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void PCScale::viewMouseReleaseEvent(QMouseEvent* event)
{
	//Deselect the PC only if was a end of move
	if(button == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier))
	{
		if(pc.moving && pc.state == movingController)
		{
			pc.state = doNothing;
			pc.moving = false;
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
	if(pc.moving && pc.state == movingController && pc.track && pc.part)
	{
		int x = event->x();
		x = AL::sigmap.raster(x, *raster);
		if (x < 0)
			x = 0;
		//int tick = pc.event.tick() + pc.part->tick();
		if((unsigned int)x < pc.part->tick())
			return;
		Event nevent = pc.event.clone();
		nevent.setTick(x);
		int diff = nevent.tick() - pc.part->lenTick();
		if (diff > 0)
		{// too short part? extend it
			int endTick = song->roundUpBar(pc.part->lenTick() + diff);
			pc.part->setLenTick(endTick);
		}
		audio->msgChangeEvent(pc.event, nevent, pc.part, true, false, false);
		pc.event = nevent;
		emit drawSelectedProgram(pc.event.tick(), true);
	}
}/*}}}*/

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void PCScale::leaveEvent(QEvent*)
{
	//emit timeChanged(MAXINT);
}

void PCScale::setEditor(PianoRoll* editor)
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
void PCScale::selectProgramChange()/*{{{*/
{
	Track* track = song->findTrack(currentEditor->curCanvasPart());
	PartList* parts = track->parts();
	bool stop = false;
	int x = song->cpos();
	for (iPart pt = parts->begin(); pt != parts->end() && !stop; ++pt)
	{
		Part* mprt = pt->second;
		EventList* eventList = mprt->events();
		for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
		{
			//Get event type.
			Event pcevt = evt->second;
			if (!pcevt.isNote())
			{
				if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
				{
					int xp = pcevt.tick() + mprt->tick();
					if (xp >= x && xp <= (x + 50))
					{
						//Select the event and break
						if(pc.moving)
						{
							pc.moving = false;
							pc.state = doNothing;
							emit drawSelectedProgram(-1, false);
						}
						else
						{
							pc.track = track;
							pc.part = mprt;
							pc.event = pcevt;
							pc.moving = true;
							pc.state = selectedController;
							emit drawSelectedProgram(pcevt.tick(), true);
							stop = true;
							break;
						}
					}
				}
			}
		}
	}
	update();
}/*}}}*/

/**
 * deleteProgramChange
 * @param Event event object to compare against selected
 */
void PCScale::deleteProgramChange(Event evt)
{
	if(pc.moving && (pc.event == evt))
	{
		pc.moving = false;
		pc.state = doNothing;
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
	if(pc.moving && pc.state == selectedController && pc.track && pc.part)
	{
		int x = pc.event.tick();
		if(dir > 0)
		{
        	x += currentEditor->rasterStep(x) + pc.part->tick();
		}
		else
		{
        	x -= currentEditor->rasterStep(x) + pc.part->tick();
		}
		//printf("PCScale::moveSelected(%d) tick; %d\n", dir, x);

		if (x < 0)
			x = 0;
		if((unsigned int)x < pc.part->tick())
			return;
		Event nevent = pc.event.clone();
		nevent.setTick(x);
		int diff = nevent.tick() - pc.part->lenTick();
		if (diff > 0)
		{// too short part? extend it
			int endTick = song->roundUpBar(pc.part->lenTick() + diff);
			pc.part->setLenTick(endTick);
		}
		audio->msgChangeEvent(pc.event, nevent, pc.part, true, false, false);
		pc.event = nevent;
		emit drawSelectedProgram(pc.event.tick(), true);
	}
	update();
}/*}}}*/

/**
 * copySelected
 * Used to copy the selected program change to the current song position
 */
void PCScale::copySelected()/*{{{*/
{
	if(pc.moving && pc.state == selectedController)
	{
		Event nevent = pc.event.clone();
		nevent.setTick(song->cpos());
		if(song->cpos() < pc.part->tick())
			return;
		int diff = nevent.tick() - pc.part->lenTick();
		if (diff > 0)
		{// too short part? extend it
			int endTick = song->roundUpBar(pc.part->lenTick() + diff);
			pc.part->setLenTick(endTick);
		}
		pc.part->addEvent(nevent);
		update();
	}
}/*}}}*/

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void PCScale::pdraw(QPainter& p, const QRect& r)/*{{{*/
{
	if (waveMode)
		return;
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
	Track* track = song->findTrack(currentEditor->curCanvasPart());
	PartList* parts = track->parts();
	for (iPart m = parts->begin(); m != parts->end(); ++m)
	{
		Part* mprt = m->second;
		EventList* eventList = mprt->events();
		for (iEvent evt = eventList->begin(); evt != eventList->end(); ++evt)
		{
			//Get event type.
			Event pcevt = evt->second;
			if (!pcevt.isNote())
			{
				if (pcevt.type() == Controller && pcevt.dataA() == CTRL_PROGRAM)
				{
					int xp = mapx(pcevt.tick() + mprt->tick());
					if (xp > x + w)
					{
						//printf("Its dying from greater than bar size\n");
						break;
					}
					int xe = r.x() + r.width();
					iEvent mm = evt;
					++mm;

					QRect tr(xp, 0, xe - xp, 13);

					QRect wr = r.intersect(tr);
					if (!wr.isEmpty())
					{
						int x2;
						if (mm != eventList->end())
						{
							x2 = mapx(pcevt.tick() + mprt->tick());
						}
						else
							x2 = xp + 200;

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
							if(pc.moving && pc.event == pcevt)
							{
								p.drawPixmap(xp, 0, *flagIconSPSel);
							}
							else
							{
								p.drawPixmap(xp, 0, *flagIconSP);
							}
						}

						//	if(xp >= -1023)
						//	{
						//		QRect r = QRect(xp+10, 0, x2-xp, 12);
						//		p.setPen(Qt::black);
						//		//Use the program change info as name
						//		p.drawText(r, Qt::AlignLeft|Qt::AlignVCenter, "Test"/*pcevt.name()*/);
						//	}

						//Andrew Commenting this line to test the new flag
						//if(xp >= 0)
						//{
						//	p.setPen(Qt::red);
						//	p.drawLine(xp, y, xp, height());
						//}  
					}//END if(wr.isEmpty)
				}//END if(CTRL_PROGRAM)
			}//END if(!isNote)
		}
	}
}/*}}}*/

