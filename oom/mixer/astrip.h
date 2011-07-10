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
class EffectRack;

class AuxCheckBox : public QCheckBox
{
	Q_OBJECT
	
	int m_index;

public:
	AuxCheckBox(QString label, int index, QWidget *parent = 0)
	:QCheckBox(label, parent)
	{
		m_index = index;
		QObject::connect(this, SIGNAL(toggled(bool)), this, SLOT(checkToggled(bool)));
	}
	int index()
	{
		return m_index;
	}
	void setIndex(int i)
	{
		m_index = i;
	}

private slots:
	void checkToggled(bool state)
	{
		emit toggled(m_index, state);
	}

signals:
	void toggled(int, bool);
};

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

class AudioStrip : public Strip
{
    Q_OBJECT

    int channel;
    Slider* slider;
    DoubleLabel* sl;
    EffectRack* rack;

    Knob* pan;
    DoubleLabel* panl;

    std::vector<Knob*> auxKnob;
    std::vector<DoubleLabel*> auxLabel;

    double volume;
    double panVal;

    QString slDefaultStyle;

    Knob* addKnob(int, int, QString, DoubleLabel**);

    void updateOffState();
    void updateVolume();
    void updatePan();
    void updateChannels();
protected:
	void trackChanged();

private slots:
    void stereoToggled(bool);
    void preToggled(bool);
    void offToggled(bool);
    void iRoutePressed();
    void oRoutePressed();
    void routingPopupMenuActivated(QAction*);
    void auxChanged(double, int);
    void auxPreToggled(int, bool);
    void volumeChanged(double);
    void volumePressed();
    void volumeReleased();
    void panChanged(double);
    void panPressed();
    void panReleased();
    void volLabelChanged(double);
    void panLabelChanged(double);
    void auxLabelChanged(double, unsigned int);
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
    AudioStrip(QWidget* parent, AudioTrack*);
    ~AudioStrip();

    void toggleShowEffectsRack(bool);
};

#endif

