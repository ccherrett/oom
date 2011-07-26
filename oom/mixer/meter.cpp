//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: meter.cpp,v 1.4.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#include <stdio.h>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include "meter.h"
#include "gconfig.h"
#include "fastlog.h"

//---------------------------------------------------------
//   Meter
//---------------------------------------------------------

Meter::Meter(QWidget* parent, MeterType type, Qt::Orientation layout)
: QFrame(parent) //Qt::WNoAutoErase
{
	setBackgroundRole(QPalette::NoRole);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_StaticContents);
	// This is absolutely required for speed! Otherwise painfully slow because we get
	//  full rect paint events even on small scrolls! See help on QPainter::scroll().
	setAttribute(Qt::WA_OpaquePaintEvent);

	mtype = type;
	m_layout = layout;
	overflow = false;
	val = 0.0;
	maxVal = 0.0;
	minScale = mtype == DBMeter ? config.minMeter : 0.0; // min value in dB or int
	maxScale = mtype == DBMeter ? 10.0 : 127.0;
	yellowScale = -10;
	redScale = 0;
	setLineWidth(0);
	setMidLineWidth(0);
	green = QColor(49, 175, 197);
	yellow = QColor(156, 85, 115);
	red = QColor(197, 49, 87);
	bgColor = QColor(0, 12, 16);
}

//---------------------------------------------------------
//   setVal
//---------------------------------------------------------

void Meter::setVal(double v, double max, bool ovl)
{
	overflow = ovl;
	bool ud = false;

	if (mtype == DBMeter)
	{
		double minScaleLin = pow(10.0, minScale / 20.0);
		if ((v >= minScaleLin && val != v) || val >= minScaleLin)
		{
			val = v;
			ud = true;
		}
	}
	else
	{
		if (val != v)
		{
			val = v;
			ud = true;
		}
	}

	if (maxVal != max)
	{
		maxVal = max;
		ud = true;
	}

	if (ud)
		update();
}
//---------------------------------------------------------
//   resetPeaks
//    reset peak and overflow indicator
//---------------------------------------------------------

