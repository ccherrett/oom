//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _ROUTEEDITOR_
#define _ROUTEEDITOR_

#include  "ui_routeeditorbase.h"

class RouteEditor : public QDialog, public Ui::RouteEditorBase {
	Q_OBJECT

	public:
	RouteEditor(QWidget* parent = 0);
	~RouteEditor();

};

#endif
