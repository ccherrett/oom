//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 - 2012 Andrew Williams and Christopher Cherrett
//=========================================================

#include <stdio.h>
#include <values.h>

#include "canvas.h"

#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QCursor>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QComboBox>

#include "app.h"
#include "config.h"
#include "globals.h"
#include "gconfig.h"
#include "Composer.h"
#include "song.h"
#include "event.h"
#include "citem.h"
#include "icons.h"
#include "../marker/marker.h"
#include "part.h"
#include "utils.h"

#define ABS(x)  ((x) < 0) ? -(x) : (x)
bool PartZIndex = false;

//---------------------------------------------------------
//   Canvas
//---------------------------------------------------------

Canvas::Canvas(QWidget* parent, int sx, int sy, const char* name)
	: View(parent, sx, sy, name) ,_drawPartLines(false)
{
	_canvasTools = 0;
	_itemPopupMenu = 0;
	m_PartZIndex = false;

	_button = Qt::NoButton;
	_keyState = 0;

	_canScrollLeft = true;
	_canScrollRight = true;
	_canScrollUp = true;
	_canScrollDown = true;
	_hscrollDir = HSCROLL_NONE;
	_vscrollDir = VSCROLL_NONE;
	_scrollTimer = NULL;

	_scrollSpeed = 10; // hardcoded scroll jump

	_drag = DRAG_OFF;
	_tool = PointerTool;
	_pos[0] = song->cpos();
	_pos[1] = song->lpos();
	_pos[2] = song->rpos();
	_curPart = NULL;
	_curPartId = -1;
	_curItem = NULL;
	_selectedProgramPos = -1;
	_drawSelectedProgram = false;
	_drawPartLines = false;
	_drawPartEndLine = false;
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
}

//---------------------------------------------------------
//   setPos
//    set one of three markers
//    idx   - 0-cpos  1-lpos  2-rpos
//    flag  - emit followEvent()
//---------------------------------------------------------

void Canvas::setPos(int idx, unsigned val, bool adjustScrollbar)
{
	//if (pos[idx] == val) // Seems to be some refresh problems here, pos[idx] might be val but the gui not updated.
	//    return;          // skipping this return forces update even if values match. Matching values only seem
	// to occur when initializing
	int opos = mapx(_pos[idx]);
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
				int ppos = val - xorg - rmapxDev(width() / 8);
				if (ppos < 0)
					ppos = 0;
				emit followEvent(ppos);
				opos = mapx(_pos[idx]);
				npos = mapx(val);
			}
			else if (npos < 0)
			{
				int ppos = val - xorg - rmapxDev(width()*3 / 4);
				if (ppos < 0)
					ppos = 0;
				emit followEvent(ppos);
				opos = mapx(_pos[idx]);
				npos = mapx(val);
			}
			break;
			case Song::CONTINUOUS:
			if (npos > (width() / 2))
			{
				int ppos = _pos[idx] - xorg - rmapxDev(width() / 2);
				if (ppos < 0)
					ppos = 0;
				emit followEvent(ppos);
				opos = mapx(_pos[idx]);
				npos = mapx(val);
			}
			else if (npos < (width() / 2))
			{
				int ppos = _pos[idx] - xorg - rmapxDev(width() / 2);
				if (ppos < 0)
					ppos = 0;
				emit followEvent(ppos);
				opos = mapx(_pos[idx]);
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
	_pos[idx] = val;
	//redraw(QRect(x - 1, 0, w + 2, height()));
	update();
}

