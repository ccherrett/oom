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
class OODownload;
class DownloadPackage;

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

	OODownload* m_downloader;

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

	QString m_sessionTemplate;
	
	QMap<QString, OOSession*> m_sessionMap;
	QHash<int, DownloadPackage*> m_downloadMap;
	QList<int> m_progress;
	
	OOSession* m_current;
	QByteArray m_restoreSize;
	
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
	bool checkPackageInstall(int);

protected:
	void closeEvent(QCloseEvent*);
	void resizeEvent(QResizeEvent*);

private slots:
	void showMessage(QString);
	void linuxSamplerStarted();
	void oomStarted();
	void jackStarted();
	void customStarted();
	
	void linuxSamplerFailed();
	void oomFailed();
	void jackFailed();
	void customFailed();

	void downloadSonatina();
	void downloadMaestro();
	void downloadClassic();
	void downloadAcoustic();
	void downloadM7();
	void downloadAll();
	void download(int);
	void downloadEnded(int);
	void downloadCanceled(int);
	void downloadsComplete();
	void downloadError(int, const QString&);
	void downloadStarted(int);
	void populateDownloadMap();

	void cancelSonatina();
	void cancelMaestro();
	void cancelClassic();
	void cancelAcoustic();
	void cancelM7();

	void donateSonatina();
	void donateMaestro();
	void donateClassic();
	void donateAcoustic();
	void donateM7();

	void trackSonatinaProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackMaestroProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackClassicProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackAcousticProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackM7Progress(qint64 bytesReceived, qint64 bytesTotal);

	void setDefaultJackCommand();
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
	void currentTabChanged(int);
	//Process Listeners
	void processOOMExit(int);

signals:
	void cancelDownload(int);
};
#endif