void Meter::resetPeaks()
{
	maxVal = val;
	overflow = val > 0.0;
	update();
}

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Meter::setRange(double min, double max)
{
	minScale = min;
	maxScale = max;
	update();
}

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void Meter::paintEvent(QPaintEvent* /*ev*/)
{
	// TODO: Could make better use of event rectangle, for speed.

	QPainter p(this);

	double range = maxScale - minScale;

	int fw = frameWidth();
	int w = width() - 2 * fw;
	int h = height() - 2 * fw;
	int yv;

	if(m_layout == Qt::Vertical)
	{
		if (mtype == DBMeter)/*{{{*/
			yv = val == 0 ? h : int(((maxScale - (fast_log10(val) * 20.0)) * h) / range);
		else
			yv = val == 0 ? h : int(((maxScale - val) * h) / range);

		if (yv > h) yv = h;

		// Draw the red, green, and yellow sections.
		drawVU(p, w, h, yv);

		// Draw the peak white line.
		int ymax;
		if (mtype == DBMeter)
			ymax = maxVal == 0 ? 0 : int(((maxScale - (fast_log10(maxVal) * 20.0)) * h) / range);
		else
			ymax = maxVal == 0 ? 0 : int(((maxScale - maxVal) * h) / range);

		int y1 = int((maxScale - redScale) * h / range);
		int y2 = int((maxScale - yellowScale) * h / range);
		int y3 = int((maxScale - yellowScale) * h / range);
		int y4 = int((maxScale - -15) * h / range);
		int y5 = int((maxScale - -20) * h / range);
		int y6 = int((maxScale - -25) * h / range);
		int y7 = int((maxScale - -30) * h / range);
		int y8 = int((maxScale - -35) * h / range);
		int y9 = int((maxScale - -40) * h / range);
		int y10 = int((maxScale - -45) * h / range);
		int y11 = int((maxScale - -50) * h / range);
		int y12 = int((maxScale - -55) * h / range);
		int y13 = int((maxScale - -5) * h / range);
		int y14 = int((maxScale - 5) * h / range);
		QPen myPen = QPen(green, 5, Qt::SolidLine, Qt::RoundCap);
		if (ymax == 0)
		{
			myPen.setColor(bgColor);
		}
		else if (ymax <= y1)
		{
			myPen.setColor(red);
		}
		else if (ymax <= y2 && ymax > y1)
		{
			myPen.setColor(yellow);
		}
		p.setPen(myPen); //floating vu levels
		p.drawLine(5, ymax, w - 6, ymax);

		myPen.setWidth(1);
		myPen.setColor(QColor(63, 74, 80));
		p.setPen(myPen); //0 db
		p.drawLine(3, y1, w - 4, y1);
		
		p.setPen(myPen); //-10 db
		p.drawLine(3, y2, w - 4, y2);
		p.drawLine(3, y2, w - 4, y2);
		p.drawLine(6, y3, w - 8, y3);
		p.drawLine(6, y4, w - 8, y4);
		p.drawLine(6, y5, w - 8, y5);
		p.drawLine(6, y6, w - 8, y6);
		p.drawLine(6, y7, w - 8, y7);
		p.drawLine(6, y8, w - 8, y8);
		p.drawLine(6, y9, w - 8, y9);
		p.drawLine(6, y10, w - 8, y10);
		p.drawLine(6, y11, w - 8, y11);
		p.drawLine(6, y12, w - 8, y12);
		p.drawLine(6, y13, w - 8, y13);
		p.drawLine(6, y14, w - 8, y14);/*}}}*/
	}
	else
	{
		if (mtype == DBMeter)/*{{{*/
			yv = val == 0 ? 0 : int(((maxScale - (fast_log10(val) * 20.0)) * w) / range);
		else
			yv = val == 0 ? 0 : int(((maxScale - val) * w) / range);

		if (yv > w) yv = w;

		// Draw the red, green, and yellow sections.
		drawVU(p, w, h, yv);

		// Draw the peak white line.
		int ymax;
		if (mtype == DBMeter)
			ymax = maxVal == 0 ? 0 : int(((maxScale - (fast_log10(maxVal) * 20.0)) * w) / range);
		else
			ymax = maxVal == 0 ? 0 : int(((maxScale - maxVal) * w) / range);

		int y1 = int((maxScale - redScale) * w / range);
		int y2 = int((maxScale - yellowScale) * w / range);
		int y3 = int((maxScale - yellowScale) * w / range);
		int y4 = int((maxScale - -15) * w / range);
		int y5 = int((maxScale - -20) * w / range);
		int y6 = int((maxScale - -25) * w / range);
		int y7 = int((maxScale - -30) * w / range);
		int y8 = int((maxScale - -35) * w / range);
		int y9 = int((maxScale - -40) * w / range);
		int y10 = int((maxScale - -45) * w / range);
		int y11 = int((maxScale - -50) * w / range);
		int y12 = int((maxScale - -55) * w / range);
		int y13 = int((maxScale - -5) * w / range);
		int y14 = int((maxScale - 5) * w / range);
		QPen myPen = QPen(green, 5, Qt::SolidLine, Qt::RoundCap);
		if (ymax == 0)
		{
			myPen.setColor(bgColor);
		}
		else if (ymax <= y1)
		{
			myPen.setColor(red);
		}
		else if (ymax <= y2 && ymax > y1)
		{
			myPen.setColor(yellow);
		}
		p.setPen(myPen); //floating vu levels
		p.drawLine(5, ymax, h - 6, ymax);

		myPen.setWidth(1);
		myPen.setColor(QColor(63, 74, 80));
		p.setPen(myPen); //0 db
		p.drawLine(3, y1, h - 4, y1);
		
		p.setPen(myPen); //-10 db
		p.drawLine(3, y2, h - 4, y2);
		p.drawLine(3, y2, h - 4, y2);
		p.drawLine(6, y3, h - 8, y3);
		p.drawLine(6, y4, h - 8, y4);
		p.drawLine(6, y5, h - 8, y5);
		p.drawLine(6, y6, h - 8, y6);
		p.drawLine(6, y7, h - 8, y7);
		p.drawLine(6, y8, h - 8, y8);
		p.drawLine(6, y9, h - 8, y9);
		p.drawLine(6, y10, h - 8, y10);
		p.drawLine(6, y11, h - 8, y11);
		p.drawLine(6, y12, h - 8, y12);
		p.drawLine(6, y13, h - 8, y13);
		p.drawLine(6, y14, h - 8, y14);/*}}}*/
	}
}

