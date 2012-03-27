//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: sigscale.cpp,v 1.6 2004/04/11 13:03:32 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#include <values.h>

#include <QMouseEvent>
#include <QPainter>

#include "al/sig.h"  // Tim.
#include "globals.h"
#include "AbstractMidiEditor.h"
#include "sigscale.h"
#include "song.h"
#include "icons.h"
#include "gconfig.h"

//---------------------------------------------------------
//   SigScale
//---------------------------------------------------------

SigScale::SigScale(int* r, QWidget* parent, int xs)
: View(parent, xs, 1)
{
	setToolTip(tr("Signature scale"));
	raster = r;
	m_startCursorMove = false;
	m_movingCursor = false;
	pos[0] = song->cpos();
	pos[1] = song->lpos();
	pos[2] = song->rpos();
	pos[3] = MAXINT;
	button = Qt::NoButton;
	setMouseTracking(true);
	connect(song, SIGNAL(posChanged(int, unsigned, bool)), this, SLOT(setPos(int, unsigned, bool)));
	setFixedHeight(18);
	setBg(QColor(30,30,30));
}

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void SigScale::setPos(int idx, unsigned val, bool)
{
	if (val == pos[idx])
	{
		redraw();
		return;
	}
	unsigned opos = mapx(pos[idx]);
	pos[idx] = val;
	if (!isVisible())
		return;
	val = mapx(val);
	int x = -9;
	int w = 18;
	if (opos > val)
	{
		w += opos - val;
		x += val;
	}
	else
	{
		w += val - opos;
		x += opos;
	}
	redraw();
	//redraw(QRect(x, 0, w, height()));
}

void SigScale::viewMousePressEvent(QMouseEvent* event)/*{{{*/
{
	button = event->button();
	//viewMouseMoveEvent(event);
	int x = AL::sigmap.raster(event->x(), *raster);
	emit timeChanged(x);
	pos[3] = x;
	int i;
	switch (button)
	{
		case Qt::LeftButton:
			i = 0;
			pos[0] = x;
			break;
		case Qt::MidButton:
			i = 1;
			pos[1] = x;
			break;
		case Qt::RightButton:
			i = 2;
			pos[2] = x;
			break;
		default:
			pos[3] = x;
			redraw();
			return;
	}
	Pos p(x, true);
	song->setPos(i, p);
	redraw();
}/*}}}*/

void SigScale::viewMouseMoveEvent(QMouseEvent* event)
{
	int x = AL::sigmap.raster(event->x(), *raster);/*{{{*/
	emit timeChanged(x);
	pos[3] = x;
	int i;
	switch (button)
	{
		case Qt::LeftButton:
			i = 0;
			pos[0] = x;
			break;
		case Qt::MidButton:
			i = 1;
			pos[1] = x;
			break;
		case Qt::RightButton:
			i = 2;
			pos[2] = x;
			break;
		default:
			pos[3] = x;
			redraw();
			return;
	}
	Pos p(x, true);
	song->setPos(i, p);
	redraw();/*}}}*/
}

void SigScale::viewMouseReleaseEvent(QMouseEvent*)/*{{{*/
{
	button = Qt::NoButton;
	m_startCursorMove = false;
	m_movingCursor = false;
}/*}}}*/

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void SigScale::leaveEvent(QEvent*)
{
	emit timeChanged(MAXINT);
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void SigScale::pdraw(QPainter& p, const QRect& r)
{
	int x = r.x();
	int w = r.width();
	int h = height();

	if (x < 0)
		x = 0;
	QColor colTimeLine = QColor(172,181,176);
	p.setPen(colTimeLine);
	p.setFont(QFont("fixed-width", 9, QFont::Bold));
	for (AL::ciSigEvent si = AL::sigmap.begin(); si != AL::sigmap.end(); ++si)
	{
		AL::SigEvent* e = si->second;
		int xp = mapx(e->tick);
		if (xp > x + w)
			break;
		if (xp + 40 < x)
			continue;
		p.drawLine(xp, 0, xp, h / 2);
		p.drawLine(xp, h / 2, xp + 5, h / 2);
		QString s;
		s.sprintf("%d/%d", e->sig.z, e->sig.n);
		p.drawText(xp + 8, h - 6, s);
	}

	//---------------------------------------------------
	//    draw location marker
	//---------------------------------------------------

	//Christopher here is your color
	QList<QColor> colors;
	colors << QColor(0, 186, 255) << QColor(139, 225, 69) << QColor(139, 225, 69) << QColor(156,75,219);
	if (pos[3] != MAXINT)
	{
		p.setPen(colors.at(3));
		int xp = mapx(pos[3]);
		if (xp >= x && xp < x + w)
		{
			p.drawLine(xp, 0, xp, height()-10);
			QPixmap* pm = markIcon[3];
			p.drawPixmap(xp - pm->width() / 2, 1, *pm);
		}
	}

	//colors << QColor(139, 225, 69) << QColor(139, 225, 69) << QColor(0, 186, 255);
	for (int i = 0; i < 3; ++i)
	{
		int xp = mapx(pos[i]);
		if (xp >= x && xp < x + w)
		{
			p.setPen(colors.at(i));
			p.drawLine(xp, 0, xp, 8);
			QPixmap* pm = markIcon[i];
			p.drawPixmap(xp - pm->width() / 2, 1, *pm);
		}
	}
	/*//p.setPen(Qt::red);
	p.setPen(QColor(0, 186, 255));
	int xp = mapx(pos[0]);
	if (xp >= x && xp < x + w)
		p.drawLine(xp, 0, xp, h);
	//p.setPen(Qt::blue);
	p.setPen(QColor(139, 225, 69));
	xp = mapx(pos[1]);
	if (xp >= x && xp < x + w)
		p.drawLine(xp, 0, xp, h);
	xp = mapx(pos[2]);
	if (xp >= x && xp < x + w)
		p.drawLine(xp, 0, xp, h);*/
}


