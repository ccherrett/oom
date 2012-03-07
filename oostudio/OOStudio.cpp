//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOStudio.h"
#include "OOProcess.h"
#include "OOThread.h"
#include "OOClient.h"
#include "OODownload.h"
#include "config.h"
#include <QtGui>
#include <QDateTime>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QDomNode>
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
static const QString SONATINA_PATH = QString(QDir::rootPath()).append("usr").append(QDir::separator()).append("share").append(QDir::separator()).append("sounds").append(QDir::separator()).append("sonatina").append(QDir::separator()).append("Sonatina Symphonic Orchestra");
static const QString SONATINA_LOCAL_PATH("Sonatina Symphonic Orchestra");
static const QString MAESTRO_PATH("maestro");
static const QString CLASSIC_PATH("ClassicFREE");
static const QString ACCOUSTIC_PATH("AcousticGuitarFREE");
static const QString M7IR44_PATH("Samplicity M7 Main - 03 - Wave Quad files, 32 bit, 44.1 Khz, v1.1");
static const QString M7IR48_PATH("Samplicity M7 Main - 04 - Wave Quad files, 32 bit, 48 Khz, v1.1");
static const QString SOUNDS_DIR = QString(QDir::homePath()).append(QDir::separator()).append(".sounds").append(QDir::separator());

#define SessionNameRole Qt::UserRole+5

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
	//m_scrollAreaWidgetContents->setStyleSheet("QWidget#m_scrollAreaWidgetContents{ background-color: #525252; }");
	m_lsThread = 0;
	m_customThread = 0;
	m_oomThread = 0;
	m_jackThread = 0;
	
	m_downloader = new OODownload(this);
	m_current = 0;
	m_incleanup = false;
	m_loading = false;
	m_chkAfterOOM->hide();
	createTrayIcon();
	m_sonatinaBox->setObjectName("m_sonatinaBox");
	QString style(":/style.qss");
	loadStyle(style);

	QDir homeDir = QDir::home();
	homeDir.mkdir(".sounds");

	m_lscpLabels = (QStringList() << "Default" << "ON_DEMAND" << "ON_DEMAND_HOLD" << "PERSISTENT");
	m_cmbLSCPMode->addItems(m_lscpLabels);
	
	m_cmbEditMode->addItem("Create");
	m_cmbEditMode->addItem("Update");

	m_sessionlabels = (QStringList() << tr("Sessions") );
	m_templatelabels = (QStringList() << tr("Templates"));
	m_commandlabels = (QStringList() << "Command" );
	m_loglabels = (QStringList() << "Command" << "Level" << "Log" );

	m_txtLSPort->setValidator(new QIntValidator());
	createModels();
	createConnections();

	m_trayIcon->show();
	setWindowTitle(QString(tr("OOStudio")).append("-").append(VERSION));
	QSettings settings("OOMidi", "OOStudio");
	QSize size = settings.value("OOStudio/size", QSize(548, 526)).toSize();
	resize(size);
	m_restoreSize = saveGeometry();

	//TODO: Make this translateable
	QFile sestemp(":/html/session.tmpl");
	if(sestemp.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream in(&sestemp);
		m_sessionTemplate = in.readAll();
		sestemp.close();
	}
	else
	{
		qDebug() << "Failed to load session table template from resource";
	}

	populateSessions();
	m_lblRunning->setText(QString(NO_PROC_TEXT));
	m_btnStopSession->setEnabled(false);

#ifdef OOM_INSTALL_BIN
	//qDebug() << OOM_INSTALL_BIN;
	//qDebug() << LIBDIR;
	qDebug() << "Found install path: " << OOM_INSTALL_BIN;
#endif
	//m_webView->setUrl(QUrl("http://www.openoctave.org"));
	//m_webView->setUrl(QUrl("qrc:/oostudio.html"));

	m_heartBeat = 0;
	//m_heartBeat = new QTimer(this);
	//m_heartBeat->setObjectName("timer");
	//connect(m_heartBeat, SIGNAL(timeout()), this, SLOT(heartBeat()));
	//m_heartBeat->start(50);
	
	//Hide all the progressBars
	downloadEnded(ALL);
	
	populateDownloadMap();
	updateInstalledState();
	
	//bool advanced = settings.value("OOStudio/advanced", 0).toBool();
	toggleAdvanced(false);
	//m_btnMore->setChecked(advanced);
}

