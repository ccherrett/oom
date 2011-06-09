//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_MIXERDOCK_H_
#define _OOM_MIXERDOCK_H_

#include <QtGui>
class QString;
class QWidget;
class QDockWidget;
class QScrollArea;
class QHBoxLayout;
class AudioPortConfig;
class Strip;
class Track;

typedef std::list<Strip*> StripList;

class MixerDock : public QDockWidget
{
	Q_OBJECT

    StripList stripList;
    QScrollArea* view;
    QWidget* central;
    QHBoxLayout* lbox;
    QHBoxLayout* layout;
    QMenu* menuView;
    AudioPortConfig* routingDialog;
	int oldAuxsSize;
	
public:
	MixerDock(const QString& title, QWidget *parent = 0);
	~MixerDock();
    void clear();
	
private slots:
	void songChanged(int);
    void configChanged();
	
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
