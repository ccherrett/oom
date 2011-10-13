//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOStudio.h"
#include "OOProcess.h"
#include "OOClient.h"
#include "OODownload.h"
#include "config.h"
#include <QtGui>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QDomNode>
#include <QWebPage>
#include <QIcon>
#include <jack/jack.h>


static const char* LS_HOSTNAME  = "localhost";
static const int   LS_PORT      = 8888;
static const char* LS_COMMAND   = "linuxsampler";
static const char* JACK_COMMAND = "/usr/bin/jackd -R -P89 -p2048 -t5000 -M512 -dalsa -dhw:0 -r44100 -p512 -n2";
static const QString NO_PROC_TEXT(QObject::tr("No Running Session"));
static const QString DEFAULT_OOM = QString(SHAREDIR).append(QDir::separator()).append("templates").append(QDir::separator()).append("default.oom");
static const QString DEFAULT_LSCP = QString(SHAREDIR).append(QDir::separator()).append("templates").append(QDir::separator()).append("default.lscp");
static const QString TEMPLATE_DIR = QString(QDir::homePath()).append(QDir::separator()).append("oomidi_sessions");
static const char* OOM_TEMPLATE_NAME = "OOMidi_Orchestral_Template";
static const char* OOM_EXT_TEMPLATE_NAME = "OOMidi_Orchestral_Extended_Template";
static const char* MAP_STRING = "MAP MIDI_INSTRUMENT";

lscp_status_t client_callback ( lscp_client_t* /*_client*/, lscp_event_t /*event*/, const char * /*pchData*/, int /*cchData*/, void* pvData )
{
	OOStudio* lsc = (OOStudio*)pvData;
	if(lsc == NULL)
		return LSCP_FAILED;
	return LSCP_OK;
}

OOStudio::OOStudio()
{
	setupUi(this);
	m_oomProcess = 0;
	m_jackProcess = 0;
	m_lsProcess = 0;
	m_current = 0;
	m_incleanup = false;
	m_jackRunning = false;
	m_oomRunning = false;
	m_lsRunning = false;
	m_chkAfterOOM->hide();
	createTrayIcon();
	QString style(":/style.qss");
	loadStyle(style);

	m_lscpLabels = (QStringList() << "Default" << "ON_DEMAND" << "ON_DEMAND_HOLD" << "PERSISTENT");
	m_cmbLSCPMode->addItems(m_lscpLabels);
	
	m_cmbEditMode->addItem("Create");
	m_cmbEditMode->addItem("Update");

	m_sessionlabels = (QStringList() << "Name" << "Path" );
	m_templatelabels = (QStringList() << "Name" << "Path" );
	m_commandlabels = (QStringList() << "Command" );
	m_loglabels = (QStringList() << "Command" << "Level" << "Log" );

	m_txtLSPort->setValidator(new QIntValidator());
	createModels();
	createConnections();

	m_trayIcon->show();
	setWindowTitle(QString(tr("OOMidi: Studio")).append("-").append(VERSION));
	QSettings settings("OOMidi", "OOStudio");
	QSize size = settings.value("OOStudio/size", QSize(548, 526)).toSize();
	resize(size);
	m_restoreSize = saveGeometry();
	populateSessions();
	m_lblRunning->setText(QString(NO_PROC_TEXT));
	m_btnStopSession->setEnabled(false);

#ifdef OOM_INSTALL_BIN
	//qDebug() << OOM_INSTALL_BIN;
	//qDebug() << LIBDIR;
	qDebug() << "Found install path: " << OOM_INSTALL_BIN;
#endif
	m_webView->setUrl(QUrl("http://www.openoctave.org"));
}

void OOStudio::showMessage(QString msg)
{
	statusbar->showMessage(msg, 2000);
}

void OOStudio::showExternalLinks(const QUrl &url)
{
	QDesktopServices::openUrl(url);
}

void OOStudio::createTrayIcon()/*{{{*/
{
	m_quitAction = new QAction(tr("&Quit"), this);

	QIcon exitIcon;
	exitIcon.addPixmap(QPixmap(":/images/garbage_new_on.png"), QIcon::Normal, QIcon::On);
	exitIcon.addPixmap(QPixmap(":/images/garbage_new_off.png"), QIcon::Normal, QIcon::Off);
	exitIcon.addPixmap(QPixmap(":/images/garbage_new_over.png"), QIcon::Active);
	actionQuit->setIcon(exitIcon);
	m_quitAction->setIcon(exitIcon);

	QIcon importIcon;
	importIcon.addPixmap(QPixmap(":/images/plus_new_on.png"), QIcon::Normal, QIcon::On);
	importIcon.addPixmap(QPixmap(":/images/plus_new_off.png"), QIcon::Normal, QIcon::Off);
	importIcon.addPixmap(QPixmap(":/images/plus_new_over.png"), QIcon::Active);
	action_Import_Session->setIcon(importIcon);

	m_trayMenu = new QMenu(this);
	m_trayMenu->addAction(m_quitAction);

	m_trayIcon = new QSystemTrayIcon(this);
	m_trayIcon->setContextMenu(m_trayMenu);
	QIcon icon(":/images/oom_icon.png");
	m_trayIcon->setIcon(icon);
}/*}}}*/

void OOStudio::createModels()/*{{{*/
{
	m_sessionModel = new QStandardItemModel(m_sessionTable);
	m_sessionTable->setModel(m_sessionModel);
	m_sessionModel->setHorizontalHeaderLabels(m_sessionlabels);
	
	m_templateModel = new QStandardItemModel(m_templateTable);
	m_templateTable->setModel(m_templateModel);
	m_templateModel->setHorizontalHeaderLabels(m_templatelabels);
	
	m_commandModel = new QStandardItemModel(m_commandTable);
	m_commandTable->setModel(m_commandModel);
	m_commandModel->setHorizontalHeaderLabels(m_commandlabels);
	
	m_loggerModel = new QStandardItemModel(m_loggerTable);
	m_loggerTable->setModel(m_loggerModel);
	m_loggerModel->setHorizontalHeaderLabels(m_loglabels);

	m_sessionSelectModel = new QItemSelectionModel(m_sessionModel);
	m_templateSelectModel = new QItemSelectionModel(m_templateModel);
	m_sessionTable->setSelectionModel(m_sessionSelectModel);
	m_templateTable->setSelectionModel(m_templateSelectModel);

	m_cmbTemplate->addItem(tr("Select Template"));
	OOSession* session = readSession(QString(OOM_DEFAULT_TEMPLATE));
	if(session)
	{
		QString tag("T: ");
		tag.append(QString(OOM_TEMPLATE_NAME)).append(tr(" (Sonatina only)"));
		m_cmbTemplate->addItem(tag, QString(OOM_TEMPLATE_NAME));
		m_sessionMap[session->name] = session;
	}

	session = readSession(QString(OOM_EXT_DEFAULT_TEMPLATE));
	if(session)
	{
		QString tag("T: ");
		tag.append(QString(OOM_EXT_TEMPLATE_NAME));
		m_cmbTemplate->addItem(tag, QString(OOM_EXT_TEMPLATE_NAME));
		m_sessionMap[session->name] = session;
	}

	m_txtOOMPath->setText(DEFAULT_OOM);
	m_txtLSCP->setText(DEFAULT_LSCP);
	m_txtLocation->setText(TEMPLATE_DIR);

	QDir sessionDir(TEMPLATE_DIR);
	if(!sessionDir.exists(TEMPLATE_DIR))
	{
		sessionDir.mkpath(TEMPLATE_DIR);
	}

}/*}}}*/

void OOStudio::loadStyle(QString style)/*{{{*/
{
	QFile cf(style);
	if (cf.open(QIODevice::ReadOnly))
	{
		QByteArray ss = cf.readAll();
		QString sheet(QString::fromUtf8(ss.data()));
		qApp->setStyleSheet(sheet);
		cf.close();
	}
}/*}}}*/

