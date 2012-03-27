//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: ctrlcanvas.cpp,v 1.15.2.10 2009/11/14 03:37:48 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <values.h>

#include <QPainter>
#include <QCursor>
#include <QMouseEvent>
#include <QtGui>

#include "globals.h"
#include "ctrledit.h"
#include "AbstractMidiEditor.h"
#include "icons.h"
#include "midiport.h"
#include "song.h"
#include "midictrl.h"
#include "audio.h"
#include "gconfig.h"
#include "ctrlpanel.h"
#include "midiedit/drummap.h"
#include "traverso_shared/TConfig.h"
#include "midimonitor.h"

extern void drawTickRaster(QPainter& p, int x, int y, int w, int h, int quant, bool ctrl);

static MidiCtrlValList veloList(CTRL_VELOCITY); // dummy

//---------------------------------------------------------
//   computeVal
//---------------------------------------------------------

static int computeVal(MidiController* mc, int y, int height)
{
	int min;
	int max;
	if (mc->num() == CTRL_PROGRAM)
	{
		min = 1;
		max = 128;
	}
	else
	{
		min = mc->minVal();
		max = mc->maxVal();
	}
	int val = max - (y * (max - min) / height);
	if (val < min)
		val = min;
	if (val > max)
		val = max;
	if (mc->num() != CTRL_PROGRAM)
		val += mc->bias();
	return val;
}

//---------------------------------------------------------
//   CEvent
//---------------------------------------------------------

CEvent::CEvent(Event e, MidiPart* pt, int v, bool sel)
{
	_event = e;
	_part = pt;
	_val = v;
	ex = !e.empty() ? e.tick() : 0;
	_selected = sel;
}

//---------------------------------------------------------
//   contains
//---------------------------------------------------------

bool CEvent::contains(int x1, int x2) const
{
	int tick1 = !_event.empty() ? _event.tick() + _part->tick() : 0;
	if (ex == -1)
		return (tick1 < x2);

	int tick2 = ex + _part->tick();
	return ((tick1 >= x1 && tick1 < x2)
			//|| (tick2 >= x1 && tick2 < x2)
			|| (tick2 > x1 && tick2 < x2)
			|| (tick1 < x1 && tick2 >= x2));
}

//---------------------------------------------------------
//   clearDelete
//---------------------------------------------------------

void CEventList::clearDelete()
{
	for (ciCEvent i = begin(); i != end(); ++i)
	{
		CEvent* ce = *i;
		if (ce)
			delete ce;
	}
	clear();
}

//---------------------------------------------------------
//   CtrlCanvas
//---------------------------------------------------------

CtrlCanvas::CtrlCanvas(AbstractMidiEditor* e, QWidget* parent, int xmag,
		const char* name, CtrlPanel* pnl) : View(parent, xmag, 1, name)
{
	setBg(QColor(63,63,63));

	editor = e;
	drag = DRAG_OFF;
	tool = PointerTool;
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = 0;
	noEvents = false;
	m_collapsed = false;
	m_feedbackMode = midiMonitor->feedbackMode();

	ctrl = &veloList;
	_controller = &veloCtrl;
	_panel = pnl;
	_cnum = CTRL_VELOCITY;
	_dnum = CTRL_VELOCITY;
	_didx = CTRL_VELOCITY;
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));

	setMouseTracking(true);
	if (editor->parts()->empty())
	{
		curPart = 0;
		curTrack = 0;
	}
	else
	{
		setCurTrackAndPart();
	}
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

	curDrumInstrument = editor->curDrumInstrument();
	//printf("CtrlCanvas::CtrlCanvas curDrumInstrument:%d\n", curDrumInstrument);

	connect(editor, SIGNAL(curDrumInstrumentChanged(int)), SLOT(setCurDrumInstrument(int)));
    connect(heartBeatTimer, SIGNAL(timeout()), SLOT(heartBeat()));
    updateItems();
}

