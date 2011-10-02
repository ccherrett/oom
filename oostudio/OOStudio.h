//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOSTUDIO_H
#define OOSTUDIO_H

#include <lscp/client.h>
#include <lscp/device.h>
#include <lscp/socket.h>

#include <QDialog>
#include <QSystemTrayIcon>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QProcess>
#include "ui_OOStudioBase.h"

class QCloseEvent;
class QResizeEvent;
class QAction;
class QMenu;
class QStandardItemModel;
class QStandardItem;
class QItemSelectionModel;
class QModelIndex;
class QSettings;
class OOProcess;
class QSize;

struct OOSession
{
	QString path;
	QString name;
	bool istemplate;
	bool loadJack;
	bool loadls;
	QString jackcommand;
	QString lscommand;
	QStringList commands;
	QStringList postCommands;
	QString lscpPath;
	int lscpMode;
	QString songfile;
	QString lshostname;
	int lsport;
};

class OOStudio :public QDialog ,public Ui::OOStudioBase
{
	Q_OBJECT

public:
	OOStudio();
	void setVisible(bool);

private:
	bool m_incleanup;
	QAction* maximizeAction;
	QAction* minimizeAction;
	QAction* restoreAction;
	QAction* quitAction;
	QAction* importAction;
	QSystemTrayIcon* trayIcon;
	QMenu* trayMenu;
	QProcess* m_jackProcess;
	QProcess* m_lsProcess;
	QProcess* m_oomProcess;
	QStandardItemModel* m_sessionModel;
	QItemSelectionModel* m_sessionSelectModel;
	QStandardItemModel* m_templateModel;
	QItemSelectionModel* m_templateSelectModel;
	QStandardItemModel* m_commandModel;
	QStandardItemModel* m_loggerModel;
	QStringList m_sessionlabels;
	QStringList m_templatelabels;
	QStringList m_commandlabels;
	QStringList m_loglabels;
	QStringList m_lscpLabels;
	QList<QProcess*> m_procList;
	QMap<QString, OOSession*> m_sessionMap;
	QHash<long, OOProcess*> m_procMap;
	OOSession* m_current;
	QSize m_restoreSize;

	void loadStyle(QString);
	void createTrayIcon();
	void createConnections();
	void createModels();
	bool runJack(OOSession*);
	bool runLinuxsampler(OOSession*);
	void runCommands(OOSession*, bool post = false);
	bool runOOM(OOSession*);
	bool checkOOM();
	void populateSessions(bool usehash = false);
	void processMessages(int, QString, QProcess*);
	bool validateCreate();
	OOSession* readSession(QString);
	void saveSettings();
	void loadSession(OOSession*);
	void updateHeaders();
	bool pingLinuxsampler(OOSession*);
	bool loadLSCP(OOSession*);
	QString getValidName(QString);
	QString convertPath(QString);
	void doSessionDelete(OOSession*);

protected:
	void closeEvent(QCloseEvent*);
	void resizeEvent(QResizeEvent*);

private slots:
	void cleanupProcessList();
	void iconActivated(QSystemTrayIcon::ActivationReason);
	void resetCreate(bool fromClear = true);
	void loadSessionClicked();
	void loadTemplateClicked();
	void createSession();
	void updateSession();
	void browseLocation();
	void browseLSCP();
	void browseOOM();
	void browse(int);
	void addCommand();
	void deleteCommand();
	void templateSelectionChanged(int);
	void clearLogger();
	void shutdown();
	void importSession();
	void deleteTemplate();
	void deleteSession();
	bool stopCurrentSession();
	void editModeChanged(int);
	void oomRemoteShutdown();
	void sessionDoubleClicked(QModelIndex);
	void templateDoubleClicked(QModelIndex);
	//Process Listeners
	void processJackExit(int, QProcess::ExitStatus);
	void processJackExecError(QProcess::ProcessError);
	void processJackMessages();
	void processJackErrors();
	void processLSMessages();
	void processLSErrors();
	void processOOMMessages();
	void processOOMErrors();
	void processCustomMessages(QString, long);
	void processCustomErrors(QString, long);
	void processOOMExit(int, QProcess::ExitStatus);
};
#endif
