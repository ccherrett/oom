//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "rmapdialog.h"

RouteMapDialog::RouteMapDialog(bool readonly, QWidget* parent) : QDialog(parent)
{
	setupUi(this);
	txtName->setReadOnly(readonly);
	connect(txtName, SIGNAL(textEdited(QString)), SLOT(txtNameChanged(QString)));
	connect(btnSave, SIGNAL(clicked(bool)), SLOT(btnSaveClicked(bool)));
	connect(btnCancel, SIGNAL(clicked(bool)), SLOT(btnCancelClicked(bool)));
}

void RouteMapDialog::btnSaveClicked(bool)
{
	if(txtName->text().isEmpty())
		return;
	emit saveRouteMap(txtName->text(), txtNotes->toPlainText());
	close();
}

void RouteMapDialog::btnCancelClicked(bool)
{
	close();
}

void RouteMapDialog::txtNameChanged(QString val)
{
	btnSave->setEnabled(!val.isEmpty());
}

void RouteMapDialog::setFileName(QString _name)
{
	txtName->setText(_name);
}

void RouteMapDialog::setNotes(QString _note)
{
	txtNotes->document()->setPlainText(_note);
}
