//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOSTUDIO_H
#define OOSTUDIO_H

#include <QDialog>
#include <QSystemTrayIcon>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>
#include "ui_OOStudioBase.h"

class QCloseEvent;
class QAction;
class QMenu;
class QProcess;
class QStandardItemModel;
class QStandardItem;
class QItemSelectionModel;
class QSettings;

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
};

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
	QList<QProcess*> m_procList;
	QMap<QString, OOSession*> m_sessionMap;
	QHash<long, QString> m_procMap;

	void createTrayIcon();
	void runJack();
	void runCommads();
	void runPostCommads();
	void runOOM();
	void populateSessions();
	void processMessages(int, QString, QProcess*);
	bool validateCreate();
	OOSession* readSession(QString);
	void saveSettings();
	void loadSession(OOSession*);
	void updateHeaders();

protected:
	void closeEvent(QCloseEvent*);

private slots:
	void iconActivated(QSystemTrayIcon::ActivationReason);
	void resetCreate(bool fromClear = true);
	void loadSessionClicked();
	void loadTemplateClicked();
	void createSession();
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
	//Process Listeners
	void processJackMessages();
	void processJackErrors();
	void processLSMessages();
	void processLSErrors();
	void processOOMMessages();
	void processOOMErrors();
	void processCustomMessages();
	void processCustomErrors();
};
#endif