void OOStudio::createConnections()/*{{{*/
{
	m_webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	connect(m_webView, SIGNAL(linkClicked(const QUrl&)), this, SLOT(showExternalLinks(const QUrl&)));
	connect(action_Import_Session, SIGNAL(triggered()), this, SLOT(importSession()));
	connect(actionQuit, SIGNAL(triggered()), this, SLOT(shutdown()));
	connect(m_quitAction, SIGNAL(triggered()), this, SLOT(shutdown()));

	connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	
	connect(m_btnLoadSession, SIGNAL(clicked()), this, SLOT(loadSessionClicked()));
	connect(m_btnLoadTemplate, SIGNAL(clicked()), this, SLOT(loadTemplateClicked()));
	connect(m_btnClearCreate, SIGNAL(clicked()), this, SLOT(resetCreate()));
	connect(m_btnDoCreate, SIGNAL(clicked()), this, SLOT(createSession()));
	connect(m_btnUpdate, SIGNAL(clicked()), this, SLOT(updateSession()));
	connect(m_btnBrowsePath, SIGNAL(clicked()), this, SLOT(browseLocation()));
	connect(m_btnBrowseLSCP, SIGNAL(clicked()), this, SLOT(browseLSCP()));
	connect(m_btnBrowseOOM, SIGNAL(clicked()), this, SLOT(browseOOM()));
	connect(m_btnAddCommand, SIGNAL(clicked()), this, SLOT(addCommand()));
	connect(m_btnDeleteCommand, SIGNAL(clicked()), this, SLOT(deleteCommand()));
	connect(m_btnClearLog, SIGNAL(clicked()), this, SLOT(clearLogger()));
	connect(m_btnDeleteTemplate, SIGNAL(clicked()), this, SLOT(deleteTemplate()));
	connect(m_btnDeleteSession, SIGNAL(clicked()), this, SLOT(deleteSession()));
	//connect(m_btnStopSession, SIGNAL(clicked()), this, SLOT(stopCurrentSession()));
	connect(m_btnStopSession, SIGNAL(clicked()), this, SLOT(cleanupProcessList()));
	connect(m_cmbTemplate, SIGNAL(currentIndexChanged(int)), this, SLOT(templateSelectionChanged(int)));
	connect(m_cmbTemplate, SIGNAL(activated(int)), this, SLOT(templateSelectionChanged(int)));
	connect(m_cmbEditMode, SIGNAL(currentIndexChanged(int)), this, SLOT(editModeChanged(int)));

	connect(m_sessionTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(sessionDoubleClicked(QModelIndex)));
	connect(m_templateTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(templateDoubleClicked(QModelIndex)));

	connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTopTabChanged(int)));
}/*}}}*/

void OOStudio::updateHeaders()/*{{{*/
{
	m_sessionModel->setHorizontalHeaderLabels(m_sessionlabels);
	
	m_templateModel->setHorizontalHeaderLabels(m_templatelabels);
	
	m_sessionTable->setColumnWidth(0, 200);
	m_templateTable->setColumnWidth(0, 200);

	m_commandModel->setHorizontalHeaderLabels(m_commandlabels);

	m_loggerModel->setHorizontalHeaderLabels(m_loglabels);
	m_loggerTable->setColumnWidth(1, 60);
}/*}}}*/

void OOStudio::populateSessions(bool usehash)/*{{{*/
{
	m_sessionModel->clear();
	m_templateModel->clear();
	while(m_cmbTemplate->count() > 3)
	{
		m_cmbTemplate->removeItem((m_cmbTemplate->count()-1));
	}

	int sess = 0;
	if(usehash)
	{
		printf("OOStudio::populateSession: sessions: %d\n", m_sessionMap.count());
		QMap<QString, OOSession*>::const_iterator iter = m_sessionMap.constBegin();
		while (iter != m_sessionMap.constEnd())
		{
			OOSession* session = iter.value();
			if(session && session->name != QString(OOM_TEMPLATE_NAME) && session->name != QString(OOM_EXT_TEMPLATE_NAME))
			{
				//printf("Processing session %s \n", session->name.toUtf8().constData());
				QStandardItem* name = new QStandardItem(session->name);
				QStandardItem* path = new QStandardItem(session->path);
				path->setToolTip(session->path);
				QList<QStandardItem*> rowData;
				rowData << name << path;
				if(session->istemplate)
				{
					m_templateModel->appendRow(rowData);
					m_cmbTemplate->addItem("T: "+name->text(), name->text());
				}
			}
			++iter;
		}
		iter = m_sessionMap.constBegin();
		while (iter != m_sessionMap.constEnd())
		{
			OOSession* session = iter.value();
			if(session && session->name != QString(OOM_TEMPLATE_NAME) && session->name != QString(OOM_EXT_TEMPLATE_NAME))
			{
				//printf("Processing session %s \n", session->name.toUtf8().constData());
				QStandardItem* name = new QStandardItem(session->name);
				QStandardItem* path = new QStandardItem(session->path);
				path->setToolTip(session->path);
				QList<QStandardItem*> rowData;
				rowData << name << path;
				if(!session->istemplate)
				{
					m_sessionModel->appendRow(rowData);
					m_cmbTemplate->addItem("S: "+name->text(), name->text());
				}
			}
			++iter;
		}
	}
	else
	{
		QSettings settings("OOMidi", "OOStudio");
		sess = settings.beginReadArray("Sessions");
		QString msg("OOStudio::populateSession: sessions: ");
		msg.append(QString::number(sess));
		qDebug() << msg;
		//printf("OOStudio::populateSession: sessions: %d\n", sess);
		showMessage(msg);

		for(int i = 0; i < sess; ++i)
		{
			settings.setArrayIndex(i);
			msg = QString("Processing template ");
			msg.append(settings.value("name").toString());
			qDebug() << msg;
			showMessage(msg);
			//printf("Processing session %s \n", settings.value("name").toString().toUtf8().constData());
			OOSession* session = readSession(settings.value("path").toString());
			if(session)
			{
				//printf("Found valid session\n");
				QStandardItem* name = new QStandardItem(session->name);
				QStandardItem* path = new QStandardItem(session->path);
				path->setToolTip(session->path);
				QList<QStandardItem*> rowData;
				rowData << name << path;
				if(session->istemplate)
				{
					m_templateModel->appendRow(rowData);
					m_cmbTemplate->addItem("T: "+name->text(), name->text());
					m_sessionMap[session->name] = session;
				}
			}
		}

		for(int i = 0; i < sess; ++i)
		{
			settings.setArrayIndex(i);
			msg = QString("Processing session ");
			msg.append(settings.value("name").toString());
			qDebug() << msg;
			showMessage(msg);
			OOSession* session = readSession(settings.value("path").toString());
			if(session)
			{
				//printf("Found valid session\n");
				QStandardItem* name = new QStandardItem(session->name);
				QStandardItem* path = new QStandardItem(session->path);
				path->setToolTip(session->path);
				QList<QStandardItem*> rowData;
				rowData << name << path;
				if(!session->istemplate)
				{
					m_sessionModel->appendRow(rowData);
					m_cmbTemplate->addItem("S: "+name->text(), name->text());
					m_sessionMap[session->name] = session;
				}
			}
		}
	}

	updateHeaders();
}/*}}}*/

void OOStudio::clearLogger()/*{{{*/
{
	m_loggerModel->clear();
	updateHeaders();
}/*}}}*/

void OOStudio::browseLocation()/*{{{*/
{
	browse(0);
}/*}}}*/

void OOStudio::browseLSCP()/*{{{*/
{
	browse(1);
}/*}}}*/

void OOStudio::browseOOM()/*{{{*/
{
	browse(2);
}/*}}}*/

void OOStudio::importSession()/*{{{*/
{
	browse(3);
}/*}}}*/

void OOStudio::currentTopTabChanged(int tab)/*{{{*/
{
	if(tab == 2)
	{
		m_loggerTable->resizeRowsToContents();
		m_loggerTable->scrollToBottom();
	}
}/*}}}*/

QString OOStudio::getValidName(QString name)/*{{{*/
{
	QString base("_Copy");
	int i = 1;
	QString newName(name.append(base));
	while(m_sessionMap.contains(newName))
	{
		newName = name.append(base).append(QString::number(i));
		++i;
	}
	return newName;
}/*}}}*/

