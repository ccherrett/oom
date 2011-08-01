//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: meter.h,v 1.1.1.1.2.2 2009/05/03 04:14:00 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __METER_H__
#define __METER_H__

#include <QFrame>
#include "track.h"

class QResizeEvent;
class QMouseEvent;
class QPainter;

class Meter : public QFrame
{
    Q_OBJECT
public:
	enum MeterType
	{
	    DBMeter, LinMeter
	};

private:
    MeterType mtype;
    Track::TrackType m_track;
    bool overflow;
    double val;
    double maxVal;
    double minScale, maxScale;
    int yellowScale, redScale;
    QColor green;
    QColor red;
    QColor yellow;
    QColor bgColor;
    QColor m_trackColor;
	Qt::Orientation m_layout;
	bool m_redrawVU;
	QPixmap *m_pixmap_h;
	QPixmap *m_pixmap_w;
	int m_height;
	int m_width;
	QPixmap m_scaledPixmap_w;
	QPixmap m_scaledPixmap_h;

    void drawVU(QPainter& p, int, int, int, bool);

    void paintEvent(QPaintEvent*);
    virtual void resizeEvent(QResizeEvent*);
    virtual void mousePressEvent(QMouseEvent*);

public slots:
    void resetPeaks();
    void setVal(double, double, bool);

signals:
    void mousePress(bool shift = false);
    void meterClipped();

public:
	Meter(QWidget* parent, Track::TrackType track = Track::MIDI, MeterType type = DBMeter, Qt::Orientation = Qt::Vertical);
	virtual ~Meter(){}
    void setRange(double min, double max);
};
#endif

