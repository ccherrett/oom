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
	m_cmbTemplate->addItem("Default", "Default");
	m_cmbLSCPMode->addItem("Default", "Default");
	m_cmbLSCPMode->addItem("ON_DEMAND", "ON_DEMAND");
	m_cmbLSCPMode->addItem("ON_DEMAND_HOLD", "ON_DEMAND_HOLD");
	m_cmbLSCPMode->addItem("PERSISTENT", "PERSISTENT");

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	connect(m_btnLoadSession, SIGNAL(clicked()), this, SLOT(loadSession()));
	connect(m_btnClearCreate, SIGNAL(clicked()), this, SLOT(resetCreate()));
	connect(m_btnBrowsePath, SIGNAL(clicked()), this, SLOT(browseLocation()));
	connect(m_btnBrowseLSCP, SIGNAL(clicked()), this, SLOT(browseLSCP()));
	connect(m_btnAddCommand, SIGNAL(clicked()), this, SLOT(addCommand()));
	connect(m_btnDeleteCommand, SIGNAL(clicked()), this, SLOT(deleteCommand()));

	trayIcon->show();
	setWindowTitle(tr("OOMIDI: Studio"));
	//resize(400, 300);
}

void OOStudio::browseLocation()
{
	browse(0);
}

void OOStudio::browseLSCP()
{
	browse(1);
}

void OOStudio::addCommand()
{
}

void OOStudio::deleteCommand()
{
}

void OOStudio::browse(int form)/*{{{*/
{
	switch(form)
	{
		case 0: //Location path
		{
			QString dirname = QFileDialog::getExistingDirectory(
				this,
				tr("Select a Directory"),
				QDir::currentPath());
			if(!dirname.isNull())
			{
				m_txtLocation->setText(dirname);
			}
		}
		break;
		case 1: //LSCP file path
		{
			QString filename = QFileDialog::getOpenFileName(
				this,
				tr("Open LSCP File"),
				QDir::currentPath(),
				tr("LSCP Files (*.lscp);;All Files (*.*)"));
			if(!filename.isNull())
			{
				m_txtLSCP->setText(filename);
			}
		}
		break;
	}
}/*}}}*/

void OOStudio::resetCreate()
{
	m_txtName->setText("");
	m_txtLocation->setText("");
	m_txtLSCP->setText("");
	m_cmbTemplate->setCurrentIndex(0);
	m_cmbLSCPMode->setCurrentIndex(0);
}

void OOStudio::runJack()
{
	if(m_chkStartJack->isChecked())
	{
		m_jackProcess = new QProcess(this);
		QString jackCmd = m_txtJackCommand->text();
		QStringList args = jackCmd.split(" ");
		jackCmd = args.takeFirst();
		if(args.isEmpty())
			m_jackProcess->start(jackCmd);
		else
			m_jackProcess->start(jackCmd, args);
		if(m_jackProcess->waitForStarted())
		{
			if(m_chkStartLS->isChecked())
			{
				m_lsProcess = new QProcess(this);
				QString lscmd = m_txtLSCommand->text();
				QStringList lsargs = lscmd.split(" ");
				if(lsargs.isEmpty())
				{
					m_lsProcess->start(lscmd);
				}
				else
				{
					lscmd = lsargs.takeFirst();
					m_lsProcess->start(lscmd, lsargs);
				}
			}
		}
	}
}

void OOStudio::runCommads()
{
}

void OOStudio::runOOM()
{
	m_oomProcess = new QProcess(this);
}

void OOStudio::runPostCommads()
{
}

void OOStudio::loadSession()
{
}

void OOStudio::createSession()
{
}

void OOStudio::setVisible(bool visible)
{
	minimizeAction->setEnabled(visible);
	maximizeAction->setEnabled(!isMaximized());
	restoreAction->setEnabled(isMaximized() || !visible);
	QDialog::setVisible(visible);
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