void OOStudio::browse(int form)/*{{{*/
{
	switch(form)
	{
		case 0: //Location path
		{
			QString dirname = QFileDialog::getExistingDirectory(
				this,
				tr("Select a Directory"),
				TEMPLATE_DIR);
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
		case 2: //OOM file path
		{
			QString basedir(SHAREDIR);
			basedir.append(QDir::separator()).append("templates");
			QString filename = QFileDialog::getOpenFileName(
				this,
				tr("Open OOM File"),
				basedir,
				tr("OOM Song Files (*.oom);;All Files (*.*)"));
			if(!filename.isNull())
			{
				m_txtOOMPath->setText(filename);
			}
		}
		break;
		case 3: //Import session
		{
			QString filename = QFileDialog::getOpenFileName(
				this,
				tr("Import OOStudio File"),
				QDir::currentPath(),
				tr("OOStudio Session Files (*.oos);;All Files (*.*)"));
			if(!filename.isNull())
			{
				OOSession* session = readSession(filename);
				if(session)
				{
					if(m_sessionMap.contains(session->name))
					{
						session->name = getValidName(session->name);
					}
					QStandardItem* name = new QStandardItem(session->name);
					QStandardItem* path = new QStandardItem(session->path);
					path->setToolTip(session->path);
					QList<QStandardItem*> rowData;
					rowData << name << path;
					if(session->istemplate)
					{
						m_templateModel->appendRow(rowData);
						m_cmbTemplate->addItem(name->text(), path->text());
						m_sessionBox->setCurrentIndex(1);
					}
					else
					{
						m_sessionModel->appendRow(rowData);
						m_sessionBox->setCurrentIndex(0);
					}
					m_tabWidget->setCurrentIndex(0);
					updateHeaders();
					m_sessionMap[session->name] = session;
					saveSettings();
					populateSessions(true);
					//TODO: message to tell what type was imported
				}
				else
				{
					//TODO: send error messages
				}
			}
		}
		break;
	}
}/*}}}*/

void OOStudio::addCommand()/*{{{*/
{
	if(m_txtCommand->text().isEmpty())
		return;
	QStandardItem* item = new QStandardItem(m_txtCommand->text());
	item->setCheckable(true);
	m_commandModel->appendRow(item);
	updateHeaders();
	m_txtCommand->setText("");
	m_txtCommand->setFocus(Qt::OtherFocusReason);
}/*}}}*/

void OOStudio::deleteCommand()/*{{{*/
{
	for(int i = 0; i < m_commandModel->rowCount(); ++i)
	{
		QStandardItem* item = m_commandModel->item(i, 0);
		if(item->checkState() == Qt::Checked)
		{
			m_commandModel->takeRow(i);
		}
	}
}/*}}}*/

void OOStudio::resetCreate(bool fromClear)/*{{{*/
{
	//m_txtLSCP->setText("");
	//m_txtOOMPath->setText("");
	m_txtOOMPath->setText(DEFAULT_OOM);
	m_txtLSCP->setText(DEFAULT_LSCP);
	m_txtCommand->setText("");
	if(fromClear)
	{
		m_txtName->setText("");
		m_cmbTemplate->setCurrentIndex(0);
	}
	m_txtLocation->setText(TEMPLATE_DIR);
	m_txtLSHost->setText(LS_HOSTNAME);
	m_txtLSPort->setText(QString::number(LS_PORT));
	m_txtLSCommand->setText(LS_COMMAND);
	m_txtJackCommand->setText(JACK_COMMAND);
	m_cmbLSCPMode->setCurrentIndex(0);
	m_commandModel->clear();
	m_chkTemplate->setChecked(false);
	updateHeaders();
}/*}}}*/

void OOStudio::cleanupProcessList()/*{{{*/
{
	printf("OOStudio::cleanupProcessList\n");
	m_incleanup = false;
	OOClient* oomclient = 0;
	if(m_oomProcess && m_oomRunning/*checkOOM()*/)
	{
		oomclient = new OOClient("127.0.0.1", 8415, this);
		oomclient->callShutdown();
		//oomRemoteShutdown();
		//m_oomProcess->terminate();
		m_oomProcess->waitForFinished(500000);
		delete m_oomProcess;
		m_oomProcess = 0;
		m_oomRunning = false;
	}
	if(m_lsProcess && m_lsRunning/* && m_lsProcess->state() == QProcess::Running*/)
	{
		m_lsProcess->terminate();
		m_lsProcess->waitForFinished();
		delete m_lsProcess;
		m_lsProcess = 0;
		m_lsRunning = false;
	}
	QList<long> keys = m_procMap.keys();
	foreach(long key, keys)
	{
		OOProcess* p = m_procMap.take(key);
		if(p)
		{
			p->terminate();
			p->waitForFinished();
			delete p;
		}
	}
	if(m_jackProcess && m_jackRunning/* && m_jackProcess->state() == QProcess::Running*/)
	{
		m_jackProcess->kill();
		m_jackProcess->waitForFinished();
		delete m_jackProcess;
		m_jackProcess = 0;
		m_jackRunning = false;
	}
	m_lblRunning->setText(QString(NO_PROC_TEXT));
	m_btnStopSession->setEnabled(false);
	m_current = 0;
	if(oomclient)
		delete oomclient;
	m_incleanup = false;
}/*}}}*/

void OOStudio::shutdown()/*{{{*/
{
	//if(stopCurrentSession())
	//{
		cleanupProcessList();
		saveSettings();
		qApp->quit();
	//}
}/*}}}*/

void OOStudio::saveSettings()/*{{{*/
{
	QSettings settings("OOMidi", "OOStudio");
	if(!m_sessionMap.isEmpty())
	{
		QMap<QString, OOSession*>::const_iterator i = m_sessionMap.constBegin();
		int id = 0;
		settings.beginWriteArray("Sessions");
		while (i != m_sessionMap.constEnd())
		{
			OOSession* session = i.value();
			if(session && session->name != QString(OOM_TEMPLATE_NAME) && session->name != QString(OOM_EXT_TEMPLATE_NAME))
			{
				settings.setArrayIndex(id);
				
				settings.setValue("name", i.key());
				settings.setValue("path", session->path);
				
				++id;
			}
			++i;
		}
		settings.endArray();
	}
	settings.beginGroup("OOStudio");
	settings.setValue("size", size());
	settings.endGroup();
	settings.sync();
}/*}}}*/

bool OOStudio::runJack(OOSession* session)/*{{{*/
{
	if(session)
	{
		if(session->loadJack)
		{
			showMessage("Launching jackd");
			printf("Launching jackd: ");
			m_jackProcess = new QProcess(this);
			connect(m_jackProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processJackMessages()));
			connect(m_jackProcess, SIGNAL(readyReadStandardError()), this, SLOT(processJackErrors()));
			//connect(m_jackProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processJackExit(int, QProcess::ExitStatus)));
			//connect(m_jackProcess, SIGNAL(error(QProcess::ProcessError )), this, SLOT(processJackExecError(QProcess::ProcessError)));
			QString jackCmd = session->jackcommand;
			QStringList args = jackCmd.split(" ");
			jackCmd = args.takeFirst();
			if(args.isEmpty())
				m_jackProcess->start(jackCmd);
			else
				m_jackProcess->start(jackCmd, args);
			m_jackProcess->waitForStarted(50000);
			/*bool rv = m_jackProcess->waitForStarted(150000);
			rv = (m_jackProcess->state() == QProcess::Running);
			if(rv && m_jackProcess->state() == QProcess::Running)
			{
				rv = true;
			}
			else
			{
				m_jackProcess->close();
				rv = false;
			}*/
			bool rv = pingJack();
			printf("%s\n", rv ? " Complete" : " FAILED");
			showMessage(rv ? "Launching jackd Complete" : "Launching jackd FAILED");
			showMessage("Launching jackd");
			m_jackRunning = rv;
			return rv;
		}
		else
			return true;
	}
	return false;
}/*}}}*/

bool OOStudio::pingJack()/*{{{*/
{
	bool rv = true;
	jack_status_t status;
	jack_client_t *client;
	int retry = 0;
	sleep(1);
	while((client = jack_client_open("OOStudio", JackNoStartServer, &status, NULL)) == 0 && retry < 3)
	{
		rv = false;
		++retry;
		sleep(1);
	}
	
	if(client != 0)
	{
		switch(status)
		{
			case JackServerStarted:
				printf(" Server Startup");
				rv = true;
			break;
			case JackFailure:
				printf(" General Failure");
				rv = false;
			break;
			case JackServerError:
				printf(" Server Error");
				rv = false;
			case JackClientZombie:
			break;
			case JackServerFailed:
				printf(" Server Failed");
				rv = false;
			break;
			case JackNoSuchClient:
				printf(" No Such Client");
				rv = false;
			break;
			case JackInitFailure:
				printf(" Init Failure");
				rv = false;
			break;
			case JackNameNotUnique:
				printf(" Name Not Unique");
				rv = false;
			break;
			case JackInvalidOption:
				printf(" Invalid Option");
				rv = false;
			break;
			case JackLoadFailure:
				printf(" Load Failure");
				rv = false;
			break;
			case JackVersionError:
				printf(" Version Error");
				rv = false;
			break;
			case JackBackendError:
				printf(" Backend Error");
				rv = false;
			break;
			default:
				rv = true;
			break;
		}
		//printf(" %d", status);
		jack_client_close(client);
	}
	else
	{
		printf("jack server not running?");
	}
	client = NULL;
	return rv;
}/*}}}*/

