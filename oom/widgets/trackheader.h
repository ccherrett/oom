//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_TRACKHEADER_H_
#define _OOM_TRACKHEADER_H_
#include "ui_trackheaderbase.h"

class Track;
class QAction;
class QSize;

class TrackHeader : public QFrame, public Ui::TrackHeaderBase
{
	Q_OBJECT

	Track* m_track;

private slots:
	void songChanged(int);
	void generateAutomationMenu();
    void changeAutomation(QAction*);
	void toggleRecord(bool);
	void toggleMute(bool);
	void toggleSolo(bool);
	void toggleOffState(bool);
	void updateTrackName(QString);
	void generatePopupMenu();

public slots:

protected:
	virtual QSize sizeHint();

public:
	TrackHeader(Track* track, QWidget* parent = 0);
	virtual ~TrackHeader(){}
};

#endif