bool Canvas::smallerZValue(const CItem* first, const CItem* second)
{
	return first->zValue(PartZIndex) < second->zValue(PartZIndex);
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Canvas::draw(QPainter& p, const QRect& rect)/*{{{*/
{
	//      printf("draw canvas %x virt %d\n", this, virt());

	int x = rect.x();
	int y = rect.y();
	int w = rect.width();
	int h = rect.height();
	int x2 = x + w;

	if (virt())
	{
		//printf("If virt() code running\n");
		drawCanvas(p, rect);

		//---------------------------------------------------
		// draw Canvas Items
		//---------------------------------------------------

		iCItem to(_items.lower_bound(x2));

		// Draw items from other parts behind all others.
		// Only for items with events (not Composer parts).
		QList<CItem*> sortedByZValue;

		// Draw items from other parts behind all others.
		// Only for items with events (not Composer parts).
		for (iCItem i = _items.begin(); i != to; ++i)
		{
			sortedByZValue.append(i->second);
		}
		PartZIndex = m_PartZIndex;

		qSort(sortedByZValue.begin(), sortedByZValue.end(), Canvas::smallerZValue);

		foreach(CItem* ci, sortedByZValue)
		{
			if (!ci->event().empty() && ci->part() != _curPart)
			{
				drawItem(p, ci, rect);
			}
			else if (!ci->isMoving() && (ci->event().empty() || ci->part() == _curPart))
			{
				drawItem(p, ci, rect);
			}
		}
		to = _moving.lower_bound(x2);
		for (iCItem i = _moving.begin(); i != to; ++i)
		{
			drawItem(p, i->second, rect);
		}

		drawTopItem(p,rect);
	}
	else
	{
		//printf("If NOT virt() code running\n");
		p.save();
		setPainter(p);

		if (xmag <= 0)
		{
			x -= 1;
			w += 2;
			x = (x + xpos + rmapx(xorg)) * (-xmag);
			w = w * (-xmag);
		}
		else
		{
			x = (x + xpos + rmapx(xorg)) / xmag;
			w = (w + xmag - 1) / xmag;
			x -= 1;
			w += 2;
		}
		if (ymag <= 0)
		{
			y -= 1;
			h += 2;
			y = (y + ypos + rmapy(yorg)) * (-ymag);
			h = h * (-ymag);
		}
		else
		{
			y = (rect.y() + ypos + rmapy(yorg)) / ymag;
			h = (rect.height() + ymag - 1) / ymag;
			y -= 1;
			h += 2;
		}

		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		x2 = x + w;

		drawCanvas(p, QRect(x, y, w, h));
		p.restore();

		//---------------------------------------------------
		// draw Canvas Items
		//---------------------------------------------------

		// Draw items from other parts behind all others.
		// Only for items with events (not Composer parts).
		for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			CItem* ci = i->second;
			if (!ci->event().empty() && ci->part() != _curPart)
			{
				drawItem(p, ci, rect);
			}
			else if (!ci->isMoving() && (ci->event().empty() || ci->part() == _curPart))
			{
				drawItem(p, ci, rect);
			}
		}

		/*for (iCItem i = items.begin(); i != items.end(); ++i)
		{
			CItem* ci = i->second;
			// Draw unselected parts behind selected.
			if (!ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
			{
				drawItem(p, ci, rect);
			}
		}*/

		// Draw selected parts in front of unselected.
		/*for (iCItem i = items.begin(); i != items.end(); ++i)
		{
			CItem* ci = i->second;
			if (ci->isSelected() && !ci->isMoving() && (ci->event().empty() || ci->part() == curPart))
			{
				drawItem(p, ci, rect);
			}
		}*/
		for (iCItem i = _moving.begin(); i != _moving.end(); ++i)
		{
			drawItem(p, i->second, rect);
		}
		drawTopItem(p, QRect(x,y,w,h));
		p.save();
		setPainter(p);
	}

	//---------------------------------------------------
	//    draw marker
	//---------------------------------------------------

	int y2 = y + h;
	MarkerList* marker = song->marker();
	for (iMarker m = marker->begin(); m != marker->end(); ++m)
	{
		int xp = m->second.tick();
		if (xp >= x && xp < x + w)
		{
			p.setPen(QColor(243,191,124));
			p.drawLine(xp, y, xp, y2);
		}
	}

	//Draw selected program change marker line
	if(_drawSelectedProgram && _selectedProgramPos >= 0)
	{
		//printf("Canvas::draw() drawing selected PC line\n");
		if (_selectedProgramPos >= x && _selectedProgramPos < (x + w))
		{
			//p.setPen(QColor(243,206,105));
			p.setPen(QColor(200,146,0));
			p.drawLine(_selectedProgramPos, y, _selectedProgramPos, y2);
		}
	}

	

	// //---------------------------------------------------
	// //    draw location marker
	// //---------------------------------------------------

	// p.setPen(Qt::blue);
	// if (pos[1] >= unsigned(x) && pos[1] < unsigned(x2))
	// {
	//       p.drawLine(pos[1], y, pos[1], y2);
	// }
	// if (pos[2] >= unsigned(x) && pos[2] < unsigned(x2))
	//       p.drawLine(pos[2], y, pos[2], y2);
	//
	// QPen playbackPen(QColor(51,56,55), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	// p.setPen(playbackPen);
	// //p.setPen(Qt::red);

	// if (pos[0] >= unsigned(x) && pos[0] < unsigned(x2))
	// {
	//       p.drawLine(pos[0], y, pos[0], y2);
	// }
	//

	//---------------------------------------------------
	//    draw lasso
	//---------------------------------------------------

    if (_drag == DRAG_LASSO)
	{
		QColor outlineColor = QColor(145,255,250);
		QColor fillColor = QColor(63,63,63);
		if(_curPart)
		{
			outlineColor = QColor(config.partColors[_curPart->colorIndex()]);
			fillColor = QColor(config.partWaveColors[_curPart->colorIndex()]);
		}	
		fillColor.setAlpha(127);
		QPen mypen3 = QPen(outlineColor, 2, Qt::SolidLine);
		mypen3.setCosmetic(true);
		p.setPen(mypen3);
		//p.setPen(QColor(181, 109, 16));
		p.setBrush(QBrush(fillColor));
        p.drawRect(_lasso);
	}

	//---------------------------------------------------
	//    draw moving items
	//---------------------------------------------------

	if (virt())
	{
		for (iCItem i = _moving.begin(); i != _moving.end(); ++i)
			drawMoving(p, i->second, rect);
	}
	else
	{
		p.restore();
		for (iCItem i = _moving.begin(); i != _moving.end(); ++i)
			drawMoving(p, i->second, rect);
		setPainter(p);
	}
	//---------------------------------------------------
	//    draw location marker
	//---------------------------------------------------

	p.setPen(QColor(139, 225, 69));
	if ((song->loop() || song->punchin()) && _pos[1] >= unsigned(x) && _pos[1] < unsigned(x2))
	{
		p.drawLine(_pos[1], y, _pos[1], y2);
	}
	if ((song->loop() || song->punchout()) && _pos[2] >= unsigned(x) && _pos[2] < unsigned(x2))
		p.drawLine(_pos[2], y, _pos[2], y2);

	//Draw part end start lines
	if(_curPart && _drawPartLines)
	{
		QColor lineColor(config.partColors[_curPart->colorIndex()]);
		lineColor.setAlpha(200);
		//QColor fillColor(config.partWaveColors[_curPart->colorIndex()]);
		QPen posPen(lineColor, 5);
		//QBrush fillBrush(fillColor);
		posPen.setCosmetic(true);
		p.setPen(posPen);
		//p.setBrush(fillBrush);
		//p.drawRect(QRect(_curPart->tick(), y, 10, y2));
		//p.drawRect(QRect(_curPart->endTick(), y, 10, y2));
		p.drawLine(_curPart->tick(), y, _curPart->tick(), y2);
		if(_drawPartEndLine)
			p.drawLine(_curPart->endTick(), y, _curPart->endTick(), y2);
	}

	p.setPen(QColor(156,75,219));
	if (_pos[3] != MAXINT)
	{
		if (_pos[3] >= unsigned(x) && _pos[3] < unsigned(x2))
			p.drawLine(_pos[3], y, _pos[3], y2);
	}

	p.setPen(QColor(0, 186, 255));
    if (_pos[0] >= unsigned(x) && _pos[0] < unsigned(x2))
	{
    	p.drawLine(_pos[0], y, _pos[0], y2);
	}

}/*}}}*/

void Canvas::drawSelectedProgram(int pos, bool set)
{
	_selectedProgramPos = pos;
	_drawSelectedProgram = set;
	redraw();
	//printf("Canvas::drawSelectedProgram\n");
	//update();
}

