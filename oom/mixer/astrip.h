//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: astrip.h,v 1.8.2.6 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ASTRIP_H__
#define __ASTRIP_H__

#include <vector>
#include <QCheckBox>
#include <QHash>

#include "strip.h"
#include "route.h"

class Slider;
class Knob;
class QToolButton;
class PopupMenu;
class QButton;
class TransparentToolButton;
class AudioTrack;
class DoubleLabel;
//class EffectRack;

/*class AuxCheckBox : public QCheckBox
{
	Q_OBJECT
	
	qint64 m_index;

public:
	AuxCheckBox(QString label, qint64 index, QWidget *parent = 0)
	:QCheckBox(label, parent)
	{
		m_index = index;
		QObject::connect(this, SIGNAL(toggled(bool)), this, SLOT(checkToggled(bool)));
	}
	qint64 index()
	{
		return m_index;
	}
	void setIndex(qint64 i)
	{
		m_index = i;
	}

private slots:
	void checkToggled(bool state)
	{
		emit toggled(m_index, state);
	}

signals:
	void toggled(qint64, bool);
};*/

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

class AudioStrip : public Strip
{
    Q_OBJECT

    int channel;
    Slider* slider;
    DoubleLabel* sl;
    //EffectRack* rack;

	AudioTrack* m_track;

    Knob* pan;
    DoubleLabel* panl;
	
	//QHash<int, qint64> auxIndexList;
	//QHash<qint64, Knob*> auxKnobList;
	//QHash<qint64, DoubleLabel*> auxLabelList;
	//QHash<qint64, QLabel*> auxNameLabelList;

    double volume;
    double panVal;

    QString slDefaultStyle;

    Knob* addKnob(QString, DoubleLabel**);
    //Knob* addAuxKnob(qint64, QString, DoubleLabel**, QLabel**);

    void updateOffState();
    void updateVolume();
    void updatePan();
    void updateChannels();
	//void updateAuxNames();
protected:
	void trackChanged();

private slots:
    void stereoToggled(bool);
    void preToggled(bool);
    void offToggled(bool);
    void iRoutePressed();
    void oRoutePressed();
    void routingPopupMenuActivated(QAction*);
    //void auxChanged(double, int);
    //void auxPreToggled(qint64, bool);
    void volumeChanged(double);
    void volumePressed();
    void volumeReleased();
    void panChanged(double);
    void panPressed();
    void panReleased();
    void volLabelChanged(double);
    void panLabelChanged(double);
    //void auxLabelChanged(double, int);
    void volumeRightClicked(const QPoint &);
    void panRightClicked(const QPoint &);
    void playbackClipped();
    void resetPeaks();

protected slots:
    virtual void heartBeat();

public slots:
    virtual void configChanged();
    virtual void songChanged(int);

public:
    AudioStrip(QWidget* parent, Track*);
    virtual ~AudioStrip();

    void toggleShowEffectsRack(bool);
};

#endif