void CtrlCanvas::heartBeat()
{
	int mode = midiMonitor->feedbackMode();
	if(mode != m_feedbackMode)
	{
		m_feedbackMode = mode;
		update();
	}
}

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void CtrlCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)/*{{{*/
{
	if (pos[idx] == val)
		return;

	int opos = mapx(pos[idx]);
	int npos = mapx(val);

	if (adjustScrollbar && idx == 0)
	{
		switch (song->follow())
		{
			case Song::NO:
				break;
			case Song::JUMP:
				if (npos >= width())
				{
					int ppos = val - rmapxDev(width() / 4);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < 0)
				{
					int ppos = val - rmapxDev(width()*3 / 4);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				break;
			case Song::CONTINUOUS:
				if (npos > (width()*5) / 8)
				{
					int ppos = pos[idx] - rmapxDev(width()*5 / 8);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < (width()*3) / 8)
				{
					int ppos = pos[idx] - rmapxDev(width()*3 / 8);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				break;
		}
	}

	int x;
	int w = 1;
	if (opos > npos)
	{
		w += opos - npos;
		x = npos;
	}
	else
	{
		w += npos - opos;
		x = opos;
	}
    pos[idx] = val;

        // FIXME: redrawing this rectangle wipes out the controller lane
        // items when cursor passes over them.
        // Redrawing the whole view fixes that for now, but it could be a cpu hog ?
//	redraw(QRect(x, 0, w, height()));
        update();
}/*}}}*/

void CtrlCanvas::toggleCollapsed(bool val)
{
	m_collapsed = val;
	update();
}

//---------------------------------------------------------
//   setMidiController
//---------------------------------------------------------

void CtrlCanvas::setMidiController(int num)
{
	_cnum = num;
	partControllers(curPart, _cnum, &_dnum, &_didx, &_controller, &ctrl);
	if (_panel)
	{
		if (_cnum == CTRL_VELOCITY)
			_panel->setHWController(curTrack, &veloCtrl);
		else
			_panel->setHWController(curTrack, _controller);
	}
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void CtrlCanvas::leaveEvent(QEvent*)
{
	emit xposChanged(MAXINT);
	emit yposChanged(-1);
}

//---------------------------------------------------------
//   raster
//---------------------------------------------------------

QPoint CtrlCanvas::raster(const QPoint& p) const
{
	return p;
}

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void CtrlCanvas::deselectAll()
{
	//    for (iCEvent i = selection.begin(); i != selection.end(); ++i)
	//            (*i)->setState(CEvent::Normal);
	//      selection.clear();
	//      update();
}

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void CtrlCanvas::selectItem(CEvent*)
{
	//      e->setState(CEvent::Selected);
	//      selection.push_back(e);
	//      update();
}

//---------------------------------------------------------
//   deselectItem
//---------------------------------------------------------

void CtrlCanvas::deselectItem(CEvent*)
{
	/*      e->setState(CEvent::Normal);
		  for (iCEvent i = selection.begin(); i != selection.end(); ++i) {
				if (*i == e) {
					  selection.erase(i);
					  break;
					  }
				}
		  update();
	 */
}

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void CtrlCanvas::setController(int num)
{
	setMidiController(num);
	updateItems();
}

//---------------------------------------------------------
//   setCurTrackAndPart
//---------------------------------------------------------

bool CtrlCanvas::setCurTrackAndPart()
{
	bool changed = false;
	MidiPart* part = 0;
	MidiTrack* track = 0;

	if (!editor->parts()->empty())
	{
		Part* pt = editor->curCanvasPart();
		if (pt && pt->track())
		{
			if (pt->track()->isMidiTrack())
			{
				part = (MidiPart*) pt;
				track = part->track();
			}
		}
	}

	if (part != curPart)
	{
		curPart = part;
		changed = true;
	}

	if (track != curTrack)
	{
		curTrack = track;
		changed = true;
	}

	return changed;
}

//---------------------------------------------------------
//   songChanged
//    all marked parts are added to the internal event list
//---------------------------------------------------------

void CtrlCanvas::songChanged(int type)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (type == SC_MIDI_CONTROLLER)
		return;

	bool changed = false;
	if (type & (SC_CONFIG | SC_PART_MODIFIED | SC_SELECTION))
		changed = setCurTrackAndPart();

	// Although changing the instrument/device in the
	//  config window generates a type of -1, we can eliminate
	//  some other useless calls using SC_CONFIG, which was not used
	//  anywhere else in oom before now, except song header.
	if ((type & (SC_CONFIG | SC_DRUMMAP)) || ((type & (SC_PART_MODIFIED | SC_SELECTION)) && changed))
	{
		setMidiController(_cnum);
		//return;
	}

    updateItems();

}

//---------------------------------------------------------
//   partControllers
//---------------------------------------------------------

void CtrlCanvas::partControllers(const MidiPart* part, int num, int* dnum, int* didx, MidiController** mc, MidiCtrlValList** mcvl)/*{{{*/
{
	if (num == CTRL_VELOCITY) // special case
	{
		if (mcvl)
			*mcvl = &veloList;
		if (mc)
			*mc = &veloCtrl;
		if (dnum)
			*dnum = num;
		if (didx)
			*didx = num;
	}
	else
	{
		MidiTrack* mt = part->track();
		MidiPort* mp;
		int di;
		int n;

		if ((mt->type() != Track::DRUM) && curDrumInstrument != -1)
			printf("keyfilter != -1 in non drum track?\n");

		if ((mt->type() == Track::DRUM) && (curDrumInstrument != -1) && ((num & 0xff) == 0xff))
		{
			di = (num & ~0xff) | curDrumInstrument;
			n = (num & ~0xff) | drumMap[curDrumInstrument].anote; // construct real controller number
			//num = (num & ~0xff) | curDrumInstrument);  // construct real controller number
			mp = &midiPorts[drumMap[curDrumInstrument].port];
		}
		else
		{
			di = num;
			n = num;
			mp = &midiPorts[mt->outPort()];
		}

		if (dnum)
			*dnum = n;

		if (didx)
			*didx = di;

		if (mc)
			*mc = mp->midiController(n);

		if (mcvl)
		{
			MidiCtrlValList* tmcvl = 0;
			MidiCtrlValListList* cvll = mp->controller();
			for (iMidiCtrlValList i = cvll->begin(); i != cvll->end(); ++i)
			{
				if (i->second->num() == n)
				{
					tmcvl = i->second;
					break;
				}
			}
			*mcvl = tmcvl;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   updateItems
//---------------------------------------------------------

void CtrlCanvas::updateItems()/*{{{*/
{
	items.clearDelete();

    if (!editor->parts()->empty())
	{
		MidiPart* cPart = static_cast<MidiPart*>(editor->curCanvasPart());
        QList<Event> selEvents = editor->getSelectedEvents();
		for (iPart p = editor->parts()->begin(); p != editor->parts()->end(); ++p)
        {
			Event last;
			CEvent* lastce = 0;

			MidiPart* part = (MidiPart*) (p->second);
			EventList* el = part->events();
			MidiController* mc;
			MidiCtrlValList* mcvl;
			partControllers(part, _cnum, 0, 0, &mc, &mcvl);
			unsigned len = part->lenTick();
			bool isPart = (cPart && cPart == part);
			if(multiPartSelectionAction && multiPartSelectionAction->isChecked())
                isPart = true;


			for (iEvent i = el->begin(); i != el->end(); ++i)
            {
				Event e = i->second;
				// Added by T356. Do not add events which are past the end of the part.
				if (e.tick() >= len)
					break;
                bool sel = (isPart && (!selEvents.isEmpty() && selEvents.contains(e)));//false;//(isPart && editor->isEventSelected(e));

				if (_cnum == CTRL_VELOCITY && e.type() == Note)
				{
					//printf("CtrlCanvas::updateItems CTRL_VELOCITY curDrumInstrument:%d\n", curDrumInstrument);
					if (curDrumInstrument == -1)
					{
						// This is interesting - it would allow ALL drum note velocities to be shown.
						// But currently the drum list ALWAYS has a selected item so this is not supposed to happen.
						items.add(new CEvent(e, part, e.velo(), sel));
					}
					else if (e.dataA() == curDrumInstrument) //same note
						items.add(new CEvent(e, part, e.velo(), sel));
				}
				else if (e.type() == Controller && e.dataA() == _didx)
				{
					if (mcvl && last.empty())
					{
						lastce = new CEvent(Event(), part, mcvl->value(part->tick()));
						items.add(lastce);
					}
					if (lastce)
						lastce->setEX(e.tick());
					lastce = new CEvent(e, part, e.dataB());
					lastce->setEX(-1);
					items.add(lastce);
					last = e;
				}
			}
		}
	}


    redraw();
}/*}}}*/

//---------------------------------------------------------
//   newValRamp
//---------------------------------------------------------

void CtrlCanvas::newValRamp(int x1, int y1, int x2, int y2)/*{{{*/
{
	int xx1 = editor->rasterVal1(x1);
	int xx2 = editor->rasterVal2(x2);
	int type = _controller->num();

	int raster = editor->raster();
	if (raster == 1) // set reasonable raster
		raster = config.division / 4;

	song->startUndo();

	// delete existing events

	int lastpv = CTRL_VAL_UNKNOWN;
	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		Event event = ev->event();
		if (event.empty())
			continue;
		int x = event.tick() + curPart->tick();

		if (x < xx1)
		{
			continue;
		}
		if (x >= xx2)
			break;

		// Indicate no undo, and do port controller values and clone parts.
        audio->msgDeleteEvent(event, curPart, false, true, true, false);
	}

	if (ctrl)
		lastpv = ctrl->hwVal();

	// insert new events
	for (int x = xx1; x < xx2; x += raster)
	{
		int y = (x2 == x1) ? y1 : (((y2 - y1)*(x - x1)) / (x2 - x1)) + y1;
		int nval = computeVal(_controller, y, height());
		int tick = x - curPart->tick();
		// Do not add events which are past the end of the part.
		if ((unsigned) tick >= curPart->lenTick())
			break;
		Event event(Controller);
		event.setTick(tick);
		event.setA(_didx);
		if (type == CTRL_PROGRAM)
		{
			if (lastpv == CTRL_VAL_UNKNOWN)
			{
				if (song->mtype() == MT_GM)
					event.setB(0xffff00 | (nval - 1));
				else
					event.setB(nval - 1);
			}
			else
				event.setB((lastpv & 0xffff00) | (nval - 1));
		}
		else
			event.setB(nval);

		// Indicate no undo, and do port controller values and clone parts.
        audio->msgAddEvent(event, curPart, false, true, true, false);
	}

	song->update(0);
	redraw();
	song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED | SC_EVENT_REMOVED);
}/*}}}*/

//---------------------------------------------------------
//   changeValRamp
//---------------------------------------------------------

void CtrlCanvas::changeValRamp(int x1, int y1, int x2, int y2)/*{{{*/
{
	int h = height();
	bool changed = false;
	int type = _controller->num();

	song->startUndo();
	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		if ((*i)->contains(x1, x2))
		{
			CEvent* ev = *i;
			if (ev->part() != curPart)
				continue;
			Event event = ev->event();
			if (event.empty())
				continue;

			int x = event.tick() + curPart->tick();
			int y = (x2 == x1) ? y1 : (((y2 - y1)*(x - x1)) / (x2 - x1)) + y1;
			int nval = computeVal(_controller, y, h);
			if (type == CTRL_PROGRAM)
			{
				if (event.dataB() == CTRL_VAL_UNKNOWN)
				{
					--nval;
					if (song->mtype() == MT_GM)
						nval |= 0xffff00;
				}
				else
					nval = (event.dataB() & 0xffff00) | (nval - 1);
			}

			ev->setVal(nval);

			if (type == CTRL_VELOCITY)
			{
				if ((event.velo() != nval))
				{
					Event newEvent = event.clone();
					newEvent.setVelo(nval);
					// Indicate no undo, and do not do port controller values and clone parts.
                    audio->msgChangeEvent(event, newEvent, curPart, false, false, false, false);
					ev->setEvent(newEvent);
					changed = true;
				}
			}
			else
			{
				if (!event.empty())
				{
					if ((event.dataB() != nval))
					{
						Event newEvent = event.clone();
						newEvent.setB(nval);
						// Indicate no undo, and do port controller values and clone parts.
                        audio->msgChangeEvent(event, newEvent, curPart, false, true, true, false);
						ev->setEvent(newEvent);
						changed = true;
					}
				}
			}
		}
	}
	if (changed)
		redraw();
	song->endUndo(SC_EVENT_MODIFIED);
}/*}}}*/

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void CtrlCanvas::viewMousePressEvent(QMouseEvent* event)/*{{{*/
{
	if(m_collapsed)
		return; //No mouse action in collapsed mode
	start = event->pos();
	Tool activeTool = tool;
	//bool shift = event->modifiers() & Qt::ShiftModifier;
	if (event->buttons() & Qt::RightButton)
	{
		QMenu *p = new QMenu(this);
    	p->addAction(tr("READ"))->setData(FEEDBACK_MODE_READ);
    	p->addAction(tr("WRITE"))->setData(FEEDBACK_MODE_WRITE);
    	p->addAction(tr("TOUCH"))->setData(FEEDBACK_MODE_TOUCH);
    	p->addAction(tr("AUDITION"))->setData(FEEDBACK_MODE_AUDITION);

		QAction* act = p->exec(QCursor::pos());
		if(act)
		{
			int val = act->data().toInt();
    		midiMonitor->setFeedbackMode(static_cast<FeedbackMode>(val));
			m_feedbackMode = val;
			update();
		}
		return;
	}

	int xpos = start.x();
	int ypos = start.y();

	MidiController::ControllerType type = midiControllerType(_controller->num());

	switch (activeTool)
	{
		case PointerTool:
			drag = DRAG_LASSO_START;
			break;

		case PencilTool:
			{
				if (type != MidiController::Velo)
				{
					drag = DRAG_NEW;
					song->startUndo();
					newVal(xpos, xpos, ypos);
				}
				else
				{
					drag = DRAG_RESIZE;
					song->startUndo();
					changeVal(xpos, xpos, ypos);
				}
			}
			break;

		case RubberTool:
			if (type != MidiController::Velo)
			{
				drag = DRAG_DELETE;
				song->startUndo();
				deleteVal(xpos, xpos, ypos);
			}
			break;

		case DrawTool:
			if (drawLineMode)
			{
				line2x = xpos;
				line2y = ypos;
				if (type != MidiController::Velo)
					newValRamp(line1x, line1y, line2x, line2y);
				else
					changeValRamp(line1x, line1y, line2x, line2y);
				drawLineMode = false;
			}
			else
			{
				line2x = line1x = xpos;
				line2y = line1y = ypos;
				drawLineMode = true;
			}
			redraw();
			break;

		default:
			break;
	}
}/*}}}*/

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseMoveEvent(QMouseEvent* event)/*{{{*/
{
	if(m_collapsed)
		return; //No mouse action in collapsed mode
	QPoint pos = event->pos();
	QPoint dist = pos - start;
	bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;
	switch (drag)
	{
		case DRAG_LASSO_START:
			if (!moving)
				break;
			drag = DRAG_LASSO;
			// weiter mit DRAG_LASSO:
		case DRAG_LASSO:
			lasso.setRect(start.x(), start.y(), dist.x(), dist.y());
			redraw();
			break;
        case DRAG_RESIZE:
			changeVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

        case DRAG_NEW:
			newVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

        case DRAG_DELETE:
			deleteVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

		default:
			break;
	}
	if (tool == DrawTool && drawLineMode)
	{
		line2x = pos.x();
		line2y = pos.y();
		redraw();
	}
	emit xposChanged(pos.x());


	int val = computeVal(_controller, pos.y(), height());
	emit yposChanged(val);
}/*}}}*/

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void CtrlCanvas::viewMouseReleaseEvent(QMouseEvent* event)/*{{{*/
{
	if(m_collapsed)
		return; //No mouse action in collapsed mode
	bool shift = event->modifiers() & Qt::ShiftModifier;

	switch (drag)
	{
		case DRAG_RESIZE:
		case DRAG_NEW:
		case DRAG_DELETE:
			song->endUndo(SC_EVENT_MODIFIED | SC_EVENT_INSERTED);
			break;

		case DRAG_LASSO_START:
			lasso.setRect(-1, -1, -1, -1);

		case DRAG_LASSO:
			if (!shift)
				deselectAll();
			lasso = lasso.normalized();
			for (iCEvent i = items.begin(); i != items.end(); ++i)
			{
#if 0
				if ((*i)->intersects(lasso))
				{
					if (shift && (*i)->isSelected())
						deselectItem(*i);
					else
						selectItem(*i);
				}
#endif
			}
			drag = DRAG_OFF;
			redraw();
			break;

		default:
			break;
	}
	drag = DRAG_OFF;
}/*}}}*/

//---------------------------------------------------------
//   changeVal
//---------------------------------------------------------

void CtrlCanvas::changeVal(int x1, int x2, int y)/*{{{*/
{
	bool changed = false;
	int newval = computeVal(_controller, y, height());
	int type = _controller->num();

	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		if (!(*i)->contains(x1, x2))
			continue;
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		Event event = ev->event();

		if (type == CTRL_VELOCITY)
		{
			if ((event.velo() != newval))
			{
				ev->setVal(newval);
				Event newEvent = event.clone();
				newEvent.setVelo(newval);
				// Indicate no undo, and do not do port controller values and clone parts.
                audio->msgChangeEvent(event, newEvent, curPart, false, false, false, false);
				ev->setEvent(newEvent);
				changed = true;
			}
		}
		else
		{
			if (!event.empty())
			{
				int nval = newval;
				if (type == CTRL_PROGRAM)
				{
					if (event.dataB() == CTRL_VAL_UNKNOWN)
					{
						--nval;
						if (song->mtype() == MT_GM)
							nval |= 0xffff00;
					}
					else
						nval = (event.dataB() & 0xffff00) | (nval - 1);
				}
				ev->setVal(nval);

				if ((event.dataB() != nval))
				{
					Event newEvent = event.clone();
					newEvent.setB(nval);
					// Indicate no undo, and do port controller values and clone parts.
                    audio->msgChangeEvent(event, newEvent, curPart, false, true, true, false);
					ev->setEvent(newEvent);
					changed = true;
				}
			}
			else
			{
			}
		}
	}
	if (changed)
		redraw();
}/*}}}*/

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void CtrlCanvas::newVal(int x1, int x2, int y)/*{{{*/
{
	int xx1 = editor->rasterVal1(x1);
	int xx2 = editor->rasterVal2(x2);
	int newval = computeVal(_controller, y, height());
    int type = _controller->num();

	bool found = false;
	bool song_changed = false;

	int lastpv = CTRL_VAL_UNKNOWN;
	if (ctrl)
        lastpv = ctrl->hwVal();

	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		
		int partTick = curPart->tick();
		Event event = ev->event();
		if (event.empty())
			continue;
        int ax = event.tick() + partTick;

        if (ax < xx1)
            continue;
        if (ax >= xx2)
            break;

        int nval = newval;
        if (type == CTRL_PROGRAM)
        {
            if (event.dataB() == CTRL_VAL_UNKNOWN)
            {
                if (lastpv == CTRL_VAL_UNKNOWN)
                {
                    --nval;
                    if (song->mtype() == MT_GM)
                        nval |= 0xffff00;
                }
                else
                    nval = (lastpv & 0xffff00) | (nval - 1);
            }
            else
                nval = (event.dataB() & 0xffff00) | (nval - 1);
        }

		if (ax == xx1)
		{
			// change event
			found = true;
			ev->setVal(nval);
			if ((event.dataB() != nval))
			{
                Event newEvent = event.clone();
                newEvent.setB(nval);

				// Indicate no undo, and do port controller values and clone parts.
                audio->msgChangeEvent(event, newEvent, curPart, false, true, true, false);

                ev->setEvent(newEvent);
				song_changed = true;
			}
		}
        else if (ax < xx2)
		{
			// Indicate no undo, and do port controller values and clone parts.
            audio->msgDeleteEvent(event, curPart, false, true, true, false);

			song_changed = true;
        }
	}
	if (!found)
	{
		// new event
		int tick = xx1 - curPart->tick();
		// Do not add events which are past the end of the part.
		if ((unsigned) tick < curPart->lenTick())
		{
			Event event(Controller);
			event.setTick(tick);
			event.setA(_didx);
			if (type == CTRL_PROGRAM)
			{
				if (lastpv == CTRL_VAL_UNKNOWN)
				{
					if (song->mtype() == MT_GM)
						event.setB(0xffff00 | (newval - 1));
					else
						event.setB(newval - 1);
				}
				else
					event.setB((lastpv & 0xffff00) | (newval - 1));
			}
			else
				event.setB(newval);

			// Indicate no undo, and do port controller values and clone parts.
            audio->msgAddEvent(event, curPart, false, true, true, false);

			song_changed = true;
		}
	}
	if (song_changed)
	{
		songChanged(0);
		return;
	}
	redraw();
}/*}}}*/

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