#define WHEEL_STEPSIZE 40
#define WHEEL_DELTA   120

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void Canvas::wheelEvent(QWheelEvent* ev)
{
	int delta = ev->delta() / WHEEL_DELTA;
	int ypixelscale = rmapyDev(1);

	if (ypixelscale <= 0)
		ypixelscale = 1;

	int scrollstep = WHEEL_STEPSIZE * (-delta);
	///if (ev->state() == Qt::ShiftModifier)
	if (((QInputEvent*) ev)->modifiers() == Qt::ShiftModifier)
		scrollstep = scrollstep / 10;

	int newYpos = ypos + ypixelscale * scrollstep;

	if (newYpos < 0)
		newYpos = 0;

	//setYPos(newYpos);
	emit verticalScroll((unsigned) newYpos);

}

void Canvas::redirectedWheelEvent(QWheelEvent* ev)
{
	wheelEvent(ev);
}

//---------------------------------------------------------
//   deselectAll
//---------------------------------------------------------

void Canvas::deselectAll()
{
	for (iCItem i = _items.begin(); i != _items.end(); ++i)
		i->second->setSelected(false);
	//m_multiSelect.clear();
}

//---------------------------------------------------------
//   selectItem
//---------------------------------------------------------

void Canvas::selectItem(CItem* e, bool flag)
{
	e->setSelected(flag);
}

//---------------------------------------------------------
//   startMoving
//    copy selection-List to moving-List
//---------------------------------------------------------

void Canvas::startMoving(const QPoint& pos, DragType)
{
	for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->second->isSelected())
		{
			i->second->setMoving(true);
			_moving.add(i->second);
		}
	}
	moveItems(pos, 0);
}

//---------------------------------------------------------
//   moveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void Canvas::moveItems(const QPoint& pos, int dir, bool rasterize)
{
	int dp;
	if (rasterize)
		dp = y2pitch(pos.y()) - y2pitch(_start.y());
	else
		dp = pos.y() - _start.y();
	int dx = pos.x() - _start.x();
	if (dir == 1)
		dp = 0;
	else if (dir == 2)
		dx = 0;
	for (iCItem i = _moving.begin(); i != _moving.end(); ++i)
	{
		int x = i->second->pos().x();
		int y = i->second->pos().y();
		int nx = x + dx;
		int ny;
		QPoint mp;
		if (rasterize)
		{
			ny = pitch2y(y2pitch(y) + dp);
			mp = raster(QPoint(nx, ny));
		}
		else
		{
			ny = y + dp;
			mp = QPoint(nx, ny);
		}
		if (i->second->mp() != mp)
		{
			i->second->setMp(mp);
			itemMoved(i->second, mp);
		}
	}
	redraw();
}

//---------------------------------------------------------
//   viewKeyPressEvent
//---------------------------------------------------------