bool OOStudio::runLinuxsampler(OOSession* session)/*{{{*/
{
	if(session)
	{
		if(session->loadls)
		{
			showMessage("Launching linuxsampler... ");
			printf("Launching linuxsampler ");
			m_lsProcess = new QProcess(this);
			connect(m_lsProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processLSMessages()));
			connect(m_lsProcess, SIGNAL(readyReadStandardError()), this, SLOT(processLSErrors()));
			QString lscmd = session->lscommand;
			QStringList lsargs = lscmd.split(" ");
			lscmd = lsargs.takeFirst();
			if(lsargs.isEmpty())
			{
				m_lsProcess->start(lscmd);
			}
			else
			{
				m_lsProcess->start(lscmd, lsargs);
			}
			if(!m_lsProcess->waitForStarted())
			{
				showMessage("LinuxSampler startup FAILED");
				printf("FAILED\n");
				return false;
			}
			sleep(1);
			int retry = 0;
			bool rv = pingLinuxsampler(session);
			while(!rv && retry < 5)
			{
				sleep(1);
				rv = pingLinuxsampler(session);
				++retry;
			}
			printf("%s\n", rv ? "Complete" : "FAILED");
			showMessage(rv ? "Launching linuxsampler Complete" : "Launching linuxsampler FAILED");
			m_lsRunning = rv;
			return rv;
		}
		else
			return true; //You did not want it loaded here so all is good
	}
	return false;
}/*}}}*/

bool OOStudio::pingLinuxsampler(OOSession* session)/*{{{*/
{
	if(session)
	{
		lscp_client_t* client = ::lscp_client_create(session->lshostname.toUtf8().constData(), session->lsport, client_callback, this);
		if(client == NULL)
			return false;
		//printf("Query LS info: ");
		lscp_server_info_t* info = lscp_get_server_info(client);
		if(info == NULL)
		{
			//printf("FAILED!!\n");
			return false;
		}
		else
		{
			//printf("Description: %s, Version: %s, Protocol Version: %s\n", info->description, info->version, info->protocol_version);
			return true;
		}
	}
	return true;
}/*}}}*/

bool OOStudio::loadLSCP(OOSession* session)/*{{{*/
{
	if(session)
	{
		showMessage("Loading LSCP File into linuxsampler engine");
		printf("Loading LSCP ");
		lscp_client_t* client = ::lscp_client_create(session->lshostname.toUtf8().constData(), session->lsport, client_callback, this);
		if(client == NULL)
			return false;
		QFile lsfile(session->lscpPath);
		if(!lsfile.open(QIODevice::ReadOnly))
		{
			//TODO:We need to error here
			lscp_client_destroy(client);
			printf("FAILED to open file\n");
			return false;
		}
		QTextStream in(&lsfile);
		QString command = in.readLine();
		while(!command.isNull())
		{
			if(!command.startsWith("#") && !command.isEmpty())
			{
				QString cmd = command.append("\r\n");
				if(session->lscpMode && cmd.startsWith(QString(MAP_STRING)))
				{
					QString rep(m_lscpLabels.at(session->lscpMode));
					int count = 0;
					foreach(QString str, m_lscpLabels)
					{
						if(count && str != rep)
							cmd.replace(str, rep);
						++count;
					}
				}
				::lscp_client_query(client, cmd.toUtf8().constData());
				//return true;
				/*if(::lscp_client_query(client, cmd.toUtf8().constData()) != LSCP_OK)
				{
					lsfile.close();
					lscp_client_destroy(client);
					printf("FAILED to perform query\n");
					printf("%s\n", command.toUtf8().constData());
					return false;
				}*/
			}
			command = in.readLine();
		}
		lsfile.close();
		lscp_client_destroy(client);
		printf("Complete\n");
		return true;
	}
	return false;
}/*}}}*/

void OOStudio::runCommands(OOSession* session, bool )/*{{{*/
{
	if(session)
	{
		//QStringList commands = post ? session->postCommands : session->commands;
		QStringList commands = session->commands;
		foreach(QString command, commands)
		{
			qDebug() << "Running command: " << command;
			QString cmd(command);
			QStringList args = cmd.split(" ");
			cmd = args.takeFirst();
			OOProcess* process = new OOProcess(cmd, this);
			connect(process, SIGNAL(readyReadStandardOutput(QString, long)), this, SLOT(processCustomMessages(QString, long)));
			connect(process, SIGNAL(readyReadStandardError(QString, long)), this, SLOT(processCustomErrors(QString, long)));
			if(args.isEmpty())
			{
				process->start(cmd);
			}
			else
			{
				process->start(cmd, args);
			}
			if(process->waitForStarted())
			{
				long pid = process->pid();
				m_procMap[pid] = process;
			}
		}
	}
}/*}}}*/

bool OOStudio::runOOM(OOSession* session)/*{{{*/
{
	if(session)
	{
		showMessage("Loading OOMidi...");
		printf("Loading OOMidi ");
		m_oomProcess = new QProcess(this);
		connect(m_oomProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOOMMessages()));
		connect(m_oomProcess, SIGNAL(readyReadStandardError()), this, SLOT(processOOMErrors()));
		connect(m_oomProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processOOMExit(int, QProcess::ExitStatus)));
		QStringList args;
		args << session->songfile;
		m_oomProcess->start(QString(OOM_INSTALL_BIN), args);
		bool rv = m_oomProcess->waitForStarted();
		printf("%s\n", rv ? "Complete" : "FAILED");
		showMessage(rv ? "Loading OOMidi Complete" : "Loading OOMidi FAILED");
		m_oomRunning = rv;
		return rv;
	}
	return false;
}/*}}}*/

void OOStudio::sessionDoubleClicked(QModelIndex index)/*{{{*/
{
	//printf("OOStudio::sessionDoubleClicked - row: %d\n", index.row());
	if(index.isValid())
	{
		QStandardItem* item = m_sessionModel->item(index.row());
		if(item)
		{
			OOSession* session = m_sessionMap.value(item->text());
			if(session)
			{
				loadSession(session);
			}
		}
	}
}/*}}}*/

void OOStudio::templateDoubleClicked(QModelIndex index)/*{{{*/
{
	//printf("OOStudio::templateDoubleClicked - row: %d\n", index.row());
	if(index.isValid())
	{
		QStandardItem* item = m_templateModel->item(index.row());
		if(item)
		{
			OOSession* session = m_sessionMap.value(item->text());
			if(session)
			{
				if(QMessageBox::warning(
					this,
					tr("Open Template"),
					tr("You are about to open a template session \nAre you sure you want to do this "),
					QMessageBox::Ok, QMessageBox::Cancel
				) == QMessageBox::Ok)
				{
					loadSession(session);
				}
			}
		}
	}
}/*}}}*/

void OOStudio::loadSessionClicked()/*{{{*/
{
	if(m_sessionSelectModel && m_sessionSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_sessionSelectModel->selectedRows(0);
		if(selected.size())
		{
			QModelIndex index = selected.at(0);
			QStandardItem* item = m_sessionModel->itemFromIndex(index);
			if(item)
			{
				OOSession* session = m_sessionMap.value(item->text());
				if(session)
				{
					loadSession(session);
				}
			}
		}
	}
}/*}}}*/

void OOStudio::loadTemplateClicked()/*{{{*/
{
	if(m_templateSelectModel && m_templateSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_templateSelectModel->selectedRows(0);
		if(selected.size())
		{
			QModelIndex index = selected.at(0);
			QStandardItem* item = m_templateModel->itemFromIndex(index);
			if(item)
			{
				OOSession* session = m_sessionMap.value(item->text());
				if(session)
				{
					if(QMessageBox::warning(
						this,
						tr("Open Template"),
						tr("You are about to open a template session \nAre you sure you want to do this "),
						QMessageBox::Ok, QMessageBox::Cancel
					) == QMessageBox::Ok)
					{
						loadSession(session);
					}
				}
			}
		}
	}
}/*}}}*/

