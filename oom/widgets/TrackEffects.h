//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================
#ifndef _OOM_TRACK_EFFECTS_
#define _OOM_TRACK_EFFECTS_

#include <QFrame>
#include <QTabWidget>
#include <QCheckBox>
#include <QHash>

class Slider;
class Knob;
class QToolButton;
class PopupMenu;
class QButton;
class QLabel;
class TransparentToolButton;
class AudioTrack;
class DoubleLabel;
class EffectRack;
class Track;
class AudioTrack;
class QVBoxLayout;
class QScrollArea;

class AuxPreCheckBox : public QCheckBox
{
	Q_OBJECT
	
	qint64 m_index;

public:
	AuxPreCheckBox(QString label, qint64 index, QWidget *parent = 0)
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
};

class AuxProxy : public QObject
{
	Q_OBJECT

	AudioTrack* m_track;

public slots:
    void auxChanged(double, int);
    void auxPreToggled(qint64, bool);
    void auxLabelChanged(double, int);
	void updateAuxNames();
	void songChanged(int);

public:
	AuxProxy(AudioTrack* t)
	:m_track(t)
	{
	}
	AudioTrack* track(){return m_track;}
	QHash<int, qint64> auxIndexList;
	QHash<qint64, Knob*> auxKnobList;
	QHash<qint64, DoubleLabel*> auxLabelList;
	QHash<qint64, QLabel*> auxNameLabelList;

};

class TrackEffects : public QFrame
{
	Q_OBJECT

	qint64 m_trackId;
	Track* m_track;
	QTabWidget* m_tabWidget;
    QScrollArea *m_auxScroll;
	QWidget *m_auxTab;
	QWidget *m_fxTab;
    QFrame *m_auxBase;

	EffectRack* m_inputRack;
    EffectRack* m_rack;

    QVBoxLayout *m_mainVBoxLayout;
    QVBoxLayout *m_auxBox;
	QVBoxLayout *m_auxTabLayout;
    QVBoxLayout *rackBox;

	bool hasAux;
	QHash<qint64, AuxProxy*> m_proxy;
    Knob* addAuxKnob(AuxProxy*, qint64, QString, DoubleLabel**, QLabel**);
	void layoutUi();
	void populateAuxForTrack(AudioTrack* t);

signals:

public slots:
    void songChanged(int);

private slots:
	void tabChanged(int);
	void setupAuxPanel();

public:
	TrackEffects(Track* track, QWidget* parent = 0);
	void setTrack(Track* t);
	Track* getTrack()
	{
		return m_track;
	}

};

#endif