void Canvas::viewKeyPressEvent(QKeyEvent* event)
{
	keyPress(event);
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void Canvas::viewMousePressEvent(QMouseEvent* event)/*{{{*/
{
	///keyState = event->state();
	_keyState = ((QInputEvent*) event)->modifiers();
	_button = event->button();

	//printf("viewMousePressEvent buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());

	// special events if right button is clicked while operations
	// like moving or drawing lasso is performed.
	///if (event->stateAfter() & Qt::RightButton) {
	if (event->buttons() & Qt::RightButton & ~(event->button()))
	{
		//printf("viewMousePressEvent special buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
		switch (_drag)
		{
		case DRAG_LASSO:
			_drag = DRAG_OFF;
			redraw();
			return;
		case DRAG_MOVE:
			_drag = DRAG_OFF;
			endMoveItems(_start, MOVE_MOVE, 0);
			return;
		default:
			break;
		}
	}

	// ignore event if (another) button is already active:
	///if (keyState & (Qt::LeftButton|Qt::RightButton|Qt::MidButton)) {
	if (event->buttons() & (Qt::LeftButton | Qt::RightButton | Qt::MidButton) & ~(event->button()))
	{
		//printf("viewMousePressEvent ignoring buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
		return;
	}
	bool shift = _keyState & Qt::ShiftModifier;
	bool alt = _keyState & Qt::AltModifier;
	bool ctrl = _keyState & Qt::ControlModifier;
	_start = event->pos();

	//---------------------------------------------------
	//    set curItem to item mouse is pointing
	//    (if any)
	//---------------------------------------------------

	if (virt())
		_curItem = _items.find(_start);
	else
	{
		_curItem = 0;
		iCItem ius;
		bool usfound = false;
		for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			QRect box = i->second->bbox();
			int x = rmapxDev(box.x());
			int y = rmapyDev(box.y());
			int w = rmapxDev(box.width());
			int h = rmapyDev(box.height());
			QRect r(x, y, w, h);
			///r.moveBy(i->second->pos().x(), i->second->pos().y());
			r.translate(i->second->pos().x(), i->second->pos().y());
			if (r.contains(_start))
			{
				if (i->second->isSelected())
				{
					_curItem = i->second;
					break;
				}
				else if (!usfound)
				{
					ius = i;
					usfound = true;
				}
			}
		}
		if (!_curItem && usfound)
			_curItem = ius->second;
	}

	if (_curItem && (event->button() == Qt::MidButton))
	{
		if (!_curItem->isSelected())
		{
			selectItem(_curItem, true);
			updateSelection();
			redraw();
		}
		startDrag(_curItem, shift);
	}
	else if (event->button() == Qt::RightButton)
	{
		if (_curItem)
		{
			if (shift)
			{
				_drag = DRAG_RESIZE;
				setCursor();
				int dx = _start.x() - _curItem->x();
				_curItem->setWidth(dx);
				_start.setX(_curItem->x());
				deselectAll();
				selectItem(_curItem, true);
				updateSelection();
				redraw();
			}
			else
			{
				_itemPopupMenu = genItemPopup(_curItem);
				if (_itemPopupMenu)
				{
					QAction *act = _itemPopupMenu->exec(QCursor::pos());
					if (act)
						itemPopup(_curItem, act->data().toInt(), _start);
					if(_itemPopupMenu)
						delete _itemPopupMenu;
				}
			}
		}
		else
		{
			_canvasPopupMenu = genCanvasPopup();
			if (_canvasPopupMenu)
			{
				QAction *act = _canvasPopupMenu->exec(QCursor::pos(), 0);
				if (act)
					canvasPopup(act->data().toInt());
				if(_canvasPopupMenu)
					delete _canvasPopupMenu;
			}
		}
	}
	else if (event->button() == Qt::LeftButton)
	{
		switch (_tool)
		{
			case PointerTool:
			if (_curItem)
			{
				Track* ctrack = _curItem->part()->track();
				if(ctrack && ctrack->isMidiTrack())
				{
					oom->composer->_setRaster(config.midiRaster, false);
					//oom->composer->raster->setCurrentIndex(config.midiRaster);
				}
				else
				{
					oom->composer->_setRaster(config.audioRaster, false);
					//oom->composer->raster->setCurrentIndex(config.audioRaster);
				}
				if (_curItem->part() != _curPart)
				{
					_curPart = _curItem->part();
					_curPartId = _curPart->sn();
					curPartChanged();
				}
				itemPressed(_curItem);
				// Changed by T356. Alt is default reserved for moving the whole window in KDE. Changed to Shift-Alt.
				// Hmm, nope, shift-alt is also reserved sometimes. Must find a way to bypass,
				//  why make user turn off setting? Left alone for now...
				if (shift)
					_drag = DRAG_COPY_START;
				else if (alt)
				{
					_drag = DRAG_CLONE_START;
				}
				//
				//if (shift)
				//{
				//  if (alt)
				//    drag = DRAG_CLONE_START;
				//  else
				//    drag = DRAG_COPY_START;
				//}
				else if (ctrl)
				{ //Select all on the same pitch (e.g. same y-value)
					deselectAll();
					//printf("Yes, ctrl and press\n");
					for (iCItem i = _items.begin(); i != _items.end(); ++i)
					{
						if (i->second->y() == _curItem->y())
							selectItem(i->second, true);
					}
					updateSelection();
					redraw();
				}
				else
					_drag = DRAG_MOVE_START;
			}
			else
				_drag = DRAG_LASSO_START;
			setCursor();
			break;

			case RubberTool:
			deleteItem(_start);
			_drag = DRAG_DELETE;
			setCursor();
			break;

			case PencilTool:
			if (_curItem)
			{
				Track* ctrack = _curItem->part()->track();
				if(ctrack && ctrack->isMidiTrack())
				{
					oom->composer->_setRaster(config.midiRaster, false);
					oom->composer->raster->setCurrentIndex(config.midiRaster);
				}
				else
				{
					oom->composer->_setRaster(config.audioRaster, false);
					oom->composer->raster->setCurrentIndex(config.audioRaster);
				}
				if(shift && ctrack->type() == Track::WAVE)
				{
					_drag = DRAG_RESIZE_LEFT;
					//Christopher
					//Set up you flag here to draw the left border
					//Unset both flags in viewMouseRelease() function below
					m_myLeftFlag = true;
					setCursor();
					int endp = _curItem->x() + _curItem->width();
					_end.setX(endp);

					int dx = _end.x() - _start.x();
					_curItem->setWidth(dx);
					QPoint np(_start.x(), _curItem->y());
					_curItem->move(np);
				}
				else
				{
					_drag = DRAG_RESIZE;
					//Christopher
					//Set up you flag here to draw the right border
					//Unset both flags in viewMouseRelease() function below
					m_myRightFlag = true;
					setCursor();
					int dx = _start.x() - _curItem->x();
					_curItem->setWidth(dx);
					_start.setX(_curItem->x());
				}
			}
			else
			{
				_drag = DRAG_NEW;
				setCursor();
				_curItem = newItem(_start, event->modifiers());
				if (_curItem)
					_items.add(_curItem);
				else
				{
					_drag = DRAG_OFF;
					setCursor();
				}
			}
			deselectAll();
			if (_curItem)
			{
				selectItem(_curItem, true);
				//song->deselectTracks();
				//_curItem->part()->track()->setSelected(true);
				//song->update(SC_SELECTION);
			}
			updateSelection();
			redraw();
			break;

            case StretchTool: // copied from pencil tool
            if (_curItem)
            {
                Track* ctrack = _curItem->part()->track();
                if (ctrack && ctrack->type() == Track::WAVE)
                {
                    _drag = DRAG_RESIZE;
                    //Christopher
                    //Set up you flag here to draw the right border
                    //Unset both flags in viewMouseRelease() function below
                    m_myRightFlag = true;
                    setCursor();
                    int dx = _start.x() - _curItem->x();
                    _curItem->setWidth(dx);
                    _start.setX(_curItem->x());
                }
            }
            break;
            
			default:
			break;
		}
	}
	mousePress(event);
}/*}}}*/