void OOStudio::heartBeat()
{
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
	QIcon oomIco(":/images/oostudio-oop-icon.png");
	QSize iconSize(471, 78);
	m_lblLogoSession->setPixmap(oomIco.pixmap(iconSize));

	QIcon donateIcon;
	donateIcon.addPixmap(QPixmap(":/images/oostudio-donate-on.png"), QIcon::Normal, QIcon::On);
	donateIcon.addPixmap(QPixmap(":/images/oostudio-donate-off.png"), QIcon::Normal, QIcon::Off);
	donateIcon.addPixmap(QPixmap(":/images/oostudio-donate-on.png"), QIcon::Active);
	m_btnDonateSonatina->setIcon(QIcon(donateIcon));
	m_btnDonateMaestro->setIcon(QIcon(donateIcon));
	m_btnDonateClassic->setIcon(QIcon(donateIcon));
	m_btnDonateAcoustic->setIcon(QIcon(donateIcon));
	m_btnDonateM7->setIcon(QIcon(donateIcon));

	QIcon moreIcon;
	moreIcon.addPixmap(QPixmap(":/images/oostudio-more-small-on.png"), QIcon::Normal, QIcon::On);
	moreIcon.addPixmap(QPixmap(":/images/oostudio-more-small-off.png"), QIcon::Normal, QIcon::Off);
	moreIcon.addPixmap(QPixmap(":/images/oostudio-more-small-on.png"), QIcon::Active);
	m_btnMore->setIcon(moreIcon);

	QIcon updateIcon;
	updateIcon.addPixmap(QPixmap(":/images/oostudio-update-small-on.png"), QIcon::Normal, QIcon::On);
	updateIcon.addPixmap(QPixmap(":/images/oostudio-update-small-off.png"), QIcon::Normal, QIcon::Off);
	updateIcon.addPixmap(QPixmap(":/images/oostudio-update-small-on.png"), QIcon::Active);
	m_btnUpdate->setIcon(updateIcon);

	QIcon stopIcon;
	stopIcon.addPixmap(QPixmap(":/images/oostudio-stop-small-on.png"), QIcon::Normal, QIcon::On);
	stopIcon.addPixmap(QPixmap(":/images/oostudio-stop-small-off.png"), QIcon::Normal, QIcon::Off);
	stopIcon.addPixmap(QPixmap(":/images/oostudio-stop-small-on.png"), QIcon::Active);
	m_btnStopSession->setIcon(QIcon(stopIcon));

	m_btnCancelSonatina->setIcon(QIcon(stopIcon));
	m_btnCancelMaestro->setIcon(QIcon(stopIcon));
	m_btnCancelClassic->setIcon(QIcon(stopIcon));
	m_btnCancelAcoustic->setIcon(QIcon(stopIcon));
	m_btnCancelM7->setIcon(QIcon(stopIcon));

	qDebug() << "OOStudio::downloadsComplete";
	//m_progressBox->setVisible(false);
	QIcon clearIcon;
	clearIcon.addPixmap(QPixmap(":/images/oostudio-clear-small-on.png"), QIcon::Normal, QIcon::On);
	clearIcon.addPixmap(QPixmap(":/images/oostudio-clear-small-off.png"), QIcon::Normal, QIcon::Off);
	clearIcon.addPixmap(QPixmap(":/images/oostudio-clear-small-on.png"), QIcon::Active);
	m_btnClearLog->setIcon(QIcon(clearIcon));
	m_btnClearCreate->setIcon(QIcon(clearIcon));

	QIcon newIcon;
	newIcon.addPixmap(QPixmap(":/images/oostudio-new-small-on.png"), QIcon::Normal, QIcon::On);
	newIcon.addPixmap(QPixmap(":/images/oostudio-new-small-off.png"), QIcon::Normal, QIcon::Off);
	newIcon.addPixmap(QPixmap(":/images/oostudio-new-small-on.png"), QIcon::Active);
	m_btnDoCreate->setIcon(newIcon);

	QIcon openIcon;
	openIcon.addPixmap(QPixmap(":/images/oostudio-open-on.png"), QIcon::Normal, QIcon::On);
	openIcon.addPixmap(QPixmap(":/images/oostudio-open-off.png"), QIcon::Normal, QIcon::Off);
	openIcon.addPixmap(QPixmap(":/images/oostudio-open-on.png"), QIcon::Active);

	QIcon deleteIcon;
	deleteIcon.addPixmap(QPixmap(":/images/oostudio-delete-on.png"), QIcon::Normal, QIcon::On);
	deleteIcon.addPixmap(QPixmap(":/images/oostudio-delete-off.png"), QIcon::Normal, QIcon::Off);
	deleteIcon.addPixmap(QPixmap(":/images/oostudio-delete-on.png"), QIcon::Active);

	m_btnDeleteSession->setIcon(QIcon(deleteIcon));
	m_btnDeleteTemplate->setIcon(QIcon(deleteIcon));
	m_btnLoadSession->setIcon(QIcon(openIcon));
	m_btnLoadTemplate->setIcon(QIcon(openIcon));

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

	QIcon downloadAllIcon;
	downloadAllIcon.addPixmap(QPixmap(":/images/oostudio-download-all-on.png"), QIcon::Normal, QIcon::On);
	downloadAllIcon.addPixmap(QPixmap(":/images/oostudio-download-all-off.png"), QIcon::Normal, QIcon::Off);
	downloadAllIcon.addPixmap(QPixmap(":/images/oostudio-download-all-on.png"), QIcon::Active);
	m_btnDownload->setIcon(downloadAllIcon);

	QIcon downloadIcon;
	downloadIcon.addPixmap(QPixmap(":/images/oostudio-download-on.png"), QIcon::Normal, QIcon::On);
	downloadIcon.addPixmap(QPixmap(":/images/oostudio-download-off.png"), QIcon::Normal, QIcon::Off);
	downloadIcon.addPixmap(QPixmap(":/images/oostudio-download-on.png"), QIcon::Active);
	m_btnDownloadSonatina->setIcon(QIcon(downloadIcon));
	m_btnDownloadMaestro->setIcon(QIcon(downloadIcon));
	m_btnDownloadClassic->setIcon(QIcon(downloadIcon));
	m_btnDownloadAccoustic->setIcon(QIcon(downloadIcon));
	m_btnDownloadM7->setIcon(QIcon(downloadIcon));

	m_lblSSOTitle->setPixmap(QPixmap(":/images/oostudio-sonatina.png"));
	m_lblMaestroTitle->setPixmap(QPixmap(":/images/oostudio-maestro.png"));
	m_lblClassicTitle->setPixmap(QPixmap(":/images/oostudio-classic_guitar.png"));
	m_lblAccousticTitle->setPixmap(QPixmap(":/images/oostudio-accoustic_guitar.png"));
	m_lblM7Title->setPixmap(QPixmap(":/images/oostudio-irs.png"));

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
	
	m_commandSelectModel = new QItemSelectionModel(m_commandModel);
	m_commandTable->setSelectionModel(m_commandSelectModel);

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
	//m_webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
	//connect(m_webView, SIGNAL(linkClicked(const QUrl&)), this, SLOT(showExternalLinks(const QUrl&)));
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
	connect(m_txtCommand, SIGNAL(returnPressed()), this, SLOT(addCommand()));
	connect(m_btnDeleteCommand, SIGNAL(clicked()), this, SLOT(deleteCommand()));
	connect(m_btnClearLog, SIGNAL(clicked()), this, SLOT(clearLogger()));
	connect(m_btnDeleteTemplate, SIGNAL(clicked()), this, SLOT(deleteTemplate()));
	connect(m_btnDeleteSession, SIGNAL(clicked()), this, SLOT(deleteSession()));
	//connect(m_btnStopSession, SIGNAL(clicked()), this, SLOT(stopCurrentSession()));
	connect(m_btnStopSession, SIGNAL(clicked()), this, SLOT(cleanupProcessList()));
	connect(m_cmbTemplate, SIGNAL(currentIndexChanged(int)), this, SLOT(templateSelectionChanged(int)));
	connect(m_cmbTemplate, SIGNAL(activated(int)), this, SLOT(templateSelectionChanged(int)));
	connect(m_cmbEditMode, SIGNAL(currentIndexChanged(int)), this, SLOT(editModeChanged(int)));
	connect(m_btnMore, SIGNAL(toggled(bool)), this, SLOT(toggleAdvanced(bool)));
	connect(m_btnDefault, SIGNAL(clicked()), this, SLOT(setDefaultJackCommand()));

	connect(m_sessionTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(sessionDoubleClicked(QModelIndex)));
	connect(m_templateTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(templateDoubleClicked(QModelIndex)));

	connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTopTabChanged(int)));
	connect(m_sessionBox, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));

	//Download Tab connections
	connect(m_downloader, SIGNAL(downloadStarted(int)), this, SLOT(downloadStarted(int)));
	connect(m_downloader, SIGNAL(downloadEnded(int)), this, SLOT(downloadEnded(int)));
	connect(m_downloader, SIGNAL(downloadCanceled(int)), this, SLOT(downloadCanceled(int)));
	connect(m_downloader, SIGNAL(downloadError(int, QString)), this, SLOT(downloadError(int, QString)));
	connect(m_downloader, SIGNAL(downloadsComplete()), this, SLOT(downloadsComplete()));
	connect(m_downloader, SIGNAL(displayMessage(QString)), this, SLOT(showMessage(QString)));

	connect(m_downloader, SIGNAL(trackSonatinaProgress(qint64, qint64)), this, SLOT(trackSonatinaProgress(qint64, qint64)));
	connect(m_downloader, SIGNAL(trackMaestroProgress(qint64, qint64)), this, SLOT(trackMaestroProgress(qint64, qint64)));
	connect(m_downloader, SIGNAL(trackClassicProgress(qint64, qint64)), this, SLOT(trackClassicProgress(qint64, qint64)));
	connect(m_downloader, SIGNAL(trackAcousticProgress(qint64, qint64)), this, SLOT(trackAcousticProgress(qint64, qint64)));
	connect(m_downloader, SIGNAL(trackM7Progress(qint64, qint64)), this, SLOT(trackM7Progress(qint64, qint64)));
	
	connect(this, SIGNAL(cancelDownload(int)), m_downloader, SLOT(cancelDownload(int)));

	connect(m_chk44, SIGNAL(toggled(bool)), this, SLOT(updateInstalledState()));
	connect(m_chk48, SIGNAL(toggled(bool)), this, SLOT(updateInstalledState()));
	connect(m_btnDownloadSonatina, SIGNAL(clicked()), this, SLOT(downloadSonatina()));
	connect(m_btnDownloadMaestro, SIGNAL(clicked()), this, SLOT(downloadMaestro()));
	connect(m_btnDownloadClassic, SIGNAL(clicked()), this, SLOT(downloadClassic()));
	connect(m_btnDownloadAccoustic, SIGNAL(clicked()), this, SLOT(downloadAcoustic()));
	connect(m_btnDownloadM7, SIGNAL(clicked()), this, SLOT(downloadM7()));
	connect(m_btnDownload, SIGNAL(clicked()), this, SLOT(downloadAll()));

	connect(m_btnCancelSonatina, SIGNAL(clicked()), this, SLOT(cancelSonatina()));
	connect(m_btnCancelMaestro, SIGNAL(clicked()), this, SLOT(cancelMaestro()));
	connect(m_btnCancelClassic, SIGNAL(clicked()), this, SLOT(cancelClassic()));
	connect(m_btnCancelAcoustic, SIGNAL(clicked()), this, SLOT(cancelAcoustic()));
	connect(m_btnCancelM7, SIGNAL(clicked()), this, SLOT(cancelM7()));

	connect(m_btnDonateSonatina, SIGNAL(clicked()), this, SLOT(donateSonatina()));
	connect(m_btnDonateMaestro, SIGNAL(clicked()), this, SLOT(donateMaestro()));
	connect(m_btnDonateClassic, SIGNAL(clicked()), this, SLOT(donateClassic()));
	connect(m_btnDonateAcoustic, SIGNAL(clicked()), this, SLOT(donateAcoustic()));
	connect(m_btnDonateM7, SIGNAL(clicked()), this, SLOT(donateM7()));
}/*}}}*/

void OOStudio::updateHeaders()/*{{{*/
{
	m_sessionModel->setHorizontalHeaderLabels(m_sessionlabels);
	
	m_templateModel->setHorizontalHeaderLabels(m_templatelabels);
	
	//m_sessionTable->setColumnWidth(0, 200);
	//m_templateTable->setColumnWidth(0, 200);
	m_sessionTable->resizeRowsToContents();
	m_templateTable->resizeRowsToContents();

	m_commandModel->setHorizontalHeaderLabels(m_commandlabels);

	m_loggerModel->setHorizontalHeaderLabels(m_loglabels);
	m_loggerTable->setColumnWidth(0, 110);
	m_loggerTable->setColumnWidth(1, 70);
}/*}}}*/

void OOStudio::populateSessions(bool usehash)/*{{{*/
{
	QIcon oomIco(":/images/oostudio_session_icon.png");
	QIcon oomIcoTemplate(":/images/oostudio_session_icon_template.png");
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
				if(session->istemplate)
				{
					QFileInfo finfo(session->path);
					QDateTime cdate(QDate::currentDate());
					if(finfo.exists())
					{
						cdate = finfo.created();
					}
					QStandardItem* name = new QStandardItem(m_sessionTemplate.arg(session->name).arg(cdate.toString()).arg(session->fromTemplate ? tr("Template") : tr("Session")).arg(session->createdFrom).arg(session->notes));
					name->setData(session->name, SessionNameRole);
					name->setIcon(QIcon(oomIcoTemplate));
					m_templateModel->appendRow(name);
					m_cmbTemplate->addItem("T: "+session->name, session->name);
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
				if(!session->istemplate)
				{
					QFileInfo finfo(session->path);
					QDateTime cdate(QDate::currentDate());
					if(finfo.exists())
					{
						cdate = finfo.created();
					}
					QStandardItem* name = new QStandardItem(m_sessionTemplate.arg(session->name).arg(cdate.toString()).arg(session->fromTemplate ? tr("Template") : tr("Session")).arg(session->createdFrom).arg(session->notes));
					name->setData(session->name, SessionNameRole);
					name->setIcon(QIcon(oomIco));
					m_sessionModel->appendRow(name);
					m_cmbTemplate->addItem("S: "+session->name, session->name);
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
				if(session->istemplate)
				{
					QFileInfo finfo(session->path);
					QDateTime cdate(QDate::currentDate());
					if(finfo.exists())
					{
						cdate = finfo.created();
					}
					QStandardItem* name = new QStandardItem(m_sessionTemplate.arg(session->name).arg(cdate.toString()).arg(session->fromTemplate ? tr("Template") : tr("Session")).arg(session->createdFrom).arg(session->notes));
					name->setData(session->name, SessionNameRole);
					name->setIcon(QIcon(oomIcoTemplate));
					m_templateModel->appendRow(name);
					m_cmbTemplate->addItem("T: "+session->name, session->name);
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
				if(!session->istemplate)
				{
					QFileInfo finfo(session->path);
					QDateTime cdate(QDate::currentDate());
					if(finfo.exists())
					{
						cdate = finfo.created();
					}
					QStandardItem* name = new QStandardItem(m_sessionTemplate.arg(session->name).arg(cdate.toString()).arg(session->fromTemplate ? tr("Template") : tr("Session")).arg(session->createdFrom).arg(session->notes));
					name->setData(session->name, SessionNameRole);
					name->setIcon(QIcon(oomIco));
					m_sessionModel->appendRow(name);
					m_cmbTemplate->addItem("S: "+session->name, session->name);
					m_sessionMap[session->name] = session;
				}
			}
		}
	}

	updateHeaders();
}/*}}}*/

