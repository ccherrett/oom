//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#ifndef __AUDIOMIXER_H__
#define __AUDIOMIXER_H__

#include <QMainWindow>
#include "config.h"
#include "node.h"
#include "globals.h"
#include "track.h"


class QHBoxLayout;
class QComboBox;
class QLabel;
class QMenu;
class QToolButton;
class QWidget;
class QShowEvent;
class QResizeEvent;
class QScrollArea;
class QSplitter;
class QPushButton;

class AudioTrack;
class Meter;
class Track;
class Slider;
class Knob;
class DoubleLabel;
class ComboBox;
class AudioPortConfig;
class Strip;
class MixerDock;
class MixerView;

typedef std::list<MixerDock*> DockList;

//---------------------------------------------------------
//   AudioMixer
//---------------------------------------------------------

class AudioMixer : public QMainWindow
{
    Q_OBJECT

    DockList m_dockList;
	QScrollArea* m_view;
	QSplitter* m_splitter;
	QComboBox* m_cmbRows;
	MixerView *m_mixerView;
	QPushButton* m_btnAux;

	TrackList* m_tracklist;
	void getRowCount(int, int, int&, int&);

protected:
    virtual void closeEvent(QCloseEvent*);
	virtual void hideEvent(QHideEvent*);
	virtual void showEvent(QShowEvent*);
	virtual void resizeEvent(QResizeEvent*);

signals:
	void closed();

private slots:
    void songChanged(int);
    void configChanged();
    void toggleAuxRack(bool);
    void updateMixer(int);
	void trackListChanged(TrackList* list);

public:
    AudioMixer(const QString&, QWidget* parent = 0);
    ~AudioMixer();
	TrackList* tracklist() { return m_tracklist; }
    void clear();
};

#endif