void Canvas::scrollTimerDone()/*{{{*/
{
	//printf("Canvas::scrollTimerDone drag:%d doScroll:%d\n", drag, doScroll);

	if (_drag != DRAG_OFF && _doScroll)
	{
		//printf("Canvas::scrollTimerDone drag != DRAG_OFF && doScroll\n");

		bool doHMove = false;
		bool doVMove = false;
		int hoff = rmapx(xOffset()) + mapx(xorg) - 1;
		int curxpos;
		switch (_hscrollDir)
		{
		case HSCROLL_RIGHT:
			hoff += _scrollSpeed;
			switch (_drag)
			{
			case DRAG_NEW:
			case DRAG_RESIZE:
			case DRAG_RESIZE_LEFT:
			case DRAGX_MOVE:
			case DRAGX_COPY:
			case DRAGX_CLONE:
			case DRAGY_MOVE:
			case DRAGY_COPY:
			case DRAGY_CLONE:
			case DRAG_MOVE:
			case DRAG_COPY:
			case DRAG_CLONE:
				emit horizontalScrollNoLimit(hoff);
				_canScrollLeft = true;
				_evPos.setX(rmapxDev(rmapx(_evPos.x()) + _scrollSpeed));
				doHMove = true;
				break;
			default:
				if (_canScrollRight)
				{
					curxpos = xpos;
					emit horizontalScroll(hoff);
					if (xpos <= curxpos)
					{
						_canScrollRight = false;
					}
					else
					{
						_canScrollLeft = true;
						_evPos.setX(rmapxDev(rmapx(_evPos.x()) + _scrollSpeed));
						doHMove = true;
					}
				}
				else
				{
				}
				break;
			}
			break;
			case HSCROLL_LEFT:
			if (_canScrollLeft)
			{
				curxpos = xpos;
				hoff -= _scrollSpeed;
				emit horizontalScroll(hoff);
				if (xpos >= curxpos)
				{
					_canScrollLeft = false;
				}
				else
				{
					_canScrollRight = true;
					_evPos.setX(rmapxDev(rmapx(_evPos.x()) - _scrollSpeed));
					doHMove = true;
				}
			}
			else
			{
			}
			break;
			default:
			break;
		}
		int voff = rmapy(yOffset()) + mapy(yorg);
		int curypos;
		switch (_vscrollDir)
		{
		case VSCROLL_DOWN:
			if (_canScrollDown)
			{
				curypos = ypos;
				voff += _scrollSpeed;
				emit verticalScroll(voff);
				if (ypos <= curypos)
				{
					_canScrollDown = false;
				}
				else
				{
					_canScrollUp = true;
					_evPos.setY(rmapyDev(rmapy(_evPos.y()) + _scrollSpeed));
					doVMove = true;
				}
			}
			else
			{
			}
			break;
			case VSCROLL_UP:
			if (_canScrollUp)
			{
				curypos = ypos;
				voff -= _scrollSpeed;
				emit verticalScroll(voff);
				if (ypos >= curypos)
				{
					_canScrollUp = false;
				}
				else
				{
					_canScrollDown = true;
					_evPos.setY(rmapyDev(rmapy(_evPos.y()) - _scrollSpeed));
					doVMove = true;
				}
			}
			else
			{
			}
			break;
			default:
			break;
		}

		//printf("Canvas::scrollTimerDone doHMove:%d doVMove:%d\n", doHMove, doVMove);

		if (!doHMove && !doVMove)
		{
			delete _scrollTimer;
			_scrollTimer = NULL;
			_doScroll = false;
			return;
		}
		QPoint dist = _evPos - _start;
		switch (_drag)
		{
		case DRAG_MOVE:
		case DRAG_COPY:
		case DRAG_CLONE:
			moveItems(_evPos, 0, false);
			break;
		case DRAGX_MOVE:
		case DRAGX_COPY:
		case DRAGX_CLONE:
			moveItems(_evPos, 1, false);
			break;
		case DRAGY_MOVE:
		case DRAGY_COPY:
		case DRAGY_CLONE:
			moveItems(_evPos, 2, false);
			break;
		case DRAG_LASSO:
			_lasso = QRect(_start.x(), _start.y(), dist.x(), dist.y());
			redraw();
			break;
		case DRAG_NEW:
		case DRAG_RESIZE:
			if (dist.x())
			{
				if (dist.x() < 1)
					_curItem->setWidth(1);
				else
					_curItem->setWidth(dist.x());
				redraw();
			}
			break;
		case DRAG_RESIZE_LEFT:
			if (dist.x())
			{
				if(_evPos.x() < _end.x())
				{
					//_curItem->setWidth(_evPos.x());
					int dx = _end.x() - _evPos.x();
					//Need a setLWidth function that knows how to resize from left
					_curItem->setWidth(dx);
					QPoint np(_evPos.x(), _curItem->y());
					_curItem->move(np);
				}
			}
			break;
			default:
			break;
		}
		//printf("Canvas::scrollTimerDone starting scrollTimer: Currently active?%d\n", scrollTimer->isActive());

		// p3.3.43 Make sure to yield to other events (for up to 3 seconds), otherwise other events
		//  take a long time to reach us, causing scrolling to take a painfully long time to stop.
		// FIXME: Didn't help at all.
		//qApp->processEvents();
		// No, try up to 100 ms for each yield.
		//qApp->processEvents(100);
		//
		//scrollTimer->start( 40, TRUE ); // X ms single-shot timer
		// OK, changing the timeout from 40 to 80 helped.
		//scrollTimer->start( 80, TRUE ); // X ms single-shot timer
		_scrollTimer->setSingleShot(true);
		_scrollTimer->start(80);
	}
	else
	{
		//printf("Canvas::scrollTimerDone !(drag != DRAG_OFF && doScroll) deleting scrollTimer\n");

		delete _scrollTimer;
		_scrollTimer = NULL;
	}
}/*}}}*/


