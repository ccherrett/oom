//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOSTUDIO_H
#define OOSTUDIO_H

#include <QDialog>
#include <QSystemTrayIcon>
#include "ui_OOStudioBase.h"

class QCloseEvent;
class QAction;
class QMenu;

class OOStudio :public QDialog ,public Ui::OOStudioBase
{
	Q_OBJECT

public:
	OOStudio();
	void setVisible(bool);

private:
	QAction* maximizeAction;
	QAction* minimizeAction;
	QAction* restoreAction;
	QAction* quitAction;
	QSystemTrayIcon* trayIcon;
	QMenu* trayMenu;
	void createDialog();
	void createTrayIcon();

protected:
	void closeEvent(QCloseEvent*);

private slots:
	void iconActivated(QSystemTrayIcon::ActivationReason);
	void toggleCreate();
	void cancelCreate();
};

#endif