bool OOStudio::checkOOM()/*{{{*/
{
	return (m_oomProcess && m_oomProcess->state() == QProcess::Running);
}/*}}}*/

QString OOStudio::convertPath(QString path)/*{{{*/
{
	if(!path.startsWith("~"))
		return path;
	
	if(path.startsWith("~/"))
	{
		return path.replace(0, 1, QDir::homePath());
	}
	return path;
}/*}}}*/

void OOStudio::doSessionDelete(OOSession* session)/*{{{*/
{
	if(session)
	{
		QFileInfo fi(session->path);
		QDir dir = fi.dir();
		if(QFile::remove(session->path) &&
			QFile::remove(session->lscpPath) &&
			QFile::remove(session->songfile))
		{
			dir.rmdir(dir.absolutePath());
		}
	}
}/*}}}*/

void OOStudio::deleteTemplate()/*{{{*/
{
	if(m_templateSelectModel && m_templateSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_templateSelectModel->selectedRows(0);
		if(selected.size())
		{
			QModelIndex index = selected.at(0);
			QStandardItem* item = m_templateModel->itemFromIndex(index);
			if(item)
			{
				QString msg(tr("WARNING: You are about to delete template \n%1 \nNOTE: This will remove all the files on disk for this session\nAre you sure you want to do this ?"));
				if(QMessageBox::question(
					this,
					tr("Delete Template"),
					msg.arg(item->text()),
					QMessageBox::Ok, QMessageBox::Cancel
				) == QMessageBox::Ok)
				{
					OOSession* session = m_sessionMap.take(item->text());
					if(session)
					{
						if(m_current && session->name == m_current->name)
						{
							cleanupProcessList();
						}
						doSessionDelete(session);
						saveSettings();
						populateSessions(true);
					}
				}
			}
		}
	}
}/*}}}*/

void OOStudio::deleteSession()/*{{{*/
{
	if(m_sessionSelectModel && m_sessionSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_sessionSelectModel->selectedRows(0);
		if(selected.size())
		{
			QModelIndex index = selected.at(0);
			QStandardItem* item = m_sessionModel->itemFromIndex(index);
			if(item)
			{
				QString msg(tr("You are about to delete session \n%1 \nThis will remove all the files on disk for this session\nAre you sure you want to do this ?"));
				if(QMessageBox::question(
					this,
					tr("Delete Session"),
					msg.arg(item->text()),
					QMessageBox::Ok, QMessageBox::Cancel
				) == QMessageBox::Ok)
				{
					OOSession* session = m_sessionMap.take(item->text());
					if(session)
					{
						if(m_current && session->name == m_current->name)
						{
							cleanupProcessList();
						}
						doSessionDelete(session);
						saveSettings();
						populateSessions(true);
					}
				}
			}
		}
	}
}/*}}}*/

