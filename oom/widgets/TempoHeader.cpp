//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2012 Andrew Williams & Christopher Cherrett
//=========================================================

#include <QtGui>

#include "TempoHeader.h"
#include "TempoCanvas.h"
#include "song.h"
#include "gconfig.h"
#include "globals.h"

TempoHeader::TempoHeader(QWidget* parent, int xmag)
: QFrame(parent)
{
	m_tempoStart = 80.0;
	m_tempoEnd = 180.0;

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0,0,0,0);

	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	setFixedHeight(60);

	m_canvas = new TempoCanvas(this, xmag);

	int offset = -(config.division / 4);
	m_canvas->setOrigin(offset, 0);
	layout->addWidget(m_canvas, 10);

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

	connect(m_canvas, SIGNAL(followEvent(int)), SIGNAL(followEvent(int)));
	connect(m_canvas, SIGNAL(timeChanged(unsigned)), SIGNAL(timeChanged(unsigned)));
	connect(m_canvas, SIGNAL(tempoChanged(int)), SIGNAL(tempoChanged(int)));
}

TempoHeader::~TempoHeader()
{
}

void TempoHeader::songChanged(int type)/*{{{*/
{
	Q_UNUSED(type);
}/*}}}*/

void TempoHeader::setXMag(float mag)
{
	if(m_canvas)
		m_canvas->setXMag(mag);
}

void TempoHeader::setXPos(int pos)
{
	if(m_canvas)
		m_canvas->setXPos(pos);
}

void TempoHeader::toolChanged(int t)
{
	if(m_canvas)
		m_canvas->setTool(t);
}

void TempoHeader::setStartTempo(double tempo)
{
	if(tempo < m_tempoEnd)
	{
		if(m_canvas)
			m_canvas->setStartTempo(int(60000000.0 / tempo));
		m_tempoStart = tempo;
	}
}

void TempoHeader::setEndTempo(double tempo)
{
	if(tempo > m_tempoStart)
	{
		m_tempoEnd = tempo;
		if(m_canvas)
			m_canvas->setEndTempo(int(60000000.0 / tempo));
	}
}

void TempoHeader::setOrigin(int x, int y)
{
	if(m_canvas)
		m_canvas->setOrigin(x, y);
}

void TempoHeader::setPos(int idx, unsigned x, bool sb)
{
	if(m_canvas)
		m_canvas->setPos(idx, x, sb);
}