//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void Canvas::viewMouseMoveEvent(QMouseEvent* event)/*{{{*/
{

	_evPos = event->pos();
	QPoint dist = _evPos - _start;
	int ax = ABS(rmapx(dist.x()));
	int ay = ABS(rmapy(dist.y()));
	bool moving = (ax >= 2) || (ay > 2);

	// set scrolling variables: doScroll, scrollRight
	if (_drag != DRAG_OFF)
	{


		int ex = rmapx(event->x()) + mapx(0);
		if (ex < 40 && _canScrollLeft)
			_hscrollDir = HSCROLL_LEFT;
		else if (ex > (width() - 40))
			switch (_drag)
			{
			case DRAG_NEW:
			case DRAG_RESIZE:
			case DRAGX_MOVE:
			case DRAGX_COPY:
			case DRAGX_CLONE:
			case DRAGY_MOVE:
			case DRAGY_COPY:
			case DRAGY_CLONE:
			case DRAG_MOVE:
			case DRAG_COPY:
			case DRAG_CLONE:
			_hscrollDir = HSCROLL_RIGHT;
			break;
			case DRAG_RESIZE_LEFT:
			_hscrollDir = HSCROLL_LEFT;
			break;
			default:
			if (_canScrollRight)
				_hscrollDir = HSCROLL_RIGHT;
			else
				_hscrollDir = HSCROLL_NONE;
			break;
		}
		else
			_hscrollDir = HSCROLL_NONE;
		int ey = rmapy(event->y()) + mapy(0);
		if (ey < 15 && _canScrollUp)
			_vscrollDir = VSCROLL_UP;
		else
			if (ey > (height() - 15) && _canScrollDown)
				_vscrollDir = VSCROLL_DOWN;
		else
			_vscrollDir = VSCROLL_NONE;
		if (_hscrollDir != HSCROLL_NONE || _vscrollDir != VSCROLL_NONE)
		{
			_doScroll = true;
			if (!_scrollTimer)
			{
				_scrollTimer = new QTimer(this);
				connect(_scrollTimer, SIGNAL(timeout()), SLOT(scrollTimerDone()));
				//scrollTimer->start( 0, TRUE ); // single-shot timer
				_scrollTimer->setSingleShot(true); // single-shot timer
				_scrollTimer->start(0);
			}
		}
		else
			_doScroll = false;

	}
	else
	{
		_doScroll = false;

		_canScrollLeft = true;
		_canScrollRight = true;
		_canScrollUp = true;
		_canScrollDown = true;
	}

	switch (_drag)
	{
	case DRAG_LASSO_START:
		if (!moving)
			break;
		_drag = DRAG_LASSO;
		setCursor();
		// proceed with DRAG_LASSO:

	case DRAG_LASSO:
		{
			_lasso = QRect(_start.x(), _start.y(), dist.x(), dist.y());

			// printf("xorg=%d xmag=%d event->x=%d, mapx(xorg)=%d rmapx0=%d xOffset=%d rmapx(xOffset()=%d\n",
			//         xorg, xmag, event->x(),mapx(xorg), rmapx(0), xOffset(),rmapx(xOffset()));

		}
		redraw();
		break;

	case DRAG_MOVE_START:
	case DRAG_COPY_START:
	case DRAG_CLONE_START:
		if (!moving)
			break;
		if (_keyState & Qt::ControlModifier)
		{
			if (ax > ay)
			{
				if (_drag == DRAG_MOVE_START)
					_drag = DRAGX_MOVE;
				else if (_drag == DRAG_COPY_START)
					_drag = DRAGX_COPY;
				else
					_drag = DRAGX_CLONE;
			}
			else
			{
				if (_drag == DRAG_MOVE_START)
					_drag = DRAGY_MOVE;
				else if (_drag == DRAG_COPY_START)
					_drag = DRAGY_COPY;
				else
					_drag = DRAGY_CLONE;
			}
		}
		else
		{
			if (_drag == DRAG_MOVE_START)
				_drag = DRAG_MOVE;
			else if (_drag == DRAG_COPY_START)
				_drag = DRAG_COPY;
			else
				_drag = DRAG_CLONE;
		}
		setCursor();
		if (!_curItem->isSelected())
		{
			if (_drag == DRAG_MOVE)
				deselectAll();
			selectItem(_curItem, true);
			updateSelection();
			redraw();
		}
		DragType dt;
		if (_drag == DRAG_MOVE)
			dt = MOVE_MOVE;
		else if (_drag == DRAG_COPY)
			dt = MOVE_COPY;
		else
			dt = MOVE_CLONE;

		startMoving(_evPos, dt);
		break;

		case DRAG_MOVE:
		case DRAG_COPY:
		case DRAG_CLONE:

		if (!_scrollTimer)
			moveItems(_evPos, 0);
		break;

		case DRAGX_MOVE:
		case DRAGX_COPY:
		case DRAGX_CLONE:
		if (!_scrollTimer)
			moveItems(_evPos, 1);
		break;

		case DRAGY_MOVE:
		case DRAGY_COPY:
		case DRAGY_CLONE:
		if (!_scrollTimer)
			moveItems(_evPos, 2);
		break;

		case DRAG_NEW:
		case DRAG_RESIZE:
		if (dist.x())
		{
			if (dist.x() < 1)
				_curItem->setWidth(1);
			else
				_curItem->setWidth(dist.x());
			redraw();
		}
		break;
		case DRAG_RESIZE_LEFT:
		if (dist.x())
		{
			if(_evPos.x() < _end.x())
			{
				int dx = _end.x() - _evPos.x();
				_curItem->setWidth(dx);
				QPoint np(_evPos.x(), _curItem->y());
				_curItem->move(np);
			}
			redraw();
		}
		break;
		case DRAG_DELETE:
		deleteItem(_evPos);
		break;

		case DRAG_OFF:
		break;
		default:
		break;
	}

	_pos[3] = _evPos.x();
	redraw();
	mouseMove(event);
}/*}}}*/

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void Canvas::viewMouseReleaseEvent(QMouseEvent* event)/*{{{*/
{
	// printf("release %x %x\n", event->state(), event->button());

	//Christopher
	m_myLeftFlag = false;
	m_myRightFlag = false;
	_doScroll = false;
	_canScrollLeft = true;
	_canScrollRight = true;
	_canScrollUp = true;
	_canScrollDown = true;
	///if (event->state() & (Qt::LeftButton|Qt::RightButton|Qt::MidButton) & ~(event->button())) {
	if (event->buttons() & (Qt::LeftButton | Qt::RightButton | Qt::MidButton) & ~(event->button()))
	{
		///printf("ignore %x %x\n", keyState, event->button());
		//printf("viewMouseReleaseEvent ignore buttons:%x mods:%x button:%x\n", (int)event->buttons(), (int)keyState, event->button());
		return;
	}

	QPoint pos = event->pos();
	///bool shift = event->state() & Qt::ShiftModifier;
	bool shift = ((QInputEvent*) event)->modifiers() & Qt::ShiftModifier;
	bool redrawFlag = false;

	switch (_drag)
	{
	case DRAG_MOVE_START:
	case DRAG_COPY_START:
	case DRAG_CLONE_START:
		if (!shift)
			deselectAll();
		selectItem(_curItem, !(shift && _curItem->isSelected()));
		updateSelection();
		redrawFlag = true;
		itemReleased(_curItem, _curItem->pos());
		break;
	case DRAG_COPY:
		endMoveItems(pos, MOVE_COPY, 0);
		break;
	case DRAGX_COPY:
		endMoveItems(pos, MOVE_COPY, 1);
		break;
	case DRAGY_COPY:
		endMoveItems(pos, MOVE_COPY, 2);
		break;
	case DRAG_MOVE:
		endMoveItems(pos, MOVE_MOVE, 0);
		break;
	case DRAGX_MOVE:
		endMoveItems(pos, MOVE_MOVE, 1);
		break;
	case DRAGY_MOVE:
		endMoveItems(pos, MOVE_MOVE, 2);
		break;
	case DRAG_CLONE:
		endMoveItems(pos, MOVE_CLONE, 0);
		break;
	case DRAGX_CLONE:
		endMoveItems(pos, MOVE_CLONE, 1);
		break;
	case DRAGY_CLONE:
		endMoveItems(pos, MOVE_CLONE, 2);
		break;
	case DRAG_OFF:
		break;
	case DRAG_RESIZE:
		resizeItem(_curItem, false);
		break;
	case DRAG_RESIZE_LEFT:
		resizeItemLeft(_curItem, raster(pos), false);
		redrawFlag = true;
		break;
	case DRAG_NEW:
		newItem(_curItem, false);
		redrawFlag = true;
		break;
	case DRAG_LASSO_START:
		_lasso.setRect(-1, -1, -1, -1);
		if (!shift)
			deselectAll();
		updateSelection();
		redrawFlag = true;
		break;

	case DRAG_LASSO:
		if(_tool != AutomationTool)
		{
			if (!shift)
				deselectAll();
			_lasso = _lasso.normalized();
			selectLasso(shift);
			updateSelection();
			redrawFlag = true;
		}
		break;

	case DRAG_DELETE:
		break;
	default:
	break;
	}
	//printf("Canvas::viewMouseReleaseEvent setting drag to DRAG_OFF\n");

	if(_tool != AutomationTool)
		_drag = DRAG_OFF;
	if (redrawFlag)
		redraw();
	setCursor();
	mouseRelease(pos);
}/*}}}*/