void CtrlCanvas::deleteVal(int x1, int x2, int)/*{{{*/
{
	int xx1 = editor->rasterVal1(x1);
	int xx2 = editor->rasterVal2(x2);

	int partTick = curPart->tick();
	xx1 -= partTick;
	xx2 -= partTick;

	bool song_changed = false;
	for (ciCEvent i = items.begin(); i != items.end(); ++i)
	{
		CEvent* ev = *i;
		if (ev->part() != curPart)
			continue;
		Event event = ev->event();
		if (event.empty())
			continue;
		int x = event.tick();
		if (x < xx1)
			continue;
		if (x >= xx2)
			break;
		if (!event.empty())
		{
			// Indicate no undo, and do port controller values and clone parts.
			//audio->msgDeleteEvent(event, ev->part(), false);
            audio->msgDeleteEvent(event, curPart, false, true, true, false);
			song_changed = true;
		}
	}
	if (song_changed)
	{
		songChanged(0);
		return;
	}
}/*}}}*/

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void CtrlCanvas::setTool(int t)/*{{{*/
{
	if (tool == Tool(t))
		return;
	tool = Tool(t);
	switch (tool)
	{
		case PencilTool:
			setCursor(QCursor(*pencilCursorIcon, 6, 15));
			break;
		case DrawTool:
			drawLineMode = false;
			break;
		default:
			setCursor(QCursor(Qt::ArrowCursor));
			break;
	}
}/*}}}*/

