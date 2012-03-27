//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Andrew Williams & Christopher Cherrett
//=========================================================

#include <stdio.h>
#include <values.h>

#include <QtGui>

#include "globals.h"
#include "gconfig.h"
#include "TempoCanvas.h"
#include "song.h"
#include "track.h"
#include "app.h"
#include "scrollscale.h"
#include "midi.h"
#include "icons.h"
#include "audio.h"

extern void drawTickRaster(QPainter& p, int x, int y,
		int w, int h, int quant, bool ctrl);


//---------------------------------------------------------
//   TempoCanvas
//---------------------------------------------------------

TempoCanvas::TempoCanvas(QWidget* parent, int xmag)
: View(parent, xmag, -1)
{
	m_tempoStart = 750000;
	m_tempoEnd = 333333;
	setBg(QColor(63,63,63));
	m_drawLineMode = false;
	m_drawToolTip = false;
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = 0;
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	setFixedHeight(60);
	//setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
	connect(song, SIGNAL(songChanged(int)), this, SLOT(redraw()));
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void TempoCanvas::setPos(int idx, unsigned val, bool adjustScrollbar)/*{{{*/
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
					int ppos = val - rmapxDev(width() / 8);
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
				if (npos > (width() / 2))
				{
					int ppos = pos[idx] - rmapxDev(width() / 2);
					if (ppos < 0)
						ppos = 0;
					emit followEvent(ppos);
					opos = mapx(pos[idx]);
					npos = mapx(val);
				}
				else if (npos < (width() / 2))
				{
					int ppos = pos[idx] - rmapxDev(width() / 2);
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
	redraw(QRect(x - 1, 0, w + 2, height()));
}/*}}}*/

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void TempoCanvas::leaveEvent(QEvent*)
{
	m_drawToolTip = false;
	redraw();
	emit tempoChanged(-1);
	emit timeChanged(MAXINT);
}

//---------------------------------------------------------
//   pdraw
//---------------------------------------------------------

void TempoCanvas::pdraw(QPainter& p, const QRect& rect)/*{{{*/
{
	View::pdraw(p, rect); // calls draw()
	p.resetTransform();

	int x = rect.x();
	int y = rect.y();
	int w = rect.width() + 2;
	int h = rect.height();

	int wh = height();
	//qDebug("DEBUG: rect.h: %d, window.h: %d", h, wh);
	//---------------------------------------------------
	// draw Canvas Items
	//---------------------------------------------------

	const TempoList* tl = &tempomap;
	for (ciTEvent i = tl->begin(); i != tl->end(); ++i)
	{
		TEvent* e = i->second;
		int etick = mapx(i->first);
		int stick = mapx(i->second->tick);
		int tempo = tempoToPixel(e->tempo, h);

		if (tempo < 0)
			tempo = 0;

		if (tempo < wh)
		{
			QColor green = QColor(config.partWaveColors[1]);
			green.setAlpha(140);
			QColor yellow = QColor(41, 130, 140);
			QColor red = QColor(config.partColors[1]);
			red.setAlpha(140);
			QLinearGradient vuGrad(QPointF(0, 0), QPointF(0, height()));
			vuGrad.setColorAt(1, green);
			vuGrad.setColorAt(0, red);
			QPen myPen = QPen();
			myPen.setBrush(QBrush(vuGrad));
			p.fillRect(stick, tempo, etick - stick, wh, QBrush(vuGrad));
			p.setPen(red);
			p.drawLine(stick, tempo, etick, tempo);
		}
	}

	//---------------------------------------------------
	//    draw marker
	//---------------------------------------------------

	int xp = mapx(pos[1]);
	if (xp >= x && xp < x + w)
	{
		p.setPen(QColor(139, 225, 69));
		p.drawLine(xp, y, xp, y + h);
	}
	xp = mapx(pos[2]);
	if (xp >= x && xp < x + w)
	{
		p.setPen(QColor(139, 225, 69));
		p.drawLine(xp, y, xp, y + h);
	}

	xp = mapx(pos[3]);
	if (pos[3] != MAXINT)
	{
		p.setPen(QColor(156,75,219));
		if (xp >= x && xp < x + w)
			p.drawLine(xp, y, xp, y+h);
	}

	xp = mapx(pos[0]);
	if (xp >= x && xp < x + w)
	{
		p.setPen(QColor(0, 186, 255));
		p.drawLine(xp, y, xp, y + h);
	}
}/*}}}*/

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void TempoCanvas::draw(QPainter& p, const QRect& rect)/*{{{*/
{
	drawTickRaster(p, rect.x(), rect.y(), rect.width(), rect.height(), 0, false);
	if (m_drawLineMode && (tool == AutomationTool))
	{
		int pcolor = 1;
		QList<qint64> tracks = song->selectedTracks();
		if(tracks.isEmpty())
		{
			Track* t = song->findTrackById(tracks.at(0));
			if(t)
			{
				pcolor = t->getDefaultPartColor();
			}
		}
		p.setRenderHint(QPainter::Antialiasing, true);
		QPen mypen = QPen(QColor(config.partColors[pcolor]), 2, Qt::SolidLine);
		p.setPen(mypen);
		p.drawLine(line1x, mapy(line1y), line2x, mapy(line2y));
	}
}/*}}}*/

void TempoCanvas::drawOverlay(QPainter& p, const QRect&)/*{{{*/
{
	if(m_drawToolTip)
	{
		int tempo = computeTempo(lastPos.y(), (height() - 1));
		tempo = int(60000000.0 / tempo);
		QString tempoStr = QString::number(double(tempo));

		QColor textColor = config.partColors[1];//QColor(255,255,255,190);
		//QColor textColor = QColor(0,0,0,180);
		p.setRenderHint(QPainter::Antialiasing, false);
		p.setPen(textColor);
		p.setFont(QFont("fixed-width", 8, QFont::Bold));

		int y = lastPos.y();
		int x = rmapx(lastPos.x());
		if(y < 10)
			y += 10;
		if(y > (height() -6))
			y -= 6;
		if(x > (width() - 40))
			x -= 60;
		p.drawText(x + 20, y, tempoStr);
	}
}/*}}}*/
//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void TempoCanvas::mousePressEvent(QMouseEvent* event)/*{{{*/
{
	//start = event->pos();
	QPoint pos = mapDev(event->pos());
	start = QPoint(pos.x(), event->pos().y());
	Tool activeTool = tool;
	bool shift = ((QInputEvent*) event)->modifiers() & Qt::ShiftModifier;
	bool ctrl = ((QInputEvent*) event)->modifiers() & Qt::ControlModifier;
	bool alt = ((QInputEvent*) event)->modifiers() & Qt::AltModifier;
	if(shift || ctrl || alt)
		return;

	int xpos = start.x();
	int ypos = start.y();

	switch (activeTool)
	{
		case PointerTool:
			drag = DRAG_LASSO_START;
			break;

		case PencilTool:
			drag = DRAG_NEW;
			song->startUndo();
			
			newVal(start.x(), start.x(), start.y());
			//qDebug("TempoCanvas::viewMouseMoveEvent: realy: %d, y: %d", event->pos().y(), start.y());
			break;

		case RubberTool:
			drag = DRAG_DELETE;
			song->startUndo();
			deleteVal(start.x(), start.x());
			break;
		case AutomationTool:
			if (m_drawLineMode)
			{
				line2x = xpos;
				line2y = ypos;
				song->startUndo();
				newValRamp(line1x, line1y, line2x, line2y);
				m_drawLineMode = false;
				song->endUndo(SC_TEMPO);
			}
			else
			{
				line2x = line1x = xpos;
				line2y = line1y = ypos;
				m_drawLineMode = true;
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

void TempoCanvas::mouseMoveEvent(QMouseEvent* event)/*{{{*/
{
	QPoint tmpPos = mapDev(event->pos());
	QPoint pos = QPoint(tmpPos.x(), event->pos().y());
	bool shift = ((QInputEvent*) event)->modifiers() & Qt::ShiftModifier;
	bool ctrl = ((QInputEvent*) event)->modifiers() & Qt::ControlModifier;
	bool alt = ((QInputEvent*) event)->modifiers() & Qt::AltModifier;
	if(shift || ctrl || alt)
		return;
	//      QPoint dist = pos - start;
	//      bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;
	lastPos = pos;

	//QPoint dist = pos - start;
	//bool moving = dist.y() >= 3 || dist.y() <= 3 || dist.x() >= 3 || dist.x() <= 3;
	switch (drag)
	{
		case DRAG_NEW:
			newVal(start.x(), pos.x(), pos.y());
			start = pos;
			break;

		case DRAG_DELETE:
			deleteVal(start.x(), pos.x());
			start = pos;
			break;

		default:
			break;
	}
	if (tool == AutomationTool)
	{
		if(m_drawLineMode)
		{
			line2x = pos.x();
			line2y = pos.y();
		}
		m_drawToolTip = true;
		redraw();
	}
	else if(tool == PencilTool || tool == RubberTool)
	{
		m_drawToolTip = true;
		redraw();//Update for drawing tooltip
	}
	else
	{
		m_drawToolTip = false;
		redraw();//Update for drawing tooltip
	}
	emit tempoChanged(280000 - event->y());
	int x = pos.x();
	if (x < 0)
		x = 0;
	emit timeChanged(oom->rasterVal(x));
}/*}}}*/

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void TempoCanvas::viewMouseReleaseEvent(QMouseEvent*)/*{{{*/
{
	switch (drag)
	{
		case DRAG_RESIZE:
		case DRAG_NEW:
		case DRAG_DELETE:
			song->endUndo(SC_TEMPO);
			break;
		default:
			break;
	}
	drag = DRAG_OFF;
}/*}}}*/

void TempoCanvas::enterEvent(QEvent*)/*{{{*/
{
	Tool activeTool = tool;
	switch (activeTool)
	{
		case PencilTool:
			m_drawToolTip = true;
			redraw();
		break;
		case RubberTool:
			m_drawToolTip = true;
			redraw();
		break;
		case AutomationTool:
			m_drawToolTip = true;
			redraw();
		break;
		default:
			m_drawToolTip = false;
		break;
	}
}/*}}}*/

int TempoCanvas::computeTempo(int y, int height)
{
//	int a = 0;
	int b = height;
	int c = m_tempoEnd;//MIN_TEMPO;
	int d = m_tempoStart;//MAX_TEMPO;
	int range = d - c;
	double unit = range / b;
	int val = (y * unit) + c;

	//val = (val / 1000);
	return val;
}

int TempoCanvas::tempoToPixel(int tempo, int height)
{
//	int a = 0;
	int b = height;
	int c = m_tempoEnd; //MIN_TEMPO;
	int d = m_tempoStart; // MAX_TEMPO;
	int range = d - c;
	double unit = (range / b);
	//tempo = (tempo / 1000);
	int val = (tempo - c) / unit;
	return val;
}

//---------------------------------------------------------
//   deleteVal
//---------------------------------------------------------

bool TempoCanvas::deleteVal1(unsigned int x1, unsigned int x2)
{
	bool songChanged = false;

	TempoList* tl = &tempomap;
	QList<void*> delList;
	for (iTEvent i = tl->begin(); i != tl->end(); ++i)
	{
		if (i->first < x1)
			continue;
		if (i->first > x2)
			break;
		songChanged = true;
		iTEvent ii = i;
		++ii;
		if (ii != tl->end())
		{
			delList.append(ii->second);
			//int tempo = ii->second->tempo;
			//audio->msgDeleteTempo(i->first, tempo, false);
			songChanged = true;
		}
	}

	if(songChanged)
		audio->msgDeleteTempoRange(delList, false);
	/*foreach(TEvent* ev, delList)
	{
		audio->msgDeleteTempo(ev->tick, ev->tempo, false);
		songChanged = true;
	}*/
	return songChanged;
}

void TempoCanvas::deleteVal(int x1, int x2)
{
	//printf("TempoCanvas::deleteVal\n");
	if (deleteVal1(oom->rasterVal1(x1), x2))
		redraw();
}

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void TempoCanvas::setTool(int t)/*{{{*/
{
	//if (tool == Tool(t))
	//	return;
	tool = Tool(t);
	//qDebug("TempoCanvas::setTool: %d", t);
	switch (tool)
	{
		case PencilTool:
			//qDebug("TempoCanvas::setTool: PencilTool");
			setCursor(QCursor(*pencilCursorIcon, 6, 15));
			break;
		case RubberTool:
			setCursor(QCursor(*deleteIcon, 4, 15));
		break;
		//TODO: Implement this for adding tempo with line tool
		case AutomationTool:
			setCursor(QCursor(*crosshairIcon));
			m_drawLineMode = false;
		break;
		default:
			setCursor(QCursor(Qt::ArrowCursor));
			break;
	}
}/*}}}*/

//---------------------------------------------------------
//   newVal
//---------------------------------------------------------

void TempoCanvas::newVal(int x1, int x2, int y)
{
	int xx1 = oom->rasterVal1(x1);
	int xx2 = oom->rasterVal2(x2);

	if (xx1 > xx2)
	{
		int tmp = xx2;
		xx2 = xx1;
		xx1 = tmp;
	}
	int raster = oom->raster();
	if (raster == 1 || raster == 0) // set reasonable raster
		raster = config.division / 4;
	deleteVal1(xx1, xx2);
	int wh = (height() - 1);
	if(y < 0)
		y = 0;
	if(y > wh)
		y = wh;
	int yp = computeTempo(y, wh);
	//yp = (60000000000.0 / yp);
	//audio->msgAddTempo(xx1, int(60000000000.0 / (280000 - y)), false);
	//qDebug("TempoCanvas::newVal: y: %d, tempo: %d", y, yp);
	if(xx1 == xx2)
	{
		audio->msgAddTempo(xx1, yp, false);
	}
	else
	{
		for (int x = xx1; x < xx2; x += raster)
		{
			audio->msgAddTempo(x, yp, false);
		}
	}
	redraw();
}


void TempoCanvas::newValRamp(int x1, int y1, int x2, int y2)/*{{{*/
{
	int xx1 = oom->rasterVal1(x1);
	int xx2 = oom->rasterVal2(x2);

	if (xx1 > xx2)
	{
		int tmp = xx2;
		xx2 = xx1;
		xx1 = tmp;
	}
	int raster = oom->raster();
	if (raster == 1 || raster == 0) // set reasonable raster
		raster = config.division / 4;

	song->startUndo();

	// delete existing events
	deleteVal1(xx1, xx2);

	//Reset them after delete
	//xx1 = oom->rasterVal1(x1);
	//xx2 = oom->rasterVal2(x2);

	int wh = (height() - 1);
	// insert new events
	//qDebug("TempoCanvas::newValRamp: x1: %d, y1: %d, x2: %d, y2: %d, raster: %d", x1, y1, x2, y2, raster);
	for (int x = xx1; x < xx2; x += raster)
	{
		int y = (x2 == x1) ? y1 : (((y2 - y1)*(x - x1)) / (x2 - x1)) + y1;
		//qDebug("TempoCanvas::newValRamp: Adding event at y: %d", y);
		if(y < 0)
			y = 0;
		if(y > wh)
			y = wh;
		int yp = computeTempo(y, wh);
		//yp = (60000000000.0 / yp);
		audio->msgAddTempo(x, yp, false);
	}

	song->update(0);
	redraw();
}/*}}}*/