void OOStudio::toggleAdvanced(bool state)/*{{{*/
{
	label_9->setVisible(state);
	label_10->setVisible(state);
	label->setVisible(state);
	label_4->setVisible(state);
	m_cmbLSCPMode->setVisible(state);
	m_txtLSCP->setVisible(state);
	m_btnBrowseLSCP->setVisible(state);
	m_btnBrowseOOM->setVisible(state);
	m_btnBrowsePath->setVisible(state);
	m_txtOOMPath->setVisible(state);
	m_commandBox->setVisible(state);
	m_customBox->setVisible(state);
	m_txtLocation->setVisible(state);
	m_chkTemplate->setVisible(state);
	label_24->setVisible(state);
	
	//Turn off in advanced
	//m_lblBlurb->setVisible(!state);
}/*}}}*/

bool OOStudio::checkPackageInstall(int id)/*{{{*/
{
	QDir soundsDir(SOUNDS_DIR);
	if(!soundsDir.exists())
		soundsDir.mkpath(SOUNDS_DIR);
	QDir templateDir(QString(TEMPLATE_DIR).append(QDir::separator()).append("libraries"));
	if(!templateDir.exists())
		templateDir.mkpath(QString(TEMPLATE_DIR).append(QDir::separator()).append("libraries"));
	QDir sso(SONATINA_PATH);
	QDir ssoLocal(QString(SOUNDS_DIR).append(QDir::separator()).append("sonatina"));

	bool retval = false;

	SamplePack pack = (SamplePack)id;

	switch(pack)
	{
		case Sonatina:
		{
			if(sso.exists() && !ssoLocal.exists())
			{
				QFile::link(QFileInfo(sso.absolutePath()).absolutePath(), QString(SOUNDS_DIR).append(QDir::separator()).append("sonatina"));
			}
			if((sso.exists() || soundsDir.exists(QString("sonatina").append(QDir::separator()).append(SONATINA_LOCAL_PATH)))
				|| m_downloader->isRunning(Sonatina))
			{
				//SSO is installed
				retval = true;
			}
		}
		break;
		case Maestro:
		{
			if(soundsDir.exists(MAESTRO_PATH) || m_downloader->isRunning(Maestro))
				retval = true;
		}
		break;
		case ClassicGuitar:
		{
			if(soundsDir.exists(CLASSIC_PATH) || m_downloader->isRunning(ClassicGuitar))
				retval = true;
		}
		break;
		case AcousticGuitar:
		{
			if(soundsDir.exists(ACCOUSTIC_PATH) || m_downloader->isRunning(AcousticGuitar))
				retval = true;
		}
		break;
		case M7IR44:
		{
			if(templateDir.exists(M7IR44_PATH) || m_downloader->isRunning(M7IR44))
				retval = true;
		}
		break;
		case M7IR48:
		{
			if(templateDir.exists(M7IR48_PATH) || m_downloader->isRunning(M7IR48))
				retval = true;
		}
		break;
		case ALL:
		{
		}
		break;
	}
	return retval;
}/*}}}*/

void OOStudio::updateInstalledState()/*{{{*/
{
	int count = 0;
	if(checkPackageInstall(Sonatina))
	{
		//SSO is installed
		if(!m_downloader->isRunning(Sonatina))
			m_lblSSOState->setPixmap(QPixmap(":/images/oostudio-installed.png"));
		else
			m_lblSSOState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadSonatina->setEnabled(false);
		++count;
	}
	else
	{
		m_lblSSOState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadSonatina->setEnabled(true);
	}
	if(checkPackageInstall(Maestro))
	{
		if(!m_downloader->isRunning(Maestro))
			m_lblMaestroState->setPixmap(QPixmap(":/images/oostudio-installed.png"));
		else
			m_lblMaestroState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadMaestro->setEnabled(false);
		++count;
	}
	else
	{
		m_lblMaestroState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadMaestro->setEnabled(true);
	}
	if(checkPackageInstall(ClassicGuitar))
	{
		if(!m_downloader->isRunning(ClassicGuitar))
			m_lblClassicState->setPixmap(QPixmap(":/images/oostudio-installed.png"));
		else
			m_lblClassicState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadClassic->setEnabled(false);
		++count;
	}
	else
	{
		m_lblClassicState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadClassic->setEnabled(true);
	}
	if(checkPackageInstall(AcousticGuitar))
	{
		if(!m_downloader->isRunning(AcousticGuitar))
			m_lblAccousticState->setPixmap(QPixmap(":/images/oostudio-installed.png"));
		else
			m_lblAccousticState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadAccoustic->setEnabled(false);
		++count;
	}
	else
	{
		m_lblAccousticState->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
		m_btnDownloadAccoustic->setEnabled(true);
	}
	if(m_chk44->isChecked())
	{
		if(checkPackageInstall(M7IR44))
		{
			if(!m_downloader->isRunning(M7IR44))
				m_lblM7State->setPixmap(QPixmap(":/images/oostudio-installed.png"));
			else
				m_lblM7State->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
			m_btnDownloadM7->setEnabled(false);
			++count;
		}
		else
		{
			m_lblM7State->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
			m_btnDownloadM7->setEnabled(true);
		}
	}
	else
	{
		if(checkPackageInstall(M7IR48))
		{
			if(!m_downloader->isRunning(M7IR48))
				m_lblM7State->setPixmap(QPixmap(":/images/oostudio-installed.png"));
			else
				m_lblM7State->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
			m_btnDownloadM7->setEnabled(false);
			++count;
		}
		else
		{
			m_lblM7State->setPixmap(QPixmap(":/images/oostudio-not-installed.png"));
			m_btnDownloadM7->setEnabled(true);
		}
	}
	if(count == 5)
	{
		m_btnDownload->setEnabled(false);
	}
	else
	{
		m_btnDownload->setEnabled(true);
	}
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
	if(tab == 3)
	{
		m_loggerTable->resizeRowsToContents();
		m_loggerTable->scrollToBottom();
	}
}/*}}}*/

void OOStudio::currentTabChanged(int tab)/*{{{*/
{
	if(tab == 0)
	{
		m_sessionTable->resizeRowsToContents();
	}
	else
	{
		m_templateTable->resizeRowsToContents();
	}
}/*}}}*/

