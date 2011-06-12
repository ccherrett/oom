//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: amixer.h,v 1.27.2.2 2009/10/18 06:13:00 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __AMIXER_H__
#define __AMIXER_H__

#include "cobject.h"
#include "synth.h"
#include "node.h"

class QHBoxLayout;
class QComboBox;
class QLabel;
class QMenu;
class QToolButton;
class QWidget;
class QShowEvent;
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

struct MixerConfig;

#define EFX_HEIGHT     16

typedef std::list<MixerDock*> DockList;

//---------------------------------------------------------
//   AudioMixerApp
//---------------------------------------------------------

class AudioMixerApp : public QMainWindow
{
    Q_OBJECT

    DockList m_dockList;
	QScrollArea* m_view;
	QSplitter* m_splitter;
	QComboBox* m_cmbRows;
	MixerView *m_mixerView;
	QPushButton* m_btnAux;
    QMenu* menuView;
    AudioPortConfig* routingDialog;
    QAction* routingId;
    int oldAuxsSize;

	TrackList* m_tracklist;
	void getRowCount(int, int, int&, int&);

protected:
    virtual void closeEvent(QCloseEvent*);
	virtual void hideEvent(QHideEvent*);
	virtual void showEvent(QShowEvent*);

    void showAudioPortConfig(bool);


signals:
    void closed();

private slots:
    void songChanged(int);
    void configChanged();
    void toggleAudioPortConfig();
    void toggleAuxRack(bool);
    void updateMixer(int);
	void trackListChanged(TrackList* list);

public:
    AudioMixerApp(const QString&, QWidget* parent = 0);
    ~AudioMixerApp();
	TrackList* tracklist() { return m_tracklist; }
    void clear();
};

#endif