//---------------------------------------------------------
//   selectLasso
//---------------------------------------------------------

void Canvas::selectLasso(bool toggle)/*{{{*/
{
	int n = 0;
	if (virt())
	{
		for (iCItem i = _items.begin(); i != _items.end(); ++i)
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
		for (iCItem i = _items.begin(); i != _items.end(); ++i)
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
}/*}}}*/

//---------------------------------------------------------
//   endMoveItems
//    dir = 0     move in all directions
//          1     move only horizontal
//          2     move only vertical
//---------------------------------------------------------

void Canvas::endMoveItems(const QPoint& pos, DragType dragtype, int dir)/*{{{*/
{
	startUndo(dragtype);

	int dp = y2pitch(pos.y()) - y2pitch(_start.y());
	int dx = pos.x() - _start.x();

	if (dir == 1)
		dp = 0;
	else if (dir == 2)
		dx = 0;



	int modified = 0;

	moveCanvasItems(_moving, dp, dx, dragtype, &modified);

	endUndo(dragtype, modified);
	_moving.clear();
	updateSelection();
	redraw();
}/*}}}*/

//---------------------------------------------------------
//   getCurrentDrag
//   returns 0 if there is no drag operation
//---------------------------------------------------------

int Canvas::getCurrentDrag()
{
	//printf("getCurrentDrag=%d\n", drag);
	return _drag;
}

CItemList Canvas::getItemlistForCurrentPart()/*{{{*/
{
	CItemList list;
	iCItem i;

	if (!_curPart)
	{
		return list;
	}

	if (_items.size() == 0) {
		return list;
	}

	i = _items.begin();
	while (i != _items.end())
	{
		if (i->second->part() == _curPart)
		{
			list.add(i->second);
		}

		++i;
	}

	return list;
}/*}}}*/

CItemList Canvas::getItemlistForPart(Part* part)/*{{{*/
{
	CItemList list;
	iCItem i;

	if (!part)
	{
		return list;
	}

	if (_items.size() == 0) {
		return list;
	}

	i = _items.begin();
	while (i != _items.end())
	{
		if (i->second->part() == part)
		{
			list.add(i->second);
		}

		++i;
	}

	return list;
}/*}}}*/

CItemList Canvas::getSelectedItemsForCurrentPart()
{
	CItemList list = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		list = getItemlistForCurrentPart();
	CItemList selected;

	iCItem i = list.begin();
	while (i != list.end())
	{
		if (i->second->isSelected())
		{
			selected.add(i->second);
		}

		++i;
	}

	return selected;
}

bool Canvas::allItemsAreSelected()
{
	CItemList list = _items;
	if(multiPartSelectionAction && !multiPartSelectionAction->isChecked())
		list = getItemlistForCurrentPart();
	//CItemList list = getItemlistForCurrentPart();
	CItemList selected = getSelectedItemsForCurrentPart();

	return (list.size() == selected.size());
}

//---------------------------------------------------------
//   deleteItem
//---------------------------------------------------------

void Canvas::deleteItem(const QPoint& p)
{
	if (virt())
	{
		for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			if (i->second->contains(p))
			{
				selectItem(i->second, false);
				if (!deleteItem(i->second))
				{
					if (_drag == DRAG_DELETE)
						_drag = DRAG_OFF;
				}
				break;
			}
		}
	}
	else
	{
		for (iCItem i = _items.begin(); i != _items.end(); ++i)
		{
			QRect box = i->second->bbox();
			int x = rmapxDev(box.x());
			int y = rmapyDev(box.y());
			int w = rmapxDev(box.width());
			int h = rmapyDev(box.height());
			QRect r(x, y, w, h);
			///r.moveBy(i->second->pos().x(), i->second->pos().y());
			r.translate(i->second->pos().x(), i->second->pos().y());
			if (r.contains(p))
			{
				if (deleteItem(i->second))
				{
					selectItem(i->second, false);
				}
				break;
			}
		}
	}
}

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void Canvas::setTool(int t)
{
	if (_tool == Tool(t))
		return;
	_tool = Tool(t);
	setCursor();
}

//---------------------------------------------------------
//   setCursor
//---------------------------------------------------------

