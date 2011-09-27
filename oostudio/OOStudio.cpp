//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOStudio.h"
#include <QtGui>

OOStudio::OOStudio()
{
	setupUi(this);
	createTrayIcon();

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	m_groupCreate->setVisible(false);
	connect(m_btnCreateSession, SIGNAL(clicked()), this, SLOT(toggleCreate()));
	connect(m_btnCancelCreate, SIGNAL(clicked()), this, SLOT(cancelCreate()));

	trayIcon->show();
	setWindowTitle(tr("OOMIDI: Studio"));
	//resize(400, 300);
}

void OOStudio::toggleCreate()
{
	m_groupSession->setVisible(false);
	m_groupCreate->setVisible(true);
}

void OOStudio::cancelCreate()
{
	m_groupSession->setVisible(true);
	m_groupCreate->setVisible(false);
}

void OOStudio::setVisible(bool visible)
{
	minimizeAction->setEnabled(visible);
	maximizeAction->setEnabled(!isMaximized());
	restoreAction->setEnabled(isMaximized() || !visible);
	QDialog::setVisible(visible);
}

void OOStudio::createDialog()
{
	//Create the screen1 for sessions
	
}

void OOStudio::createTrayIcon()/*{{{*/
{
	minimizeAction = new QAction(tr("Mi&nimize"), this);
	connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
	maximizeAction = new QAction(tr("Ma&ximize"), this);
	connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));
	restoreAction = new QAction(tr("&Restore"), this);
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));
	quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

	trayMenu = new QMenu(this);
	trayMenu->addAction(minimizeAction);
	trayMenu->addAction(maximizeAction);
	trayMenu->addAction(restoreAction);
	trayMenu->addSeparator();
	trayMenu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayMenu);
	QIcon icon(":/images/oom_icon.png");
	trayIcon->setIcon(icon);
}/*}}}*/

void OOStudio::closeEvent(QCloseEvent* ev)/*{{{*/
{
	if(trayIcon->isVisible())
	{
		hide();
		ev->ignore();
	}
}/*}}}*/

void OOStudio::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
		case QSystemTrayIcon::Trigger:
		case QSystemTrayIcon::DoubleClick:
			setVisible(!isVisible());
	    break;
		case QSystemTrayIcon::MiddleClick:
		break;
		default:
    		;
	}
}

