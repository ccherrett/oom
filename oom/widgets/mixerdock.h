//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_MIXERDOCK_H_
#define _OOM_MIXERDOCK_H_

#include <QDockWidget>

class QAction;
class QString;
class QWidget;
class QMenu;
class QScrollArea;
class QHBoxLayout;
class QVBoxLayout;
class QToolButton;
class QPushButton;
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
    QWidget* m_mixerBase;
    QHBoxLayout* m_mixerBox;
	QVBoxLayout* m_adminBox;
    QHBoxLayout* m_dockButtonBox;
    QHBoxLayout* layout;
    QMenu* menuView;
    AudioPortConfig* routingDialog;
	QToolButton* m_btnDock;
	QToolButton* m_btnClose;
	QPushButton* m_btnAux;
	QAction* closeAction;

	int oldAuxsSize;
	
public:
	MixerDock(const QString& title, QWidget *parent = 0);
	~MixerDock();
    void clear();
	
private slots:
	void songChanged(int);
    void configChanged();
	void toggleAuxRack(bool);
	void toggleDetach();
	void toggleClose();
	void updateConnections(bool);
	
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