QString OOStudio::getValidName(QString name)/*{{{*/
{
	QString newName(name);
	if(m_sessionMap.contains(newName)) 
	{
		int i = 1;
		int index = newName.lastIndexOf(QRegExp("_Copy*"));
		if(index > 0)
		{
			newName = newName.remove(index, newName.size());
		}
		newName += "_Copy";
		while (m_sessionMap.contains(newName + QString::number(i)))
			++i;
		newName += QString::number(i);
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
					if(session->istemplate)
					{
						m_sessionBox->setCurrentIndex(1);
					}
					else
					{
						m_sessionBox->setCurrentIndex(0);
					}
					m_tabWidget->setCurrentIndex(0);
					updateHeaders();
					m_sessionMap[session->name] = session;
					saveSettings();
					populateSessions(true);
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
	//item->setForeground(QColor(187, 191, 187));
	m_commandModel->appendRow(item);
	updateHeaders();
	m_txtCommand->setText("");
	m_txtCommand->setFocus(Qt::OtherFocusReason);
}/*}}}*/

void OOStudio::deleteCommand()/*{{{*/
{
	QList<int> del;
	if(m_commandSelectModel && m_commandSelectModel->hasSelection())
	{
		QModelIndexList	selected = m_commandSelectModel->selectedRows(0);
		if(selected.size())
		{
			foreach(QModelIndex index, selected)
			{
				QStandardItem* item = m_commandModel->itemFromIndex(index);
				if(item)
				{
					del.append(item->row());
				}
			}
		}
	}
	if(!del.isEmpty())
	{
		foreach(int i, del)
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
	QSettings settings("OOMidi", "OOStudio");
	QString jackCmd = settings.value("OOStudio/jackCommand", JACK_COMMAND).toString();
	m_txtJackCommand->setText(jackCmd);
	m_cmbLSCPMode->setCurrentIndex(0);
	m_commandModel->clear();
	m_chkTemplate->setChecked(false);
	m_txtNotes->setText("");
	updateHeaders();
}/*}}}*/

void OOStudio::cleanupProcessList()/*{{{*/
{
	printf("OOStudio::cleanupProcessList\n");
	m_incleanup = true;
	if(m_oomThread && m_oomThread->programStarted())
	{
		//oomRemoteShutdown();
		m_oomThread->quit();
		m_oomThread->wait();
		delete m_oomThread;
		m_oomThread = 0;
		qDebug() << "OOMidi Shudown Complete";
	}
	if(m_customThread && m_customThread->programStarted())
	{
		m_customThread->quit();
		m_customThread->wait();
		delete m_customThread;
		m_customThread = 0;
	}
	if(m_lsThread && m_lsThread->programStarted())
	{
		m_lsThread->quit();
		m_lsThread->wait();
		delete m_lsThread;
		m_lsThread = 0;
	}
	if(m_jackThread && m_jackThread->programStarted())
	{
		m_jackThread->quit();
		m_jackThread->wait();
		delete m_jackThread;
		m_jackThread = 0;
	}
	m_lblRunning->setText(QString(NO_PROC_TEXT));
	m_btnStopSession->setEnabled(false);
	m_current = 0;
	m_loading = false;
	m_incleanup = false;
}/*}}}*/

void OOStudio::linuxSamplerStarted()/*{{{*/
{
	m_customThread = new OOThread(m_current);
	connect(m_customThread, SIGNAL(startupSuccess()), this, SLOT(customStarted()));
	connect(m_customThread, SIGNAL(startupFailed()), this, SLOT(customFailed()));
	m_customThread->start();
}/*}}}*/

void OOStudio::oomStarted()/*{{{*/
{
	m_lblRunning->setText(m_current->name);
	m_btnStopSession->setEnabled(true);
	m_loading = false;
}/*}}}*/

void OOStudio::jackStarted()/*{{{*/
{
	m_lsThread = new LinuxSamplerProcessThread(m_current);
	connect(m_lsThread, SIGNAL(startupSuccess()), this, SLOT(linuxSamplerStarted()));
	connect(m_lsThread, SIGNAL(startupFailed()), this, SLOT(linuxSamplerFailed()));
	connect(m_lsThread, SIGNAL(logMessage(LogInfo*)), this, SLOT(writeLog(LogInfo*)));
	m_lsThread->start();
}/*}}}*/

void OOStudio::customStarted()/*{{{*/
{
	m_oomThread = new OOMProcessThread(m_current);
	connect(m_oomThread, SIGNAL(startupSuccess()), this, SLOT(oomStarted()));
	connect(m_oomThread, SIGNAL(startupFailed()), this, SLOT(oomFailed()));
	connect(m_oomThread, SIGNAL(processExit(int)), this, SLOT(processOOMExit(int)));
	connect(m_oomThread, SIGNAL(logMessage(LogInfo*)), this, SLOT(writeLog(LogInfo*)));
	m_oomThread->start();
}/*}}}*/

void OOStudio::linuxSamplerFailed()/*{{{*/
{
	//Cleanup jackd and self
	printf("OOStudio::linuxSamplerFailed\n");
	QString msg(tr("Failed to start to linuxsample server"));
	msg.append("\nCommand:\n").append(m_current->lscommand);
	QMessageBox::critical(this, tr("Create Failed"), msg);
	
	m_incleanup = false;
	if(m_lsThread && m_lsThread->programStarted())
	{
		m_lsThread->quit();
		m_lsThread->wait();
		delete m_lsThread;
		m_lsThread = 0;
	}
	if(m_jackThread && m_jackThread->programStarted())
	{
		m_jackThread->quit();
		m_jackThread->wait();
		delete m_jackThread;
		m_jackThread = 0;
	}
	m_incleanup = false;
	m_current = 0;
}/*}}}*/

void OOStudio::oomFailed()/*{{{*/
{
	//Cleanup everything
	QString msg(tr("Failed to run OOMidi"));
	msg.append("\nCommand:\n").append(OOM_INSTALL_BIN).append(" ").append(m_current->songfile);
	QMessageBox::critical(this, tr("Create Failed"), msg);

	cleanupProcessList();
}/*}}}*/

void OOStudio::jackFailed()/*{{{*/
{
	QString msg(tr("Failed to start to jackd server"));
	msg.append("\nwith Command:\n").append(m_current->jackcommand);
	QMessageBox::critical(this, tr("Jackd Failed"), msg);
	//Cleanup self or nothing
	if(m_jackThread)
	{
		m_jackThread->quit();
		m_jackThread->wait();
		delete m_jackThread;
		m_jackThread = 0;
	}
	m_loading = false;
	m_current = 0;
}/*}}}*/

void OOStudio::customFailed()/*{{{*/
{
	//Cleanup jackd, and ls
	printf("OOStudio::cleanupProcessList\n");
	QString msg(tr("Failed to start to custom command"));
	QMessageBox::critical(this, tr("Create Failed"), msg);
	
	m_incleanup = false;
	if(m_customThread && m_customThread->programStarted())
	{
		m_customThread->quit();
		m_customThread->wait();
		delete m_customThread;
		m_customThread = 0;
	}
	if(m_lsThread && m_lsThread->programStarted())
	{
		m_lsThread->quit();
		m_lsThread->wait();
		delete m_lsThread;
		m_lsThread = 0;
	}
	if(m_jackThread && m_jackThread->programStarted())
	{
		m_jackThread->quit();
		m_jackThread->wait();
		delete m_jackThread;
		m_jackThread = 0;
	}
	m_current = 0;
	m_loading = false;
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
	settings.setValue("advanced", m_btnMore->isChecked());
	settings.endGroup();
	settings.sync();
}/*}}}*/

void OOStudio::sessionDoubleClicked(QModelIndex index)/*{{{*/
{
	//printf("OOStudio::sessionDoubleClicked - row: %d\n", index.row());
	if(index.isValid())
	{
		QStandardItem* item = m_sessionModel->item(index.row());
		if(item)
		{
			OOSession* session = m_sessionMap.value(item->data(SessionNameRole).toString());
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
			OOSession* session = m_sessionMap.value(item->data(SessionNameRole).toString());
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
				OOSession* session = m_sessionMap.value(item->data(SessionNameRole).toString());
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
				OOSession* session = m_sessionMap.value(item->data(SessionNameRole).toString());
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
					msg.arg(item->data(SessionNameRole).toString()),
					QMessageBox::Ok, QMessageBox::Cancel
				) == QMessageBox::Ok)
				{
					OOSession* session = m_sessionMap.take(item->data(SessionNameRole).toString());
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
					msg.arg(item->data(SessionNameRole).toString()),
					QMessageBox::Ok, QMessageBox::Cancel
				) == QMessageBox::Ok)
				{
					OOSession* session = m_sessionMap.take(item->data(SessionNameRole).toString());
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
	return true;
}/*}}}*/

void OOStudio::loadSession(OOSession* session)/*{{{*/
{
	if(m_loading)
	{
		showMessage("WARNING: Already loading Session : "+m_current->name+" ... Please wait.");
		return;
	}
	if(session)
	{
		showMessage("OOStudio::loadSession :"+session->name);
		printf("OOStudio::loadSession : name: %s\n", session->name.toUtf8().constData());
		printf("OOStudio::loadSession : path: %s\n", session->path.toUtf8().constData());
		if(m_current)
			cleanupProcessList();
		m_current = session;
		m_loading = true;
		m_jackThread = new JackProcessThread(session);
		connect(m_jackThread, SIGNAL(startupSuccess()), this, SLOT(jackStarted()));
		connect(m_jackThread, SIGNAL(startupFailed()), this, SLOT(jackFailed()));
		connect(m_jackThread, SIGNAL(logMessage(LogInfo*)), this, SLOT(writeLog(LogInfo*)));
		m_jackThread->start();
		m_tabWidget->setCurrentIndex(3);
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
		bool fromtemplate = false;
		QString createdFrom(tr("Default"));
		if(m_cmbTemplate->currentIndex())
		{
			QString tpath = m_cmbTemplate->itemData(m_cmbTemplate->currentIndex()).toString();
			OOSession* oldSession = m_sessionMap.value(tpath);
			if(oldSession)
			{
				fromtemplate = oldSession->istemplate;
				createdFrom = oldSession->name;
			}
		}
		QDomDocument doc("OOStudioSession");
		QDomElement root = doc.createElement("OOStudioSession");
		doc.appendChild(root);
		QString basepath = convertPath(m_txtLocation->text());
		QString basename = m_txtName->text();
		basename = basename.simplified().replace(" ", "_");
		OOSession* newSession = new OOSession;
		newSession->name = basename;
		newSession->istemplate = istemplate;
		newSession->createdFrom = createdFrom;
		newSession->fromTemplate = fromtemplate;
		root.setAttribute("istemplate", istemplate);
		root.setAttribute("name", basename);
		root.setAttribute("fromtemplate", fromtemplate);
		root.setAttribute("createdfrom", createdFrom);

		QDir basedir(basepath);
		if(basedir.exists() && basedir.mkdir(basename) && basedir.cd(basename))
		{
			QString newPath(basedir.absolutePath());
			newPath.append(QDir::separator()).append(basename);
			QString filename(newPath+".oos");
			QString songfile(newPath+".oom");
			QString lscpfile(newPath+".lscp");
			QString notes(m_txtNotes->toPlainText());

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

				QDomElement notenode = doc.createElement("notes");
				root.appendChild(notenode);
				notenode.setAttribute("text", notes);
				newSession->notes = notes;

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
					newSession->path = filename;
					m_sessionMap[basename] = newSession;
					showMessage("Successfully Created Session: "+basename);
					if(!istemplate)
					{
						//Not a template so load it now and minimize;
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
					resetCreate();
					populateSessions(true);
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
		bool fromtemplate = oldSession->fromTemplate;
		QString createdFrom(oldSession->createdFrom);
		
		newSession->name = oldSession->name;
		newSession->istemplate = istemplate;
		newSession->fromTemplate = fromtemplate;
		newSession->createdFrom = createdFrom;
		
		root.setAttribute("istemplate", istemplate);
		root.setAttribute("name", basename);
		root.setAttribute("fromtemplate", fromtemplate);
		root.setAttribute("createdfrom", createdFrom);

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
			QString notes(m_txtNotes->toPlainText());

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

				QDomElement notenode = doc.createElement("notes");
				root.appendChild(notenode);
				notenode.setAttribute("text", notes);
				newSession->notes = notes;

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
					//Write out the session to settings and update table
					newSession->path = filename;
					m_sessionMap[basename] = newSession;
					saveSettings();
					resetCreate();
					populateSessions(true);
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
					QMessageBox::critical(this, tr("Update Failed"), msg);
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
				QMessageBox::critical(this, tr("Update Failed"), msg);
			}
		}
	}
	else
	{
		QMessageBox::information(
			this,
			tr("Invalid Form"),
			tr("All required information has not been provided, please correct the problem and click \"Create New\" again?"),
			QMessageBox::Ok);
	}
}/*}}}*/

void OOStudio::setDefaultJackCommand()
{
	QSettings settings("OOMidi", "OOStudio");
	settings.beginGroup("OOStudio");
	settings.setValue("jackCommand", m_txtJackCommand->text());
	settings.endGroup();
	settings.sync();
}

void OOStudio::templateSelectionChanged(int index)/*{{{*/
{
	QString tpath = m_cmbTemplate->itemData(index).toString();
	//printf("OOStudio::templateSelectionChanged index: %d, name: %s sessions:%d\n", index, tpath.toUtf8().constData(), m_sessionMap.size());
	if(!index || (index <= 2 && m_cmbEditMode->currentIndex() == 1))
	{
		resetCreate();
		return;
	}
	
	if(index && (index <= 2))
	{
		if(index == 1 && !checkPackageInstall(Sonatina))
		{
			QMessageBox::critical(this,
				tr("Missing Libraries"),
				tr("This template requires the following sample libraries.\n\n"
					" Required: Symphonic Orchestral Library\n"
					" Recommended: Samplicity M7 Impulse Response\n\n"
					"Install them from the downloads tab."));
			resetCreate();
			m_tabWidget->setCurrentIndex(2);
			return;
		}
		if(index == 2 && (!checkPackageInstall(Sonatina) || !checkPackageInstall(Maestro)
				|| !checkPackageInstall(ClassicGuitar) || !checkPackageInstall(AcousticGuitar)))
		{
			QMessageBox::critical(this,
				tr("Missing Libraries"),
				tr("This template requires the following sample libraries.\n\n"
					" Required: Symphonic Orchestral Library\n"
					" Required: Maestro Concert Grand\n"
					" Required: Classical Guitar FREE\n"
					" Required: Acoustic Guitar FREE\n"
					" Recommended: Samplicity M7 Impulse Response\n\n"
					"Install them from the downloads tab."));
			resetCreate();
			m_tabWidget->setCurrentIndex(2);
			return;
		}
	}
	
	//Copy the values from the other template into this
	if(!m_sessionMap.isEmpty() && m_sessionMap.contains(tpath))
	{
		OOSession* session = m_sessionMap.value(tpath);
		if(session)
		{
			QSettings settings("OOMidi", "OOStudio");
			QString jackCmd = settings.value("OOStudio/jackCommand", session->jackcommand).toString();

			if(m_cmbEditMode->currentIndex() == 1)
			{
				QFileInfo fi(session->path);
				m_txtLocation->setText(fi.dir().absolutePath());
				m_txtName->setText(session->name);
				m_chkTemplate->setChecked(session->istemplate);
				m_txtJackCommand->setText(session->jackcommand);
			}
			else
			{
				m_txtJackCommand->setText(jackCmd);
				m_txtName->setText(getValidName(session->name));
			}
			m_commandModel->clear();
			for(int i = 0; i < session->commands.size(); ++i)
			{
				QStandardItem* item = new QStandardItem(session->commands.at(i));
				m_commandModel->appendRow(item);
			}
			for(int i = 0; i < session->postCommands.size(); ++i)
			{
				QStandardItem* item = new QStandardItem(session->postCommands.at(i));
				m_commandModel->appendRow(item);
			}
			m_chkStartJack->setChecked(session->loadJack);
			m_chkStartLS->setChecked(session->loadls);
			m_txtLSCommand->setText(session->lscommand);
			m_txtLSHost->setText(session->lshostname);
			m_txtLSPort->setText(QString::number(session->lsport));
			m_cmbLSCPMode->setCurrentIndex(session->lscpMode);
			m_txtLSCP->setText(session->lscpPath);
			m_txtOOMPath->setText(session->songfile);
			m_txtNotes->setText(session->notes);
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
		m_btnMore->setChecked(true);
	}
	else
	{
		m_txtName->setEnabled(true);
		m_txtLocation->setEnabled(true);
		m_btnBrowsePath->setEnabled(true);
		m_btnDoCreate->setEnabled(true);
		m_btnUpdate->setEnabled(false);
		m_btnMore->setChecked(false);
	}
}/*}}}*/

OOSession* OOStudio::readSession(QString filename)/*{{{*/
{
	OOSession* session = 0;
	QDomDocument temp("OOStudioSession");
	QFile tempFile(filename);
	if(!tempFile.exists())
	{
		QString msg(tr("The session file:\n %1 \nwas not found"));
		QMessageBox::critical(this,
			tr("File Not Found"),
			msg.arg(filename));
		return 0;
	}
	if(!tempFile.open(QIODevice::ReadOnly))
	{
		QString msg(tr("Failed to open session file:\n %1 \n%2"));
		QMessageBox::critical(this,
			tr("File Open Error"),
			msg.arg(filename).arg(tempFile.errorString()));
		return session;
	}
	if(!temp.setContent(&tempFile))
	{
		QString msg(tr("Failed to parse xml structure for session file:\n %1"));
		QMessageBox::critical(this,
			tr("File Parsing Error"),
			msg.arg(filename));
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
		session->fromTemplate = root.attribute("fromtemplate", "1").toInt();
		session->createdFrom = root.attribute("createdfrom", tr("Default"));
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
		
		QDomNodeList lnotes = root.elementsByTagName("notes");
		//Backwards compat
		if(!lnotes.count())
			session->notes = "";
		
		for(int i = 0; i < lsong.count(); ++i)
		{
			QDomElement notes = lnotes.at(i).toElement();
			session->notes = notes.attribute("text");
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

void OOStudio::resizeEvent(QResizeEvent* evt)/*{{{*/
{
	QMainWindow::resizeEvent(evt);
	m_loggerTable->resizeRowsToContents();
	m_sessionTable->resizeRowsToContents();
	m_templateTable->resizeRowsToContents();
}/*}}}*/

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

void OOStudio::processOOMExit(int code)/*{{{*/
{
	if(!m_incleanup)
	{
		switch(code)
		{
			case 0:
			{
				cleanupProcessList();
				showNormal();
			}
			break;
			default:
			{
				qDebug() << "OOMidi exited with error code: " << code;
				cleanupProcessList();
				showNormal();
			}
			break;
		}
	}
}/*}}}*/

void OOStudio::writeLog(LogInfo* log)/*{{{*/
{
	if(log)
	{
		QString messages(log->message);
		if(messages.isEmpty())
			return;
		QList<QStandardItem*> rowData;
		QStandardItem* command = new QStandardItem(log->name);
		QStandardItem* logmsg = new QStandardItem(messages);
		QStandardItem* level = new QStandardItem(log->codeString);
		if(log->codeString == "ERROR")
		{
			command->setForeground(QColor(99, 36, 36));
			level->setForeground(QColor(99, 36, 36));
		}
		rowData << command << level << logmsg;
		m_loggerModel->appendRow(rowData);
		updateHeaders();
		m_loggerTable->resizeRowsToContents();
		m_loggerTable->scrollToBottom();
	}
}/*}}}*/


void OOStudio::populateDownloadMap()/*{{{*/
{
	m_downloadMap.clear();
	QList<DownloadPackage*> list;
	QDomDocument temp("OOStudioDownloader");
	QFile tempFile(":/oostudio.xml");
	if(!tempFile.open(QIODevice::ReadOnly))
	{
		//TODO:We need to error here
		return;
	}
	if(!temp.setContent(&tempFile))
	{
		//TODO:We need to error here
		return;
	}
	//We now have a loaded file so we get the data from it.
	QDomElement root = temp.documentElement();
	if(!root.isNull())
	{
		QDomNodeList flist = root.elementsByTagName("sonatina");
		for(int i = 0; i < flist.count(); ++i)
		{
			qDebug() << "Populating Sonatina DownloadPackage from xml";
			DownloadPackage* sso = new DownloadPackage;
			sso->name = QString("Sonatina Orchestra");
		
			QDomElement command = flist.at(i).toElement();
			sso->type = (SamplePack)command.attribute("id").toInt();
			sso->filename = command.attribute("filename");
			sso->format = command.attribute("format").toInt();
			sso->path = QUrl(command.attribute("path"));
			sso->homepage = QUrl(command.attribute("homepage"));
			sso->install_path = QFileInfo(QString(SOUNDS_DIR));
			m_downloadMap[sso->type] = sso;
		}
		flist = root.elementsByTagName("maestro");
		for(int i = 0; i < flist.count(); ++i)
		{
			qDebug() << "Populating Maestro Piano DownloadPackage from xml";
			QDomElement command = flist.at(i).toElement();

			DownloadPackage* maestro = new DownloadPackage;
			maestro->name = QString("Maestro Piano");
		
			maestro->type = (SamplePack)command.attribute("id").toInt();
			maestro->filename = command.attribute("filename");
			maestro->format = command.attribute("format").toInt();
			maestro->path = QUrl(command.attribute("path"));
			maestro->homepage = QUrl(command.attribute("homepage"));
			maestro->install_path = QFileInfo(QString(SOUNDS_DIR).append("maestro"));
			m_downloadMap[maestro->type] = maestro;
		}
		flist = root.elementsByTagName("classicfree");
		for(int i = 0; i < flist.count(); ++i)
		{
			qDebug() << "Populating Sonatina DownloadPackage from xml";
			QDomElement command = flist.at(i).toElement();

			DownloadPackage* classic = new DownloadPackage;
			classic->name = QString("Classic Guitar");
		
			classic->type = (SamplePack)command.attribute("id").toInt();
			classic->filename = command.attribute("filename");
			classic->format = command.attribute("format").toInt();
			classic->path = QUrl(command.attribute("path"));
			classic->homepage = QUrl(command.attribute("homepage"));
			classic->install_path = QFileInfo(QString(SOUNDS_DIR));
			m_downloadMap[classic->type] = classic;
		}
		flist = root.elementsByTagName("acousticfree");
		for(int i = 0; i < flist.count(); ++i)
		{
			qDebug() << "Populating Acoustic Guitar DownloadPackage from xml";
			QDomElement command = flist.at(i).toElement();

			DownloadPackage* acoustic = new DownloadPackage;
			acoustic->name = QString("Acoustic Guitar");
		
			acoustic->type = (SamplePack)command.attribute("id").toInt();
			acoustic->filename = command.attribute("filename");
			acoustic->format = command.attribute("format").toInt();
			acoustic->path = QUrl(command.attribute("path"));
			acoustic->homepage = QUrl(command.attribute("homepage"));
			acoustic->install_path = QFileInfo(QString(SOUNDS_DIR));
			m_downloadMap[acoustic->type] = acoustic;
		}
		flist = root.elementsByTagName("m7ir");
		for(int i = 0; i < flist.count(); ++i)
		{
			QDomElement command = flist.at(i).toElement();

			DownloadPackage* m7ir = new DownloadPackage;
			SamplePack id = (SamplePack)command.attribute("id").toInt();
			if(id == M7IR44)
			{
				qDebug() << "Populating M7IR 44.1Khz DownloadPackage from xml";
				m7ir->name = QString("M7 IR 44.1Khz");
			}
			else
			{
				qDebug() << "Populating M7IR 48Khz DownloadPackage from xml";
				m7ir->name = QString("M7 IR 48Khz");
			}

			m7ir->type = id;
			m7ir->filename = command.attribute("filename");
			m7ir->format = command.attribute("format").toInt();
			m7ir->path = QUrl(command.attribute("path"));
			m7ir->homepage = QUrl(command.attribute("homepage"));
			m7ir->install_path = QFileInfo(QString(TEMPLATE_DIR).append(QDir::separator()).append("libraries"));
			m_downloadMap[m7ir->type] = m7ir;
		}
	}
}/*}}}*/

void OOStudio::downloadSonatina()
{
	download(Sonatina);
}

void OOStudio::downloadMaestro()
{
	download(Maestro);
}

void OOStudio::downloadClassic()
{
	download(ClassicGuitar);
}

void OOStudio::downloadAcoustic()
{
	download(AcousticGuitar);
}

void OOStudio::downloadM7()
{
	if(m_chk44->isChecked())
	{
		download(M7IR44);
	}
	else
	{
		download(M7IR48);
	}
	m_chk44->blockSignals(true);
	m_chk48->blockSignals(true);
}

void OOStudio::downloadAll()
{
	m_btnDownload->blockSignals(true);
	download(ALL);
}

void OOStudio::download(int type)
{
	if(QMessageBox::question(this,
		tr("License Agreement"),
		tr("By selecting to download any of the following libraries you have read and agreed to accept the respective License agreements."
		"\nYou maintain all responsibility for your use of these libraries not the OpenOctaveProject."
		"\n\nDo you Agree?"),
		QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Ok)
	{
		m_chk44->blockSignals(false);
		m_chk48->blockSignals(false);
		return;
	}
	SamplePack pack = (SamplePack)type;
	switch(pack)
	{
		case Sonatina:
		{
			showMessage(tr("Sonatina download  requested"));
			if(!checkPackageInstall(Sonatina))
			{
				//DownloadPackage* pkg = m_downloadMap.value(type);
				m_downloader->startDownload(m_downloadMap.value(Sonatina));
				//QDesktopServices::openUrl(pkg->homepage);
			}
			m_btnDownloadSonatina->setEnabled(false);
		}
		break;
		case Maestro:
		{
			showMessage(tr("Maestro download  requested"));
			if(!checkPackageInstall(Maestro))
			{
				//DownloadPackage* pkg = m_downloadMap.value(type);
				m_downloader->startDownload(m_downloadMap.value(Maestro));
				//QDesktopServices::openUrl(pkg->homepage);
			}
			m_btnDownloadMaestro->setEnabled(false);
		}
		break;
		case ClassicGuitar:
		{
			showMessage(tr("Classical Guitar download  requested"));
			if(!checkPackageInstall(ClassicGuitar))
			{
				//DownloadPackage* pkg = m_downloadMap.value(type);
				m_downloader->startDownload(m_downloadMap.value(ClassicGuitar));
				//QDesktopServices::openUrl(pkg->homepage);
			}
			m_btnDownloadClassic->setEnabled(false);
		}
		break;
		case AcousticGuitar:
		{
			showMessage(tr("Acoustic Guitar download  requested"));
			if(!checkPackageInstall(AcousticGuitar))
			{
				//DownloadPackage* pkg = m_downloadMap.value(type);
				m_downloader->startDownload(m_downloadMap.value(AcousticGuitar));
				//QDesktopServices::openUrl(pkg->homepage);
			}
			m_btnDownloadAccoustic->setEnabled(false);
		}
		break;
		case M7IR44:
		{
			showMessage(tr("M7 44.1Khz IR download  requested"));
			if(!checkPackageInstall(M7IR44))
			{
				//DownloadPackage* pkg = m_downloadMap.value(type);
				m_downloader->startDownload(m_downloadMap.value(M7IR44));
				//QDesktopServices::openUrl(pkg->homepage);
			}
			m_btnDownloadM7->setEnabled(false);
		}
		break;
		case M7IR48:
		{
			showMessage(tr("M7 48Khz IR download  requested"));
			if(!checkPackageInstall(M7IR48))
			{
				//DownloadPackage* pkg = m_downloadMap.value(type);
				m_downloader->startDownload(m_downloadMap.value(M7IR48));
				//QDesktopServices::openUrl(pkg->homepage);
			}
			m_btnDownloadM7->setEnabled(false);
		}
		break;
		case ALL:
		{
			showMessage(tr("Download all requested"));
			m_btnDownload->setEnabled(false);
			m_btnDownload->blockSignals(false);

			if(!checkPackageInstall(Sonatina))
			{
				DownloadPackage* pkg = m_downloadMap.value(Sonatina);
				m_btnDownloadSonatina->setEnabled(false);
				m_downloader->startDownload(pkg);
				//QDesktopServices::openUrl(pkg->homepage);
			}
			if(!checkPackageInstall(Maestro))
			{
				DownloadPackage* pkg = m_downloadMap.value(Maestro);
				m_btnDownloadMaestro->setEnabled(false);
				m_downloader->startDownload(pkg);
				//QDesktopServices::openUrl(pkg->homepage);
			}
			if(!checkPackageInstall(ClassicGuitar))
			{
				DownloadPackage* pkg = m_downloadMap.value(ClassicGuitar);
				m_btnDownloadClassic->setEnabled(false);
				m_downloader->startDownload(pkg);
				//QDesktopServices::openUrl(pkg->homepage);
			}
			if(!checkPackageInstall(AcousticGuitar))
			{
				DownloadPackage* pkg = m_downloadMap.value(AcousticGuitar);
				m_btnDownloadAccoustic->setEnabled(false);
				m_downloader->startDownload(pkg);
				//QDesktopServices::openUrl(pkg->homepage);
			}
			if(m_chk44->isChecked())
			{
				if(!checkPackageInstall(M7IR44))
				{
					DownloadPackage* pkg = m_downloadMap.value(M7IR44);
					m_btnDownloadM7->setEnabled(false);
					m_downloader->startDownload(pkg);
					//QDesktopServices::openUrl(pkg->homepage);
				}
			}
			else
			{
				if(!checkPackageInstall(M7IR48))
				{
					DownloadPackage* pkg = m_downloadMap.value(M7IR48);
					m_btnDownloadM7->setEnabled(false);
					m_downloader->startDownload(pkg);
					//QDesktopServices::openUrl(pkg->homepage);
				}
			}
		}
		break;
	}
}

void OOStudio::cancelSonatina()
{
	if(m_downloader->isRunning(Sonatina) || m_downloader->isExtracting(Sonatina))
	{
		emit cancelDownload(Sonatina);
	}
	else
	{
		label_25->setVisible(false);
		m_progressSonatina->setVisible(false);
		m_btnCancelSonatina->setVisible(false);
		m_btnDonateSonatina->setVisible(false);
		if(m_progress.contains(Sonatina))
			m_progress.takeAt(m_progress.indexOf(Sonatina));
		if(!m_progress.count())
			m_progressBox->hide();
	}
}

void OOStudio::cancelMaestro()
{
	if(m_downloader->isRunning(Maestro) || m_downloader->isExtracting(Maestro))
	{
		emit cancelDownload(Maestro);
	}
	else
	{
		label_26->setVisible(false);
		m_progressMaestro->setVisible(false);
		m_btnCancelMaestro->setVisible(false);
		m_btnDonateMaestro->setVisible(false);
		if(m_progress.contains(Maestro))
			m_progress.takeAt(m_progress.indexOf(Maestro));
		if(!m_progress.count())
			m_progressBox->hide();
	}
}

void OOStudio::cancelClassic()
{
	if(m_downloader->isRunning(ClassicGuitar) || m_downloader->isExtracting(ClassicGuitar))
	{
		emit cancelDownload(ClassicGuitar);
	}
	else
	{
		label_27->setVisible(false);
		m_progressClassic->setVisible(false);
		m_btnCancelClassic->setVisible(false);
		m_btnDonateClassic->setVisible(false);
		if(m_progress.contains(ClassicGuitar))
			m_progress.takeAt(m_progress.indexOf(ClassicGuitar));
		if(!m_progress.count())
			m_progressBox->hide();
	}
}

void OOStudio::cancelAcoustic()
{
	if(m_downloader->isRunning(AcousticGuitar) || m_downloader->isExtracting(AcousticGuitar))
	{
		emit cancelDownload(AcousticGuitar);
	}
	else
	{
		label_30->setVisible(false);
		m_progressAcoustic->setVisible(false);
		m_btnCancelAcoustic->setVisible(false);
		m_btnDonateAcoustic->setVisible(false);
		if(m_progress.contains(AcousticGuitar))
			m_progress.takeAt(m_progress.indexOf(AcousticGuitar));
		if(!m_progress.count())
			m_progressBox->hide();
	}
}

void OOStudio::cancelM7()
{
	if((m_downloader->isRunning(M7IR44) || m_downloader->isExtracting(M7IR44))
		||(m_downloader->isRunning(M7IR48) || m_downloader->isExtracting(M7IR48)))
	{
		if(m_downloader->isRunning(M7IR44))
			emit cancelDownload(M7IR44);
		else
			emit cancelDownload(M7IR48);
	}
	else
	{
		label_31->setVisible(false);
		m_progressM7->setVisible(false);
		m_btnCancelM7->setVisible(false);
		m_btnDonateM7->setVisible(false);
		
		if(m_progress.contains(M7IR44))
			m_progress.takeAt(m_progress.indexOf(M7IR44));
		else if(m_progress.contains(M7IR48))
			m_progress.takeAt(m_progress.indexOf(M7IR48));
		if(!m_progress.count())
			m_progressBox->hide();
			
	}
}

void OOStudio::downloadEnded(int type)/*{{{*/
{
	SamplePack pack = (SamplePack)type;
	switch(pack)
	{
		case Sonatina:
		{
			showMessage(tr("Sonatina download complete"));
			//m_progressSonatina->setVisible(false);
			//m_btnDonateSonatina->setVisible(false);
			//label_25->setVisible(false);
			//m_btnCancelSonatina->setVisible(false);
			//
			m_progressSonatina->setMaximum(100);
			m_progressSonatina->setValue(100);
		}
		break;
		case Maestro:
		{
			showMessage(tr("Maestro download complete"));
			//m_progressMaestro->setVisible(false);
			//m_btnDonateMaestro->setVisible(false);
			//label_26->setVisible(false);
			//m_btnCancelMaestro->setVisible(false);
			m_progressMaestro->setMaximum(100);
			m_progressMaestro->setValue(100);
		}
		break;
		case ClassicGuitar:
		{
			showMessage(tr("Classical Guitar download complete"));
			//m_progressClassic->setVisible(false);
			//m_btnDonateClassic->setVisible(false);
			//label_27->setVisible(false);
			//m_btnCancelClassic->setVisible(false);
			m_progressClassic->setMaximum(100);
			m_progressClassic->setValue(100);
		}
		break;
		case AcousticGuitar:
		{
			showMessage(tr("Acoustic Guitar download complete"));
			//m_progressAcoustic->setVisible(false);
			//m_btnDonateAcoustic->setVisible(false);
			//label_30->setVisible(false);
			//m_btnCancelAcoustic->setVisible(false);
			m_progressAcoustic->setMaximum(100);
			m_progressAcoustic->setValue(100);
		}
		break;
		case M7IR44:
		case M7IR48:
		{
			showMessage(tr("M7 IR download complete"));
			//m_progressM7->setVisible(false);
			//m_btnDonateM7->setVisible(false);
			//label_31->setVisible(false);
			//m_btnCancelM7->setVisible(false);
			m_chk44->blockSignals(false);
			m_chk48->blockSignals(false);
			m_progressM7->setMaximum(100);
			m_progressM7->setValue(100);
		}
		break;
		case ALL: //Special case only used to hide all at startup
		{
			label_25->setVisible(false);
			m_progressSonatina->setVisible(false);
			m_btnCancelSonatina->setVisible(false);
			m_btnDonateSonatina->setVisible(false);

			label_26->setVisible(false);
			m_progressMaestro->setVisible(false);
			m_btnCancelMaestro->setVisible(false);
			m_btnDonateMaestro->setVisible(false);

			label_27->setVisible(false);
			m_progressClassic->setVisible(false);
			m_btnCancelClassic->setVisible(false);
			m_btnDonateClassic->setVisible(false);

			label_30->setVisible(false);
			m_progressAcoustic->setVisible(false);
			m_btnCancelAcoustic->setVisible(false);
			m_btnDonateAcoustic->setVisible(false);

			label_31->setVisible(false);
			m_progressM7->setVisible(false);
			m_btnCancelM7->setVisible(false);
			m_btnDonateM7->setVisible(false);

			m_progressBox->setVisible(false);
		}
		break;
	}
	updateInstalledState();
}/*}}}*/

void OOStudio::downloadCanceled(int type)/*{{{*/
{
	SamplePack pack = (SamplePack)type;
	switch(pack)
	{
		case Sonatina:
		{
			showMessage(tr("Sonatina download canceled"));
			label_25->setVisible(false);
			m_progressSonatina->setVisible(false);
			m_btnCancelSonatina->setVisible(false);
			m_btnDonateSonatina->setVisible(false);
		}
		break;
		case Maestro:
		{
			showMessage(tr("Maestro download canceled"));
			label_26->setVisible(false);
			m_progressMaestro->setVisible(false);
			m_btnCancelMaestro->setVisible(false);
			m_btnDonateMaestro->setVisible(false);
		}
		break;
		case ClassicGuitar:
		{
			showMessage(tr("Classical Guitar download canceled"));
			label_27->setVisible(false);
			m_progressClassic->setVisible(false);
			m_btnCancelClassic->setVisible(false);
			m_btnDonateClassic->setVisible(false);
		}
		break;
		case AcousticGuitar:
		{
			showMessage(tr("Acoustic Guitar download canceled"));
			label_30->setVisible(false);
			m_progressAcoustic->setVisible(false);
			m_btnCancelAcoustic->setVisible(false);
			m_btnDonateAcoustic->setVisible(false);
		}
		break;
		case M7IR44:
		case M7IR48:
		{
			showMessage(tr("M7 IR download canceled"));
			label_31->setVisible(false);
			m_progressM7->setVisible(false);
			m_btnCancelM7->setVisible(false);
			m_chk44->blockSignals(false);
			m_chk48->blockSignals(false);
			m_btnDonateM7->setVisible(false);
		}
		break;
		case ALL:
		{
			label_25->setVisible(false);
			m_progressSonatina->setVisible(false);
			m_btnCancelSonatina->setVisible(false);
			m_btnDonateSonatina->setVisible(false);

			label_26->setVisible(false);
			m_progressMaestro->setVisible(false);
			m_btnCancelMaestro->setVisible(false);
			m_btnDonateMaestro->setVisible(false);

			label_27->setVisible(false);
			m_progressClassic->setVisible(false);
			m_btnCancelClassic->setVisible(false);
			m_btnDonateClassic->setVisible(false);

			label_30->setVisible(false);
			m_progressAcoustic->setVisible(false);
			m_btnCancelAcoustic->setVisible(false);
			m_btnDonateAcoustic->setVisible(false);

			label_31->setVisible(false);
			m_progressM7->setVisible(false);
			m_btnCancelM7->setVisible(false);
			m_btnDonateM7->setVisible(false);

			m_progressBox->setVisible(false);
		}
		break;
	}
	if(m_progress.contains(type))
		m_progress.takeAt(m_progress.indexOf(type));
}/*}}}*/

void OOStudio::downloadStarted(int type)/*{{{*/
{
	m_progressBox->setVisible(true);
	m_progress.append(type);

	SamplePack pack = (SamplePack)type;
	switch(pack)
	{
		case Sonatina:
		{
			showMessage(tr("Sonatina download started"));
			label_25->setVisible(true);
			m_progressSonatina->setVisible(true);
			m_btnCancelSonatina->setVisible(true);
			m_progressSonatina->setValue(0);
			m_btnDonateSonatina->setVisible(true);
		}
		break;
		case Maestro:
		{
			showMessage(tr("Maestro download started"));
			label_26->setVisible(true);
			m_progressMaestro->setVisible(true);
			m_btnCancelMaestro->setVisible(true);
			m_progressMaestro->setValue(0);
			m_btnDonateMaestro->setVisible(true);
		}
		break;
		case ClassicGuitar:
		{
			showMessage(tr("Classical Guitar download started"));
			label_27->setVisible(true);
			m_progressClassic->setVisible(true);
			m_btnCancelClassic->setVisible(true);
			m_progressClassic->setValue(0);
			m_btnDonateClassic->setVisible(true);
		}
		break;
		case AcousticGuitar:
		{
			showMessage(tr("Acoustic Guitar download started"));
			label_30->setVisible(true);
			m_progressAcoustic->setVisible(true);
			m_btnCancelAcoustic->setVisible(true);
			m_progressAcoustic->setValue(0);
			m_btnDonateAcoustic->setVisible(true);
		}
		break;
		case M7IR44:
		case M7IR48:
		{
			showMessage(tr("M7 IR download started"));
			label_31->setVisible(true);
			m_progressM7->setVisible(true);
			m_btnCancelM7->setVisible(true);
			m_progressM7->setValue(0);
			m_btnDonateM7->setVisible(true);
		}
		break;
		case ALL:
		break;
	}
}/*}}}*/

void OOStudio::downloadsComplete()/*{{{*/
{
	//label_25->setVisible(false);
	//m_progressSonatina->setVisible(false);
	//m_btnCancelSonatina->setVisible(false);

	//label_26->setVisible(false);
	//m_progressMaestro->setVisible(false);
	//m_btnCancelMaestro->setVisible(false);

	//label_27->setVisible(false);
	//m_progressClassic->setVisible(false);
	//m_btnCancelClassic->setVisible(false);

	//label_30->setVisible(false);
	//m_progressAcoustic->setVisible(false);
	//m_btnCancelAcoustic->setVisible(false);

	//label_31->setVisible(false);
	//m_progressM7->setVisible(false);
	//m_btnCancelM7->setVisible(false);

	qDebug() << "OOStudio::downloadsComplete";
	if(!m_progress.count())
		m_progressBox->setVisible(false);
	updateInstalledState();
}/*}}}*/

void OOStudio::downloadError(int type, const QString& error)/*{{{*/
{
	QString msg(tr("Download of %1 failed: \nErrors: %2"));
	SamplePack pack = (SamplePack)type;
	switch(pack)
	{
		case Sonatina:
		{
			showMessage(QString(tr("Sonatina download Failed!! ")).append(error));
			label_25->setVisible(false);
			m_progressSonatina->setVisible(false);
			m_btnCancelSonatina->setVisible(false);
			m_btnDonateSonatina->setVisible(false);
			QMessageBox::critical(
				this,
				tr("Download Failed"),
				msg.arg("Sonatina Symphonic Orchestra").arg(error)
			);
		}
		break;
		case Maestro:
		{
			showMessage(QString(tr("Maestro download Failed!! ")).append(error));
			label_26->setVisible(false);
			m_progressMaestro->setVisible(false);
			m_btnCancelMaestro->setVisible(false);
			m_btnDonateMaestro->setVisible(false);
			QMessageBox::critical(
				this,
				tr("Download Failed"),
				msg.arg("Maestro Grand").arg(error)
			);
		}
		break;
		case ClassicGuitar:
		{
			showMessage(QString(tr("Classic download Failed!! ")).append(error));
			label_27->setVisible(false);
			m_progressClassic->setVisible(false);
			m_btnCancelClassic->setVisible(false);
			m_btnDonateClassic->setVisible(false);
			QMessageBox::critical(
				this,
				tr("Download Failed"),
				msg.arg("Classic Guitar").arg(error)
			);
		}
		break;
		case AcousticGuitar:
		{
			showMessage(QString(tr("Acoustic download Failed!! ")).append(error));
			label_30->setVisible(false);
			m_progressAcoustic->setVisible(false);
			m_btnCancelAcoustic->setVisible(false);
			m_btnDonateAcoustic->setVisible(false);
			QMessageBox::critical(
				this,
				tr("Download Failed"),
				msg.arg("Acoustic Guitar").arg(error)
			);
		}
		break;
		case M7IR44:
		case M7IR48:
		{
			showMessage(QString(tr("M7 IR download Failed!! ")).append(error));
			label_31->setVisible(false);
			m_progressM7->setVisible(false);
			m_btnCancelM7->setVisible(false);
			m_btnDonateM7->setVisible(false);
			QMessageBox::critical(
				this,
				tr("Download Failed"),
				msg.arg("Samplicity M7 IR").arg(error)
			);
			m_chk44->blockSignals(false);
			m_chk48->blockSignals(false);
		}
		break;
		case ALL:
		break;
	}
	if(m_progress.contains(type))
		m_progress.takeAt(m_progress.indexOf(type));
}/*}}}*/

void OOStudio::trackSonatinaProgress(qint64 bytesReceived, qint64 bytesTotal)/*{{{*/
{
	int max = bytesTotal;
	int val = bytesReceived;
	if(m_progressSonatina->maximum() < max)
	{
		m_progressSonatina->setMaximum(max);
		m_progressSonatina->setValue(val);
	}
	else if(val >= max)
		m_progressSonatina->setMaximum(0);
	else
		m_progressSonatina->setValue(val);
}/*}}}*/

void OOStudio::trackMaestroProgress(qint64 bytesReceived, qint64 bytesTotal)/*{{{*/
{
	int max = bytesTotal;
	int val = bytesReceived;
	if(m_progressMaestro->maximum() < max)
	{
		m_progressMaestro->setMaximum(max);
		m_progressMaestro->setValue(val);
	}
	else if(val >= max)
		m_progressMaestro->setMaximum(0);
	else
		m_progressMaestro->setValue(val);
}/*}}}*/

void OOStudio::trackClassicProgress(qint64 bytesReceived, qint64 bytesTotal)/*{{{*/
{
	int max = bytesTotal;
	int val = bytesReceived;
	if(m_progressClassic->maximum() < max)
	{
		m_progressClassic->setMaximum(max);
		m_progressClassic->setValue(val);
	}
	else if(val >= max)
		m_progressClassic->setMaximum(0);
	else
		m_progressClassic->setValue(val);
}/*}}}*/

void OOStudio::trackAcousticProgress(qint64 bytesReceived, qint64 bytesTotal)/*{{{*/
{
	int max = bytesTotal;
	int val = bytesReceived;
	if(m_progressAcoustic->maximum() < max)
	{
		m_progressAcoustic->setMaximum(max);
		m_progressAcoustic->setValue(val);
	}
	else if(val >= max)
		m_progressAcoustic->setMaximum(0);
	else
		m_progressAcoustic->setValue(val);
}/*}}}*/

void OOStudio::trackM7Progress(qint64 bytesReceived, qint64 bytesTotal)/*{{{*/
{
	int max = bytesTotal;
	int val = bytesReceived;
	if(m_progressM7->maximum() < max)
	{
		m_progressM7->setMaximum(max);
		m_progressM7->setValue(val);
	}
	else if(val >= max)
		m_progressM7->setMaximum(0);
	else
		m_progressM7->setValue(val);
}/*}}}*/

void OOStudio::donateSonatina()
{
	if(!m_downloader->isRunning(Sonatina) && !m_downloader->isExtracting(Sonatina))
	{
		cancelSonatina();
	}
	DownloadPackage* pkg = m_downloadMap.value(Sonatina);
	if(pkg)
		showExternalLinks(pkg->homepage);
}

void OOStudio::donateMaestro()
{
	if(!m_downloader->isRunning(Maestro) && !m_downloader->isExtracting(Maestro))
	{
		cancelMaestro();
	}
	DownloadPackage* pkg = m_downloadMap.value(Maestro);
	if(pkg)
		showExternalLinks(pkg->homepage);
}

void OOStudio::donateClassic()
{
	if(!m_downloader->isRunning(ClassicGuitar) && !m_downloader->isExtracting(ClassicGuitar))
	{
		cancelClassic();
	}
	DownloadPackage* pkg = m_downloadMap.value(ClassicGuitar);
	if(pkg)
		showExternalLinks(pkg->homepage);
}

void OOStudio::donateAcoustic()
{
	if(!m_downloader->isRunning(AcousticGuitar) && !m_downloader->isExtracting(AcousticGuitar))
	{
		cancelAcoustic();
	}
	DownloadPackage* pkg = m_downloadMap.value(AcousticGuitar);
	if(pkg)
		showExternalLinks(pkg->homepage);
}

void OOStudio::donateM7()
{
	if((!m_downloader->isRunning(M7IR44) && !m_downloader->isExtracting(M7IR44))
		&&(!m_downloader->isRunning(M7IR48) && !m_downloader->isExtracting(M7IR48)))
	{
		cancelM7();
	}
	DownloadPackage* pkg = m_downloadMap.value(M7IR44);
	if(pkg)
		showExternalLinks(pkg->homepage);
}
