//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_CREATE_TRACKS_DIALOG_
#define _OOM_CREATE_TRACKS_DIALOG_

#include "ui_createtrackbase.h"

class QShowEvent;

class CreateTrackDialog : public QDialog, public Ui::CreateTrackBase {
	Q_OBJECT

	int m_insertType;
	int m_insertPosition;
	
	void populateInputs();

private slots:
	void addTrack();
	void inputSelected(int);

protected:
	virtual void showEvent(QShowEvent*);

public:
	CreateTrackDialog(int type, int pos, QWidget* parent = 0);
	~CreateTrackDialog(){}
};

#endif