//---------------------------------------------------------
//   drawVU
//---------------------------------------------------------

void Meter::drawVU(QPainter& p, int w, int h, int yv)
{
	QPen myPen = QPen();
	myPen.setStyle(Qt::DashLine);
	if(m_layout == Qt::Vertical)
	{
		QPixmap *pixmap = new QPixmap(":/images/vugrad.png");
		QPixmap scaledPixmap = pixmap->scaled(1, height(), Qt::IgnoreAspectRatio);/*{{{*/
		myPen.setBrush(scaledPixmap);
		myPen.setWidth(1);
		p.setPen(myPen);

		p.fillRect(0, 0, w, h, QBrush(bgColor)); // dark red
		p.drawLine(4, 0, 4, h);
		p.drawLine(5, 0, 5, h);
		p.drawLine(6, 0, 6, h);
		p.drawLine(7, 0, 7, h);
		p.drawLine(8, 0, 8, h);
		p.drawLine(9, 0, 9, h);
		p.drawLine(10, 0, 10, h);
		p.fillRect(0, 0, w, yv, QBrush(bgColor)); // dark red
		if (yv == 0)
		{
			emit meterClipped();
		}/*}}}*/
	}
	else
	{
		QPixmap *pixmap = new QPixmap(":/images/vugrad_h.png");
		QPixmap scaledPixmap = pixmap->scaled(w, 1, Qt::IgnoreAspectRatio);/*{{{*/
		myPen.setBrush(scaledPixmap);
		myPen.setWidth(1);
		p.setPen(myPen);

		p.fillRect(0, 0, w, h, QBrush(bgColor)); // dark red

		//p.drawLine(4, 0, 4, w);
		//p.drawLine(5, 0, 5, w);
		//p.drawLine(6, 0, 6, w);
		//p.drawLine(7, 0, 7, w);
		//p.drawLine(8, 0, 8, w);
		//p.drawLine(9, 0, 9, w);
		//p.drawLine(10, 0, 10, w);
		
		p.drawLine(0, 1, w, 1);
		p.drawLine(0, 2, w, 2);
		p.drawLine(0, 3, w, 3);
		//p.drawLine(7, 0, 7, w);
		//p.drawLine(8, 0, 8, w);
		//p.drawLine(9, 0, 9, w);
		//p.drawLine(10, 0, 10, w);
		
		int start = w - yv;
		int startVU = yv;
		if(yv == 0)
		{
			startVU = w;
			start = 0;
		}
		printf("w = %d, yv = %d, start = %d, h = %d\n", w, yv, start, h);
		//p.fillRect(0, yv, start, h, QBrush(bgColor)); // dark red
		p.fillRect(start, 0, startVU, h, QBrush(bgColor)); // dark red
		if (yv == 0)
		{
			emit meterClipped();
		}/*}}}*/
	}
}

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void Meter::resizeEvent(QResizeEvent* /*ev*/)
{
	update();
}

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void Meter::mousePressEvent(QMouseEvent*)
{
	emit mousePress();
}

