//=========================================================
//  OOStudio
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2012 Andrew Williams & Christopher Cherrett
//=========================================================

#ifndef __TEMPO_HEADER_H__
#define __TEMPO_HEADER_H__

#include <QFrame>

class TempoCanvas;

//---------------------------------------------------------
//   TempoHeader
//---------------------------------------------------------

class TempoHeader : public QFrame
{
    Q_OBJECT

    TempoCanvas* m_canvas;
	double m_tempoStart;
	double m_tempoEnd;

public slots:
    void songChanged(int);
	void setXMag(float);
	void setXPos(int);
	void toolChanged(int);
	void setStartTempo(double);
	void setEndTempo(double);
    void setOrigin(int x, int y);
    void setPos(int idx, unsigned x, bool adjustScrollbar);

signals:
    void updateXMag(float);
	void updateXPos(int);
	void followEvent(int);
	void tempoChanged(int);
	void timeChanged(unsigned);
	void startTempoChanged(double);
	void endTempoChanged(double);

public:
    TempoHeader(QWidget *parent, int xmag);
    ~TempoHeader();
};

#endif

