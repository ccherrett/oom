//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _TVDOCK_
#define _TVDOCK_

#include "ui_tviewdockbase.h"

class TrackViewDock : public QFrame, public Ui::TViewDockBase {
    Q_OBJECT
	private slots:
		void btnUpClicked(bool);
		void btnDownClicked(bool);
		void btnDeleteClicked(bool);
	public:
		TrackViewDock(QWidget* parent = 0);
		~TrackViewDock();
};
#endif

