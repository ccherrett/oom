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
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QProcess>
#include <QQueue>
#include "ui_OOStudioBase.h"

class QCloseEvent;
class QResizeEvent;
class QAction;
class QMenu;
class QUrl;
class QStandardItemModel;
class QStandardItem;
class QItemSelectionModel;
class QModelIndex;
class QSettings;
class OOProcess;
class QByteArray;
class QSize;
class QTimer;
class OOThread;
class OOMProcessThread;
class JackProcessThread;
class LinuxSamplerProcessThread;

class OOSession;
class LogInfo;


class OOStudio :public QMainWindow ,public Ui::OOStudioBase
{
	Q_OBJECT

public:
	OOStudio();

private:
	bool m_incleanup;
	bool m_loading;
	
	QAction* m_quitAction;
	QSystemTrayIcon* m_trayIcon;
	QMenu* m_trayMenu;
	
	OOThread* m_customThread;
	OOMProcessThread* m_oomThread;
	JackProcessThread* m_jackThread;
	LinuxSamplerProcessThread* m_lsThread;

	QStandardItemModel* m_sessionModel;
	QItemSelectionModel* m_sessionSelectModel;
	QItemSelectionModel* m_commandSelectModel;
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
	QByteArray m_restoreSize;
	QQueue<LogInfo*> m_logQueue;
	QTimer* m_heartBeat;

	void loadStyle(QString);
	void createTrayIcon();
	void createConnections();
	void createModels();
	void populateSessions(bool usehash = false);
	void processMessages(int, QString, QProcess*);
	bool validateCreate();
	OOSession* readSession(QString);
	void saveSettings();
	void loadSession(OOSession*);
	void updateHeaders();
	QString getValidName(QString);
	QString convertPath(QString);
	void doSessionDelete(OOSession*);
	void showMessage(QString);

protected:
	void closeEvent(QCloseEvent*);
	void resizeEvent(QResizeEvent*);

private slots:
	void linuxSamplerStarted();
	void oomStarted();
	void jackStarted();
	void customStarted();
	
	void linuxSamplerFailed();
	void oomFailed();
	void jackFailed();
	void customFailed();

	void writeLog(LogInfo*);
	void toggleAdvanced(bool);
	void updateInstalledState();
	void heartBeat();
	void showExternalLinks(const QUrl &url);
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
	void sessionDoubleClicked(QModelIndex);
	void templateDoubleClicked(QModelIndex);
	void currentTopTabChanged(int);
	//Process Listeners
	void processOOMExit(int);
};
#endif
