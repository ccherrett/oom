//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _ROUTEEDITOR_
#define _ROUTEEDITOR_

#include  "ui_routeeditorbase.h"

class Track;

class RouteEditor : public QDialog, public Ui::RouteEditorBase {
	Q_OBJECT
	QAbstractButton *btnOk;
	QAbstractButton *btnCancel;
	Track* _selected;

	public:
	RouteEditor(QWidget* parent = 0);
	~RouteEditor();
	
	private slots:
		void populateInternalPorts();
		void populateSystemPorts();
		void populateJackPorts();
		void btnOkPressed(bool);
		void btnCancelPressed(bool);

	public slots:
		void setTab(int);
		void setSelectedTrack(QString);
		void setTrack(Track*);
};

#endif
