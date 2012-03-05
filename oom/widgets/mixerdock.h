//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_MIXERDOCK_H_
#define _OOM_MIXERDOCK_H_

#include <QFrame>
#include "track.h"

class QAction;
class QString;
class QMenu;
class QScrollArea;
class QHBoxLayout;
class QVBoxLayout;
class QResizeEvent;
class QToolButton;
class QPushButton;
class QPixmap;
class QImage;
class AudioPortConfig;
class Strip;
class AudioStrip;
class Track;

typedef std::list<Strip*> StripList;
enum MixerMode
{
	DOCKED, //docked as part of the main application
	MASTER, //contains the master strip in the mixer
	PANE  //Just a row in the main mixer <dumb mode>
};

class MixerDock : public QFrame
{
	Q_OBJECT

    StripList stripList;
    QScrollArea* view;
    QFrame* central;
    QHBoxLayout* m_mixerBox;
	QVBoxLayout* m_adminBox;
    QHBoxLayout* m_dockButtonBox;
    QHBoxLayout* layout;
    QHBoxLayout* m_masterBox;
	Strip* masterStrip;
    AudioPortConfig* routingDialog;
	QToolButton* m_btnAux;
	QToolButton* m_btnVUColor;
	QToolButton* m_btnFadeClipping;
	TrackList* m_tracklist;
	MixerMode m_mode;
	QAction* m_auxAction; 
	QAction* m_vuColorAction; 
	QAction* m_fadeClippingAction; 
	bool loading;

	int oldAuxsSize;
	void layoutUi();
	
public:
	MixerDock(QWidget *parent = 0);
	MixerDock(MixerMode, QWidget *parent = 0 );
	~MixerDock();
	void setTracklist(TrackList* list);
	TrackList* tracklist();
	
public slots:
	void songChanged(int);
    void configChanged();
	void toggleAuxRack(bool);
	void generateVUColorMenu();
	void updateConnections(bool);
	void composerViewChanged();
	void scrollSelectedToView(qint64);
    void clear();
	
protected:
    void addStrip(Track*, int);
    void showAudioPortConfig(bool);

    enum UpdateAction
    {
        NO_UPDATE, UPDATE_ALL, UPDATE_MIDI, STRIP_INSERTED, STRIP_REMOVED
    };
    void updateMixer(UpdateAction);
};

#endif