bool OOStudio::stopCurrentSession()/*{{{*/
{
	if(m_current)
	{
		if(checkOOM())
		{
			if(QMessageBox::question(
				this,
				tr("Stop Session"),
				tr("There appears to be a running session of oom. \nStopping this session will cause any changes to the current song to be lost\n(Recommended)  Save your current song in oomidi then click Ok.\nAre you sure you want to do this ?"),
				QMessageBox::Ok, QMessageBox::Cancel
			) == QMessageBox::Ok)
			{
				cleanupProcessList();
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}/*}}}*/

void OOStudio::loadSession(OOSession* session)/*{{{*/
{
	if(session)
	{
		showMessage("OOStudio::loadSession :"+session->name);
		printf("OOStudio::loadSession : name: %s\n", session->name.toUtf8().constData());
		printf("OOStudio::loadSession : path: %s\n", session->path.toUtf8().constData());
		/*if(checkOOM())
		{
			if(QMessageBox::warning(
				this,
				tr("Running Session"),
				tr("There appears to be a running session of oom. \nLoading a new session will cause any changes to the current song to be lost\n(Recommended)  Save your current song in oomidi then click Ok.\nAre you sure you want to do this ?"),
				QMessageBox::Ok, QMessageBox::Cancel
			) != QMessageBox::Ok)
			{
				return;
			}
		}*/
		cleanupProcessList();
		if(runJack(session))
		{
			int retry = 0;
			bool usels = session->loadls;
			bool lsrunning = runLinuxsampler(session);
			while(!lsrunning && retry < 5)
			{
				if(m_lsProcess)
				{
					m_lsProcess->kill();
					m_lsProcess = 0;
				}
				lsrunning = runLinuxsampler(session);
				++retry;
			}
			if(lsrunning)
			{
				retry = 0;
				bool lscpLoaded = true;
				if(usels)
				{
					lscpLoaded = loadLSCP(session);
					while(!lscpLoaded && retry < 5)
					{
						sleep(1);
						lscpLoaded = loadLSCP(session);
						++retry;
					}
				}
				if(lscpLoaded)
				{
					runCommands(session);
					if(runOOM(session))
					{
						//runCommands(session, true);
						m_current = session;
						m_lblRunning->setText(session->name);
						m_btnStopSession->setEnabled(true);
						m_tabWidget->setCurrentIndex(2);
						//hide();
					}
					else
					{
						cleanupProcessList();
						QString msg(tr("Failed to run OOMidi"));
						msg.append("\nCommand:\n").append(OOM_INSTALL_BIN).append(" ").append(session->songfile);
						QMessageBox::critical(this, tr("Create Failed"), msg);
					}
				}
				else
				{
					cleanupProcessList();
					QString msg(tr("Failed to load LSCP file into linuxsample"));
					msg.append("\nFile:\n").append(session->lscpPath);
					QMessageBox::critical(this, tr("Create Failed"), msg);
				}
			}
			else
			{
				m_current = 0;
				cleanupProcessList();
				QString msg(tr("Failed to start to linuxsample server"));
				msg.append("\nCommand:\n").append(session->lscommand);
				QMessageBox::critical(this, tr("Create Failed"), msg);
			}
		}
		else
		{
			QString msg(tr("Failed to start to jackd server"));
			msg.append("\nwith Command:\n").append(session->jackcommand);
			QMessageBox::critical(this, tr("Jackd Failed"), msg);
			if(m_jackProcess)
			{
				delete m_jackProcess;
				m_jackProcess = 0;
			}
		}
	}
}/*}}}*/

bool OOStudio::validateCreate()/*{{{*/
{
	bool rv = true;
	QString error(tr("You must provide the %1 for your new session\n"
			"Please enter the %1 and try again"));
	QString chkerror(tr("%1 field is checked which requires %2 \n"
			"Please enter a %2 and try again"));
	if(m_txtName->text().isEmpty())
	{
		QMessageBox::warning(
			this,
			tr("Missing Field"),
			error.arg("Name")
			);
		m_txtName->setFocus(Qt::OtherFocusReason);
		rv = false;
	}
	else if (m_txtLocation->text().isEmpty())
	{
		QMessageBox::warning(
			this,
			tr("Missing Field"),
			error.arg("Location")
			);
		m_txtLocation->setFocus(Qt::OtherFocusReason);
		rv = false;
	}
	else if (m_txtOOMPath->text().isEmpty())
	{
		QMessageBox::warning(
			this,
			tr("Missing Field"),
			error.arg("OOM Path")
			);
		m_txtOOMPath->setFocus(Qt::OtherFocusReason);
		rv = false;
	}
	else if((m_chkStartJack->isChecked() && m_txtJackCommand->text().isEmpty()))
	{
		QMessageBox::warning(
			this,
			tr("Missing Field"),
			chkerror.arg("Start Jack").arg("Jack command")
			);
		m_txtJackCommand->setFocus(Qt::OtherFocusReason);
		rv = false;
	}
	else
	{
		if(m_sessionMap.contains(m_txtName->text()) && m_cmbEditMode->currentIndex() == 0)
		{
			QMessageBox::warning(
				this,
				tr("Dubplate Name"),
				tr("There is already a session with the name you selected\nPlease change the name and try again")
				);
			m_txtName->setText(getValidName(m_txtName->text()));
			m_txtName->setFocus(Qt::OtherFocusReason);
			rv = false;
		}
	}
	if(m_chkStartLS->isChecked() && rv)
	{
		if (m_txtLSHost->text().isEmpty())
		{
			QMessageBox::warning(
				this,
				tr("Missing Field"),
				chkerror.arg("Start linuxsampler").arg("Linuxsampler host")
				);
			m_txtLSHost->setFocus(Qt::OtherFocusReason);
			rv = false;
		}
		else if(m_txtLSPort->text().isEmpty())
		{
			QMessageBox::warning(
				this,
				tr("Missing Field"),
				chkerror.arg("Start Linuxsampler").arg("Linuxsampler port")
				);
			m_txtLSPort->setFocus(Qt::OtherFocusReason);
			rv = false;
		}
		else if(m_txtLSCommand->text().isEmpty())
		{
			QMessageBox::warning(
				this,
				tr("Missing Field"),
				chkerror.arg("Start Linuxsampler").arg("Linuxsampler command")
				);
			m_txtLSCommand->setFocus(Qt::OtherFocusReason);
			rv = false;
		}
		else if(m_txtLSCP->text().isEmpty())
		{
			QMessageBox::warning(
				this,
				tr("Missing Field"),
				chkerror.arg("Start Linuxsampler").arg("Linuxsampler command")
				);
			m_txtLSCP->setFocus(Qt::OtherFocusReason);
			rv = false;
		}
	}
	return rv;
}/*}}}*/

void OOStudio::createSession()/*{{{*/
{
	if(validateCreate())
	{
		bool istemplate = m_chkTemplate->isChecked();
		QDomDocument doc("OOStudioSession");
		QDomElement root = doc.createElement("OOStudioSession");
		doc.appendChild(root);
		QString basepath = convertPath(m_txtLocation->text());
		QString basename = m_txtName->text();
		basename = basename.simplified().replace(" ", "_");
		OOSession* newSession = new OOSession;
		newSession->name = basename;
		newSession->istemplate = istemplate;
		root.setAttribute("istemplate", istemplate);
		root.setAttribute("name", basename);

		QDir basedir(basepath);
		if(basedir.exists() && basedir.mkdir(basename) && basedir.cd(basename))
		{
			QString newPath(basedir.absolutePath());
			newPath.append(QDir::separator()).append(basename);
			QString filename(newPath+".oos");
			QString songfile(newPath+".oom");
			QString lscpfile(newPath+".lscp");

			QString strSong = convertPath(m_txtOOMPath->text());
			bool oomok = QFile::copy(strSong, songfile);
			bool lscpok = QFile::copy(convertPath(m_txtLSCP->text()), lscpfile);
			if(oomok && lscpok)
			{
				QDomElement esong = doc.createElement("songfile");
				root.appendChild(esong);
				esong.setAttribute("path", songfile);
				newSession->songfile = songfile;

				QDomElement lscp = doc.createElement("lscpfile");
				root.appendChild(lscp);
				lscp.setAttribute("path", lscpfile);
				lscp.setAttribute("mode", m_cmbLSCPMode->currentIndex());
				newSession->lscpPath = lscpfile;
				newSession->lscpMode = m_cmbLSCPMode->currentIndex();

				QDomElement jack = doc.createElement("jackd");
				root.appendChild(jack);
				jack.setAttribute("path", m_txtJackCommand->text());
				jack.setAttribute("checked", m_chkStartJack->isChecked());
				newSession->loadJack = m_chkStartJack->isChecked();
				newSession->jackcommand = m_txtJackCommand->text();

				QDomElement lsnode = doc.createElement("linuxsampler");
				root.appendChild(lsnode);
				lsnode.setAttribute("path", m_txtLSCommand->text());
				lsnode.setAttribute("checked", m_chkStartLS->isChecked());
				lsnode.setAttribute("hostname", m_txtLSHost->text());
				lsnode.setAttribute("port", m_txtLSPort->text());
				newSession->loadls = m_chkStartLS->isChecked();
				newSession->lscommand = m_txtLSCommand->text();
				newSession->lshostname = m_txtLSHost->text();
				newSession->lsport = m_txtLSPort->text().toInt();

				for(int i= 0; i < m_commandModel->rowCount(); ++i)
				{
					QStandardItem* item = m_commandModel->item(i);
					QDomElement cmd = doc.createElement("command");
					root.appendChild(cmd);
					cmd.setAttribute("path", item->text());
					newSession->commands << item->text();
				}
				//QString xml = doc.toString();
				QFile xmlfile(filename);
				if(xmlfile.open(QIODevice::WriteOnly) )
				{
					QTextStream ts( &xmlfile );
					ts << doc.toString();

					xmlfile.close();
					//TODO: Write out the session to settings and update table
					newSession->path = filename;
					m_sessionMap[basename] = newSession;
					QStandardItem* nitem = new QStandardItem(basename);
					QStandardItem* npath = new QStandardItem(filename);
					npath->setToolTip(filename);
					QList<QStandardItem*> rowData;
					rowData << nitem << npath;
					showMessage("Successfully Created Session: "+basename);
					if(istemplate)
					{
						m_templateModel->appendRow(rowData);
					}
					else
					{
						//Not a template so load it now and minimize;
						m_sessionModel->appendRow(rowData);
						QString msg = QObject::tr("Successfully Created Session:\n\t%1 \nWould you like to open this session now?");
						if(QMessageBox::question(
							this,
							tr("Open Session"),
							msg.arg(basename),
							QMessageBox::Ok, QMessageBox::No
						) == QMessageBox::Ok)
						{
							loadSession(newSession);
						}
					}
					saveSettings();
					populateSessions(true);
					resetCreate();
				}
				else
				{
					QString msg(tr("Failed to open file for writing"));
					if(oomok)
					{
						basedir.remove(songfile);
					}
						
					if(lscpok)
					{
						basedir.remove(lscpfile);
					}
					msg.append("\n").append(filename);
					basedir.rmdir(newPath);
					QMessageBox::critical(this, tr("Create Failed"), msg);
				}
			}
			else
			{//Failed run cleanup
				QString msg;
				if(oomok)
				{
					basedir.remove(songfile);
					msg = tr("Failed to copy LSCP file");
				}
					
				if(lscpok)
				{
					basedir.remove(lscpfile);
					msg = tr("Failed to copy OOMidi song file");
				}
				basedir.rmdir(newPath);
				QMessageBox::critical(this, tr("Create Failed"), msg);
			}
		}
		else
		{
			QMessageBox::critical(
				this,
				tr("Create Directory Failed"),
				tr("There was a problem creating your session directory\n"
				"Please make sure there are not directories or files under\n"
				"the location with the same name as your session name."),
				QMessageBox::Ok
			);
		}
	}
}/*}}}*/

void OOStudio::updateSession()/*{{{*/
{
	if(validateCreate())
	{
		bool istemplate = m_chkTemplate->isChecked();
		QDomDocument doc("OOStudioSession");
		QDomElement root = doc.createElement("OOStudioSession");
		doc.appendChild(root);
		
		QString basepath = convertPath(m_txtLocation->text());
		QString basename = m_txtName->text();
		basename = basename.simplified().replace(" ", "_");
		
		OOSession* newSession = new OOSession;
		OOSession* oldSession = m_sessionMap.value(basename);
		
		newSession->name = oldSession->name;
		newSession->istemplate = istemplate;
		
		root.setAttribute("istemplate", istemplate);
		root.setAttribute("name", basename);

		QString destPath(basepath);
		destPath.append(QDir::separator()).append(basename);
		qDebug() << "destPath: " << destPath << "basepath: " << basepath;
		QDir basedir(basepath);
		if(basedir.exists())
		{
			QString newPath(basedir.absolutePath());
			newPath.append(QDir::separator()).append(basename);
			QString filename(newPath+".oos");
			QString songfile(newPath+".oom");
			QString lscpfile(newPath+".lscp");

			QString strSong = convertPath(m_txtOOMPath->text());
			QString strLSCP = convertPath(m_txtLSCP->text());
			bool oomok = false;
			if(oldSession->songfile == songfile)
			{
				oomok = true;
			}
			else
			{
				if(QFile::exists(songfile))
				{
					if(QFile::exists(strSong))
					{
						QFile::remove(songfile);
					}
				}
				oomok = QFile::copy(strSong, songfile);
			}
			bool lscpok = false;
			if(oldSession->lscpPath == strLSCP)
			{
				lscpok = true;
			}
			else
			{
				if(QFile::exists(lscpfile))
				{
					if(QFile::exists(strLSCP))
					{
						QFile::remove(lscpfile);
					}
				}
				lscpok = QFile::copy(convertPath(strLSCP), lscpfile);
			}
			if(oomok && lscpok)
			{
				QDomElement esong = doc.createElement("songfile");
				root.appendChild(esong);
				esong.setAttribute("path", songfile);
				newSession->songfile = songfile;

				QDomElement lscp = doc.createElement("lscpfile");
				root.appendChild(lscp);
				lscp.setAttribute("path", lscpfile);
				lscp.setAttribute("mode", m_cmbLSCPMode->currentIndex());
				newSession->lscpPath = lscpfile;
				newSession->lscpMode = m_cmbLSCPMode->currentIndex();

				QDomElement jack = doc.createElement("jackd");
				root.appendChild(jack);
				jack.setAttribute("path", m_txtJackCommand->text());
				jack.setAttribute("checked", m_chkStartJack->isChecked());
				newSession->loadJack = m_chkStartJack->isChecked();
				newSession->jackcommand = m_txtJackCommand->text();

				QDomElement lsnode = doc.createElement("linuxsampler");
				root.appendChild(lsnode);
				lsnode.setAttribute("path", m_txtLSCommand->text());
				lsnode.setAttribute("checked", m_chkStartLS->isChecked());
				lsnode.setAttribute("hostname", m_txtLSHost->text());
				lsnode.setAttribute("port", m_txtLSPort->text());
				newSession->loadls = m_chkStartLS->isChecked();
				newSession->lscommand = m_txtLSCommand->text();
				newSession->lshostname = m_txtLSHost->text();
				newSession->lsport = m_txtLSPort->text().toInt();

				for(int i= 0; i < m_commandModel->rowCount(); ++i)
				{
					QStandardItem* item = m_commandModel->item(i);
					QDomElement cmd = doc.createElement("command");
					root.appendChild(cmd);
					cmd.setAttribute("path", item->text());
					newSession->commands << item->text();
				}
				//QString xml = doc.toString();
				//TODO: Do we need to first delete the file here?
				if(QFile::exists(filename))
				{
					QFile::remove(filename);
				}
				QFile xmlfile(filename);
				if(xmlfile.open(QIODevice::WriteOnly) )
				{
					QTextStream ts( &xmlfile );
					ts << doc.toString();

					xmlfile.close();
					//TODO: Write out the session to settings and update table
					newSession->path = filename;
					m_sessionMap[basename] = newSession;
					saveSettings();
					populateSessions(true);
					resetCreate();
					QString msg = QObject::tr("Successfully Updated Session: \n\t%1");
					QMessageBox::information(
						this, 
						tr("Success"),
						msg.arg(basename)
						);
				}
				else
				{
					QString msg(tr("Failed to open file for writing"));
				/*	if(oomok)
					{
						basedir.remove(songfile);
					}
						
					if(lscpok)
					{
						basedir.remove(lscpfile);
					}*/
					msg.append("\n").append(filename);
					//basedir.rmdir(newPath);
					QMessageBox::critical(this, tr("Create Failed"), msg);
				}
			}
			else
			{//Failed run cleanup
				QString msg;
				if(oomok)
				{
					basedir.remove(songfile);
					msg = tr("Failed to copy LSCP file");
				}
					
				if(lscpok)
				{
					basedir.remove(lscpfile);
					msg = tr("Failed to copy OOMidi song file");
				}
				basedir.rmdir(newPath);
				QMessageBox::critical(this, tr("Create Failed"), msg);
			}
		}
	}
	else
	{
		//TODO: Add error message here
		QMessageBox::information(
			this,
			tr("Invalid Form"),
			tr("All required information has not been provided, please correct the problem and click \"Create New\" again?"),
			QMessageBox::Ok);
	}
}/*}}}*/

void OOStudio::templateSelectionChanged(int index)/*{{{*/
{
	QString tpath = m_cmbTemplate->itemData(index).toString();
	//printf("OOStudio::templateSelectionChanged index: %d, name: %s sessions:%d\n", index, tpath.toUtf8().constData(), m_sessionMap.size());
	if(!index || (index <= 2 && m_cmbEditMode->currentIndex() == 1))
	{
		resetCreate();
		return;
	}
	
	//Copy the values from the other template into this
	if(!m_sessionMap.isEmpty() && m_sessionMap.contains(tpath))
	{
		OOSession* session = m_sessionMap.value(tpath);
		if(session)
		{
			if(m_cmbEditMode->currentIndex() == 1)
			{
				QFileInfo fi(session->path);
				m_txtLocation->setText(fi.dir().absolutePath());
				m_txtName->setText(session->name);
				m_chkTemplate->setChecked(session->istemplate);
			}
			else
			{
				if(m_txtName->text().isEmpty())
				{
					m_txtName->setText(getValidName(session->name));
				}
			}
			m_commandModel->clear();
			for(int i = 0; i < session->commands.size(); ++i)
			{
				QStandardItem* item = new QStandardItem(session->commands.at(i));
				item->setCheckable(true);
				m_commandModel->appendRow(item);
			}
			for(int i = 0; i < session->postCommands.size(); ++i)
			{
				QStandardItem* item = new QStandardItem(session->postCommands.at(i));
				item->setCheckable(true);
				m_commandModel->appendRow(item);
			}
			m_chkStartJack->setChecked(session->loadJack);
			m_txtJackCommand->setText(session->jackcommand);
			m_chkStartLS->setChecked(session->loadls);
			m_txtLSCommand->setText(session->lscommand);
			m_txtLSHost->setText(session->lshostname);
			m_txtLSPort->setText(QString::number(session->lsport));
			m_cmbLSCPMode->setCurrentIndex(session->lscpMode);
			m_txtLSCP->setText(session->lscpPath);
			m_txtOOMPath->setText(session->songfile);
			m_txtName->setFocus(Qt::OtherFocusReason);
			updateHeaders();
		}
	}
}/*}}}*/

void OOStudio::editModeChanged(int index)/*{{{*/
{
	resetCreate();
	if(index)
	{
		m_txtName->setEnabled(false);
		m_txtLocation->setEnabled(false);
		m_btnBrowsePath->setEnabled(false);
		m_btnDoCreate->setEnabled(false);
		m_btnUpdate->setEnabled(true);
	}
	else
	{
		m_txtName->setEnabled(true);
		m_txtLocation->setEnabled(true);
		m_btnBrowsePath->setEnabled(true);
		m_btnDoCreate->setEnabled(true);
		m_btnUpdate->setEnabled(false);
	}
}/*}}}*/

OOSession* OOStudio::readSession(QString filename)/*{{{*/
{
	OOSession* session = 0;
	QDomDocument temp("OOStudioSession");
	QFile tempFile(filename);
	if(!tempFile.open(QIODevice::ReadOnly))
	{
		//TODO:We need to error here
		return session;
	}
	if(!temp.setContent(&tempFile))
	{
		//TODO:We need to error here
		return session;
	}
	//We now have a loaded file so we get the data from it.
	QDomElement root = temp.documentElement();
	if(!root.isNull())
	{
		session = new OOSession;
		session->name = root.attribute("name");
		session->istemplate = root.attribute("istemplate").toInt();
		session->path = filename;
		QDomNodeList flist = root.elementsByTagName("command");
		for(int i = 0; i < flist.count(); ++i)
		{
			QDomElement command = flist.at(i).toElement();
			bool post = command.attribute("post").toInt();
			if(post)
				session->postCommands.append(command.attribute("path"));
			else
				session->commands.append(command.attribute("path"));
		}
		QDomNodeList ljack = root.elementsByTagName("jackd");
		for(int i = 0; i < ljack.count(); ++i)
		{
			QDomElement jack = ljack.at(i).toElement();
			session->loadJack = jack.attribute("checked").toInt();
			session->jackcommand = jack.attribute("path");
		}
		QDomNodeList lls = root.elementsByTagName("linuxsampler");
		for(int i = 0; i < lls.count(); ++i)
		{
			QDomElement ls = lls.at(i).toElement();
			session->loadls = ls.attribute("checked").toInt();
			session->lscommand = ls.attribute("path");
			session->lshostname = ls.attribute("hostname", "localhost");
			session->lsport = ls.attribute("port", "8888").toInt();
		}

		QDomNodeList llscp = root.elementsByTagName("lscpfile");
		for(int i = 0; i < llscp.count(); ++i)
		{
			QDomElement ls = llscp.at(i).toElement();
			session->lscpPath = ls.attribute("path");
			session->lscpMode = ls.attribute("mode").toInt();
		}
		QDomNodeList lsong = root.elementsByTagName("songfile");
		for(int i = 0; i < lsong.count(); ++i)
		{
			QDomElement song = lsong.at(i).toElement();
			session->songfile = song.attribute("path");
		}
		return session;
	}
	return 0;
}/*}}}*/

void OOStudio::closeEvent(QCloseEvent* ev)/*{{{*/
{
	if(m_trayIcon->isVisible())
	{
		hide();
		ev->ignore();
	}
}/*}}}*/

void OOStudio::resizeEvent(QResizeEvent* evt)
{
	QMainWindow::resizeEvent(evt);
	m_loggerTable->resizeRowsToContents();
}

void OOStudio::iconActivated(QSystemTrayIcon::ActivationReason reason)/*{{{*/
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
}/*}}}*/

void OOStudio::processOOMExit(int code, QProcess::ExitStatus status)/*{{{*/
{
	if(!m_incleanup)
	{
		switch(status)
		{
			case QProcess::NormalExit:
			{
				cleanupProcessList();
				showNormal();
			}
			break;
			case QProcess::CrashExit:
			{
				cleanupProcessList();
				showNormal();
			}
			break;
		}
	}
	qDebug() << "OOMidi exited with error code: " << code;
}/*}}}*/

void OOStudio::processJackExit(int code, QProcess::ExitStatus status)/*{{{*/
{
	if(m_current)
	{
		switch(status)
		{
			case QProcess::NormalExit:
			{
				printf("Jack exited normally\n");
				//m_restoreAction->trigger();
				//cleanupProcessList();
			}
			break;
			case QProcess::CrashExit:
			{
				printf("Jack exited with crash\n");
				//cleanupProcessList();
				//m_restoreAction->trigger();
			}
			break;
		}
	}
	qDebug() << "Jack exited with error code: " << code;
}/*}}}*/

void OOStudio::processJackExecError(QProcess::ProcessError error)
{
	printf("Jack process error recieved: %d\n", error);
}


void OOStudio::processJackMessages()/*{{{*/
{
	
	QString messages = QString::fromUtf8(m_jackProcess->readAllStandardOutput().constData());
	if(messages.isEmpty())
		return;
	QList<QStandardItem*> rowData;
	QStandardItem* command = new QStandardItem("Jackd");
	QStandardItem* log = new QStandardItem(messages);
	QStandardItem* level = new QStandardItem("INFO");
	rowData << command << level << log;
	m_loggerModel->appendRow(rowData);
	updateHeaders();
	m_loggerTable->resizeRowsToContents();
	m_loggerTable->scrollToBottom();
	
	//processMessages(0, tr("Jackd"), m_jackProcess);
}/*}}}*/

void OOStudio::processJackErrors()/*{{{*/
{
	QString messages = QString::fromUtf8(m_jackProcess->readAllStandardError().constData());
	if(messages.isEmpty())
		return;
	QList<QStandardItem*> rowData;
	QStandardItem* command = new QStandardItem("Jackd");
	command->setBackground(QColor(99, 36, 36));
	QStandardItem* log = new QStandardItem(messages);
	QStandardItem* level = new QStandardItem("ERROR");
	rowData << command << level << log;
	m_loggerModel->appendRow(rowData);
	updateHeaders();
	m_loggerTable->resizeRowsToContents();
	m_loggerTable->scrollToBottom();

	//processMessages(1, tr("Jackd"), m_jackProcess);
}/*}}}*/

void OOStudio::processLSMessages()/*{{{*/
{
	QString messages = QString::fromUtf8(m_lsProcess->readAllStandardOutput().constData());
	if(messages.isEmpty())
		return;
	QList<QStandardItem*> rowData;
	QStandardItem* command = new QStandardItem("LinuxSampler");
	QStandardItem* log = new QStandardItem(messages);
	QStandardItem* level = new QStandardItem("INFO");
	rowData << command << level << log;
	m_loggerModel->appendRow(rowData);
	updateHeaders();
	m_loggerTable->resizeRowsToContents();
	m_loggerTable->scrollToBottom();
	
	//processMessages(0, tr("LinuxSampler"), m_lsProcess);
}/*}}}*/

void OOStudio::processLSErrors()/*{{{*/
{
	QString messages = QString::fromUtf8(m_lsProcess->readAllStandardError().constData());
	if(messages.isEmpty())
		return;
	QList<QStandardItem*> rowData;
	QStandardItem* command = new QStandardItem("LinuxSampler");
	command->setBackground(QColor(99, 36, 36));
	QStandardItem* log = new QStandardItem(messages);
	QStandardItem* level = new QStandardItem("ERROR");
	rowData << command << level << log;
	m_loggerModel->appendRow(rowData);
	updateHeaders();
	m_loggerTable->resizeRowsToContents();
	m_loggerTable->scrollToBottom();
	//processMessages(1, tr("LinuxSampler"), m_lsProcess);
}/*}}}*/

void OOStudio::processOOMMessages()/*{{{*/
{
	QString messages = QString::fromUtf8(m_oomProcess->readAllStandardOutput().constData());
	if(messages.isEmpty())
		return;
	QList<QStandardItem*> rowData;
	QStandardItem* command = new QStandardItem("OOMidi");
	QStandardItem* log = new QStandardItem(messages);
	QStandardItem* level = new QStandardItem("INFO");
	rowData << command << level << log;
	m_loggerModel->appendRow(rowData);
	updateHeaders();
	m_loggerTable->resizeRowsToContents();
	m_loggerTable->scrollToBottom();
	
	//processMessages(0, tr("OOMidi"), m_oomProcess);
}/*}}}*/

void OOStudio::processOOMErrors()/*{{{*/
{
	QString messages = QString::fromUtf8(m_oomProcess->readAllStandardError().constData());
	if(messages.isEmpty())
		return;
	QList<QStandardItem*> rowData;
	QStandardItem* command = new QStandardItem("OOMidi");
	command->setBackground(QColor(99, 36, 36));
	QStandardItem* log = new QStandardItem(messages);
	QStandardItem* level = new QStandardItem("ERROR");
	rowData << command << level << log;
	m_loggerModel->appendRow(rowData);
	updateHeaders();
	m_loggerTable->resizeRowsToContents();
	m_loggerTable->scrollToBottom();
	//processMessages(1, tr("OOMidi"), m_oomProcess);
}/*}}}*/

void OOStudio::processCustomMessages(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		processMessages(0, name, m_procMap.value(pid));
	}
}/*}}}*/

void OOStudio::processCustomErrors(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		processMessages(1, name, m_procMap.value(pid));
	}
}/*}}}*/