//---------------------------------------------------------
//   pdrawItems
//---------------------------------------------------------

void CtrlCanvas::pdrawItems(QPainter& p, const QRect& rect, const MidiPart* part, bool velo, bool fg)/*{{{*/
{
        // Remon: Do we really have to do that ???? Seems unnesecary to me.
    int x = 0;//rect.x() - 1; // compensate for 3 pixel line width
    int w = rect.width() + 2;
	int wh = height();

	if (velo)/*{{{*/
	{
		for (iCEvent i = items.begin(); i != items.end(); ++i)
		{
			CEvent* e = *i;
			// Draw selected part velocity events on top of unselected part events.
			if (e->part() != part || e->event().empty())
				continue;
			int tick = mapx(e->event().tick() + e->part()->tick());
			if (tick <= x)
				continue;
			if (tick > x + w)
				break;
			int y1 = wh - (e->val() * wh / 128);
			QColor fillColor = QColor(config.partColors[part->colorIndex()]);
			QColor bgfillColor = QColor(config.partColors[part->colorIndex()]);
			int bgalpha = 180;
			int ghostedAlpha = tconfig().get_property("PerformerEdit", "renderalpha", 50).toInt();
			if(noteAlphaAction && !noteAlphaAction->isChecked())
				ghostedAlpha = 0;
			if(fg)
			{
				QColor bgBarColor = QColor(config.partColors[part->colorIndex()]);
				bgBarColor.setAlpha(bgalpha);
				QPen borderPen = QPen(bgBarColor, 1);
				borderPen.setCosmetic(true);
				p.setPen(borderPen);
				
				QColor bgBarColorOutline = QColor(config.partWaveColors[part->colorIndex()]);
				bgBarColorOutline.setAlpha(bgalpha);
				p.setBrush(bgBarColorOutline);
			}
			else
			{
				QColor bgBarColor = QColor(config.partWaveColors[part->colorIndex()]);
				bgBarColor.setAlpha(ghostedAlpha);
				QPen borderPen = QPen(bgBarColor, 1);
				borderPen.setCosmetic(true);
				p.setPen(Qt::NoPen);
				
				QColor bgBarColorOutline = QColor(config.partColors[part->colorIndex()]);
				bgBarColorOutline.setAlpha(ghostedAlpha);
				p.setBrush(bgBarColorOutline);
			}
			//int alpha = 255;
			QColor outlineColor = QColor(config.partWaveColors[part->colorIndex()]);
			QColor bgoutlineColor = QColor(config.partWaveColors[part->colorIndex()]);
			bgoutlineColor.setAlpha(127);
			
			if(e->selected())
			{
				QPen mypen6 = QPen(outlineColor, 1, Qt::SolidLine);
				QPen mypen7 = QPen(bgoutlineColor, 1, Qt::SolidLine);
				p.setPen(mypen7);
				p.setBrush(QBrush(bgfillColor));
				p.drawRect(tick+1, y1-3, 5, wh);
				p.setPen(mypen6);
				p.setBrush(QBrush(fillColor));
				QRect rect(tick+1, y1-3, 5, 5);
				p.drawRect(rect);
			}
			else
			{
				p.drawRect(tick+1, y1-3, 5, wh);
			}
		}
	}/*}}}*/
	else
	{
		MidiTrack* mt = part->track();
		MidiPort* mp;

		if ((mt->type() == Track::DRUM) && (curDrumInstrument != -1) && ((_cnum & 0xff) == 0xff))
			mp = &midiPorts[drumMap[curDrumInstrument].port];
		else
			mp = &midiPorts[mt->outPort()];

		MidiController* mc = mp->midiController(_cnum);

		int min;
		int max;
		int bias;
		if (_cnum == CTRL_PROGRAM)
		{
			min = 1;
			max = 128;
			bias = 0;
		}
		else
		{
			min = mc->minVal();
			max = mc->maxVal();
			bias = mc->bias();
		}
		int x1 = rect.x();
		int lval = CTRL_VAL_UNKNOWN;
		noEvents = false;
		QColor color = QColor();
		QColor green = QColor(config.partWaveColors[part->colorIndex()]);
		green.setAlpha(140);
		QColor yellow = QColor(41, 130, 140);
		QColor red = QColor(config.partColors[part->colorIndex()]);
		red.setAlpha(140);
		QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, height()));
		vuGrad.setColorAt(1, green);
		vuGrad.setColorAt(0, red);
		QPen myPen = QPen();
		myPen.setBrush(QBrush(vuGrad));
		for (iCEvent i = items.begin(); i != items.end(); ++i)
		{
			CEvent* e = *i;
			// Draw unselected part controller events (lines) on top of selected part events (bars).
			if (e->part() != part)
			{
				continue;
			}
			Event ev = e->event();
			int tick = mapx(!ev.empty() ? ev.tick() + e->part()->tick() : 0);
			int val = e->val();
			int pval = val;
			if (_cnum == CTRL_PROGRAM)
			{
				if ((val & 0xff) == 0xff)
					// What to do here? prog = 0xff should not be allowed, but may still be encountered.
					pval = 1;
				else
					pval = (val & 0x7f) + 1;
			}
			if (tick <= x)
			{
				if (val == CTRL_VAL_UNKNOWN)
					lval = CTRL_VAL_UNKNOWN;
				else
				{
					if (_cnum == CTRL_PROGRAM)
						lval = wh - ((pval - min - bias) * wh / (max - min));
					else
						lval = wh - ((val - min - bias) * wh / (max - min));
				}
				continue;
			}
			if (tick > x + w)
				break;

			if (lval != CTRL_VAL_UNKNOWN)
			{
				QColor tickColor = QColor(config.partColors[part->colorIndex()]);
				if (!fg)
				{
					p.setPen(myPen);
					p.fillRect(x1, lval, tick - x1, wh - lval, QBrush(vuGrad)); //, config.ctrlGraphFg);
					p.setPen(tickColor);
					p.drawLine(x1, lval, tick, lval);
				}
				else
				{
					int ghostedAlpha = tconfig().get_property("PerformerEdit", "renderalpha", 50).toInt();
					if(noteAlphaAction && !noteAlphaAction->isChecked())
						ghostedAlpha = 0;
					tickColor.setAlpha(ghostedAlpha);
					p.setPen(tickColor);
					p.drawLine(x1, lval, tick, lval);
				}
			}


			x1 = tick;
			if (val == CTRL_VAL_UNKNOWN)
				lval = CTRL_VAL_UNKNOWN;
			else
			{
				if (_cnum == CTRL_PROGRAM)
					lval = wh - ((pval - min - bias) * wh / (max - min));
				else
					lval = wh - ((val - min - bias) * wh / (max - min));
			}
		}
		if(!fg)
		{
			QColor tickColor = QColor(config.partColors[part->colorIndex()]);
			//tickColor.setAlpha(127);
			p.setPen(tickColor);
			p.drawLine(x1, lval-1, x + w, lval-1);
		}	
		if (lval == CTRL_VAL_UNKNOWN)
		{
			if (!fg)
			{
				//p.fillRect(x1, 0, (x + w) - x1, wh, QColor(192, 192, 192, 127));
				noEvents = true;
			}
		}
		else
		{
			if (fg)
			{
				int ghostedAlpha = tconfig().get_property("PerformerEdit", "renderalpha", 50).toInt();
				if(noteAlphaAction && !noteAlphaAction->isChecked())
					ghostedAlpha = 0;
				QColor tickColor = QColor(config.partColors[part->colorIndex()]);
				tickColor.setAlpha(ghostedAlpha);
				p.setPen(tickColor);
				p.drawLine(x1, lval, x + w, lval);
			}
			else
			{
				p.setPen(myPen);
				p.fillRect(x1, lval, (x + w) - x1, wh - lval, QBrush(vuGrad)); //, config.ctrlGraphFg);
			}
		}
	}
}/*}}}*/

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void CtrlCanvas::pdraw(QPainter& p, const QRect& rect)/*{{{*/
{

	int x = rect.x() - 1; // compensate for 3 pixel line width
	int y = rect.y();
	int w = rect.width() + 2;
	int h = rect.height();

	//---------------------------------------------------
	// draw the grid
	//---------------------------------------------------

	p.save();
	View::pdraw(p, rect);
	p.restore();

	//---------------------------------------------------
	// draw Canvas Items
	//---------------------------------------------------
	
	if(!m_collapsed)
	{
		bool velo = (midiControllerType(_controller->num()) == MidiController::Velo);
		if (!velo)
		{
			pdrawItems(p, rect, curPart, false, false);
		}
		for (iPart ip = editor->parts()->begin(); ip != editor->parts()->end(); ++ip)
		{
			MidiPart* part = (MidiPart*) (ip->second);
			if (part == curPart)
				continue;
			pdrawItems(p, rect, part, velo, !velo);
		}
		if (velo)
		{
			pdrawItems(p, rect, curPart, true, true);
		}
	}

	//---------------------------------------------------
	//    draw marker
	//---------------------------------------------------

	int xp = mapx(pos[1]);
	if ((song->loop() || song->punchin()) && xp >= x && xp < x + w)
	{
		p.setPen(QColor(139, 225, 69));
		p.drawLine(xp, y, xp, y + h);
	}
	xp = mapx(pos[2]);
	if ((song->loop() || song->punchout()) && xp >= x && xp < x + w)
	{
		p.setPen(QColor(139, 225, 69));
		p.drawLine(xp, y, xp, y + h);
	}

	p.setPen(QColor(156,75,219));
	xp = mapx(pos[3]);
	if (pos[3] != MAXINT)
	{
		if (xp >= x && xp < x + w)
			p.drawLine(xp, y, xp, y+h);
	}

	xp = mapx(pos[0]);
	if (xp >= x && xp < x + w)
	{
		p.setPen(QColor(0, 186, 255));
		p.drawLine(xp, y, xp, y + h);
	}

	//---------------------------------------------------
	//    draw lasso
	//---------------------------------------------------

	if (!m_collapsed && drag == DRAG_LASSO)
	{
		QColor outlineColor = QColor(config.partColors[curPart->colorIndex()]);
		QColor fillColor = QColor(config.partWaveColors[curPart->colorIndex()]);
		fillColor.setAlpha(127);
		setPainter(p);
		QPen mypen2 = QPen(outlineColor, 2, Qt::SolidLine);
		mypen2.setCosmetic(true);
		p.setPen(mypen2);
		p.setBrush(QBrush(fillColor));
		p.drawRect(lasso);
	}
}/*}}}*/

