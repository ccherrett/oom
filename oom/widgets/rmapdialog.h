//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _ROUTEMAP_DIALOG_
#define _ROUTEMAP_DIALOG_

#include "ui_rmapdialogbase.h"

class RouteMapDialog : public QDialog, public Ui::RouteMapDialogBase
{
	Q_OBJECT

	public:
	RouteMapDialog(bool ro = false, QWidget* parent = 0);
	void setFileName(QString);
	void setNotes(QString);

	signals:
	void saveRouteMap(QString, QString);

	private slots:
	void btnSaveClicked(bool);
	void btnCancelClicked(bool);
	void txtNameChanged(QString);
};

#endif