void OOStudio::processMessages(int type, QString name, QProcess* p)/*{{{*/
{
	if(p)
	{
		switch(type)
		{
			case 0: //Output
			{/*{{{*/
				QString messages = QString::fromUtf8(p->readAllStandardOutput().constData());
				if(messages.isEmpty())
					return;
				QList<QStandardItem*> rowData;
				QStandardItem* command = new QStandardItem(name);
				QStandardItem* log = new QStandardItem(messages);
				QStandardItem* level = new QStandardItem("INFO");
				rowData << command << level << log;
				m_loggerModel->appendRow(rowData);
				updateHeaders();
				m_loggerTable->resizeRowsToContents();
				m_loggerTable->scrollToBottom();
			}/*}}}*/
			break;
			case 1: //Error
			{/*{{{*/
				QString messages = QString::fromUtf8(p->readAllStandardError().constData());
				if(messages.isEmpty())
					return;
				QList<QStandardItem*> rowData;
				QStandardItem* command = new QStandardItem(name);
				command->setBackground(QColor(99, 36, 36));
				QStandardItem* log = new QStandardItem(messages);
				QStandardItem* level = new QStandardItem("ERROR");
				rowData << command << level << log;
				m_loggerModel->appendRow(rowData);
				updateHeaders();
				m_loggerTable->resizeRowsToContents();
				m_loggerTable->scrollToBottom();
			}/*}}}*/
			break;
		}
	}
}/*}}}*/