//---------------------------------------------------------
//   drawOverlay
//---------------------------------------------------------

void CtrlCanvas::drawOverlay(QPainter& p, const QRect&)
{
	QString s;
	
	p.setFont(config.fonts[3]);
	
	QColor textColor = QColor(0,0,0,180);
	if(curPart)
	{
		s.append(curPart->track()->name()).append(" : ");
		textColor = QColor(config.partColors[curPart->colorIndex()]);
	}
	s.append(_controller->name());
	textColor.setAlpha(180);
	p.setPen(textColor);
	
	switch(m_feedbackMode)
	{
		case 0:
			s.append(tr(" (READ)"));
		break;
		case 1:
			s.append(tr(" (WRITE)"));
		break;
		case 2:
			s.append(tr(" (TOUCH)"));
		break;
		case 3:
			s.append(tr(" (AUDITION)"));
		break;
	}

	QFontMetrics fm(config.fonts[3]);
	int y = fm.lineSpacing() + 2;
	
	p.drawText(2, y, s);

	if (noEvents && !m_collapsed)
	{
		p.setFont(config.fonts[3]);
        p.drawText(width() / 2 - 100, height() / 2 - 10, "Use pencil or line tool to draw new events");
	}
}

//---------------------------------------------------------
//   overlayRect
//    returns geometry of overlay rectangle
//---------------------------------------------------------

QRect CtrlCanvas::overlayRect() const
{
	QFontMetrics fm(config.fonts[3]);
	QRect r(fm.boundingRect(_controller ? _controller->name() : QString("")));
	r.translate(2, 2); // top/left margin
	return r;
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void CtrlCanvas::draw(QPainter& p, const QRect& rect)
{
	drawTickRaster(p, rect.x(), rect.y(), rect.width(), rect.height(), editor->raster(), true);

	//---------------------------------------------------
	//    draw line tool
	//---------------------------------------------------

	if (drawLineMode && (tool == DrawTool))
	{
		p.setRenderHint(QPainter::Antialiasing, true);
		//p.setPen(Qt::black);
		QPen mypen = QPen(QColor(config.partColors[curPart->colorIndex()]), 2, Qt::SolidLine);
		p.setPen(mypen);
		p.drawLine(line1x, line1y, line2x, line2y);
	}
}

//---------------------------------------------------------
//   setCurDrumInstrument
//---------------------------------------------------------

void CtrlCanvas::setCurDrumInstrument(int di)
{
	curDrumInstrument = di;
}
