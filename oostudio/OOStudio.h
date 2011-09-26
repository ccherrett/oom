//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOSTUDIO_H
#define OOSTUDIO_H

#include <QDialog>
#include <QSystemTrayIcon>

class QCloseEvent;
class QAction;
class QMenu;

class OOStudio :public QDialog
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
};

#endif