void Canvas::setCursor()
{
	switch (_drag)
	{
	case DRAGX_MOVE:
	case DRAGX_COPY:
	case DRAGX_CLONE:
		QWidget::setCursor(QCursor(Qt::SizeHorCursor));
		break;

	case DRAGY_MOVE:
	case DRAGY_COPY:
	case DRAGY_CLONE:
		QWidget::setCursor(QCursor(Qt::SizeVerCursor));
		break;

	case DRAG_MOVE:
	case DRAG_COPY:
	case DRAG_CLONE:
		QWidget::setCursor(QCursor(Qt::SizeAllCursor));
		break;

	case DRAG_RESIZE:
	case DRAG_RESIZE_LEFT:
		QWidget::setCursor(QCursor(Qt::SizeHorCursor));
		break;

	case DRAG_DELETE:
	case DRAG_COPY_START:
	case DRAG_CLONE_START:
	case DRAG_MOVE_START:
	case DRAG_NEW:
	case DRAG_LASSO_START:
	case DRAG_LASSO:
	case DRAG_OFF:
		switch (_tool)
		{
		case PencilTool:
			QWidget::setCursor(QCursor(*pencilCursorIcon, 6, 15));
			break;
		case RubberTool:
			QWidget::setCursor(QCursor(*deleteIcon, 4, 15));
			break;
		case GlueTool:
			QWidget::setCursor(QCursor(*glueIcon, 4, 15));
			break;
		case CutTool:
			QWidget::setCursor(QCursor(*cutIcon, 4, 15));
			break;
		case MuteTool:
			QWidget::setCursor(QCursor(*editmuteIcon, 4, 15));
			break;
		case StretchTool:
			QWidget::setCursor(Qt::SplitHCursor);
			break;
		case AutomationTool:
			//QWidget::setCursor(QCursor(Qt::PointingHandCursor));
			QWidget::setCursor(QCursor(*crosshairIcon));
			break;
		default:
			QWidget::setCursor(QCursor(Qt::ArrowCursor));
			break;
		}
		break;
		default:
		break;
	}
}

//---------------------------------------------------------
//   keyPress
//---------------------------------------------------------

void Canvas::keyPress(QKeyEvent* event)
{
	event->ignore();
}

//---------------------------------------------------------
//   isSingleSelection
//---------------------------------------------------------

bool Canvas::isSingleSelection()
{
	return selectionSize() == 1;
}

//---------------------------------------------------------
//   selectionSize
//---------------------------------------------------------

int Canvas::selectionSize()
{
	int n = 0;
	for (iCItem i = _items.begin(); i != _items.end(); ++i)
	{
		if (i->second->isSelected())
			++n;
	}
	return n;
}

//---------------------------------------------------------
//   genCanvasPopup
//---------------------------------------------------------

QMenu* Canvas::genCanvasPopup(bool colors)
{
	if (_canvasTools == 0)
		return 0;
	QMenu* canvasPopup = new QMenu(this);
	if(colors)
	{
		QMenu* colorPopup = canvasPopup->addMenu(tr("Part Color"));/*{{{*/
	
		QMenu* colorSub; 
		for (int i = 0; i < NUM_PARTCOLORS; ++i)
		{
			QString colorname(config.partColorNames[i]);
			if(colorname.contains("menu:", Qt::CaseSensitive))
			{
				colorSub = colorPopup->addMenu(colorname.replace("menu:", ""));
			}
			else
			{
				if(_curPart)
				{
					if(_curPart->colorIndex() == i)
					{
						colorname = QString(config.partColorNames[i]);
						colorPopup->setIcon(colorRect(config.partColors[i], config.partWaveColors[i], 80, 80, true));
						colorPopup->setTitle(colorSub->title()+": "+colorname);
	
						colorname = QString("* "+config.partColorNames[i]);
						QAction *act_color = colorSub->addAction(colorRect(config.partColors[i], config.partWaveColors[i], 80, 80, true), colorname);
						act_color->setData(20 + i);
					}
					else
					{
						colorname = QString("     "+config.partColorNames[i]);
						QAction *act_color = colorSub->addAction(colorRect(config.partColors[i], config.partWaveColors[i], 80, 80), colorname);
						act_color->setData(20 + i);
					}
				}
				else
				{
					colorname = QString("     "+config.partColorNames[i]);
					QAction *act_color = colorSub->addAction(colorRect(config.partColors[i], config.partWaveColors[i], 80, 80), colorname);
					act_color->setData(20 + i);
				}
			}	
		}
	
		canvasPopup->addSeparator();/*}}}*/
	}
	QAction* act0 = 0;

	for (unsigned i = 0; i < 9; ++i)
	{
		if ((_canvasTools & (1 << i)) == 0)
			continue;
		QAction* act = canvasPopup->addAction(QIcon(**toolList[i].icon), tr(toolList[i].tip));
		act->setData(1 << i); // ddskrjo
		if (!act0)
			act0 = act;
	}
	canvasPopup->setActiveAction(act0);
	return canvasPopup;
}

//---------------------------------------------------------
//   canvasPopup
//---------------------------------------------------------

void Canvas::canvasPopup(int n)
{
	switch(n)
	{
		case 33 ... NUM_PARTCOLORS + 20:
		{
			int curColorIndex = n - 20;
			if (_curPart)
			{
				_curPart->setColorIndex(curColorIndex);
				song->update(SC_PART_COLOR_MODIFIED);
			}

			redraw();
			break;
		}
		default:
		{
			setTool(n);
			emit toolChanged(n);
			break;
		}
	}
}

void Canvas::setCurrentPart(Part* part)
{
	_curItem = NULL;
	deselectAll();
	_curPart = part;
	if (_curPart)
	{
		_curPartId = _curPart->sn();
	}
	curPartChanged();
}

void Canvas::updateCItemsZValues()
{
	for (iCItem k = _items.begin(); k != _items.end(); ++k)
	{
		CItem* cItem = k->second;
		if(cItem->part() == _curPart)
		{
			cItem->setZValue(1, false);
		}
		else
		{
			cItem->setZValue(0, false);
		}
	}
}
bool Canvas::isEventSelected(Event)
{
	return false;
}
