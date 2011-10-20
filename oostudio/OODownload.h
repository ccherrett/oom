//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OODOWNLOAD_H
#define OODOWNLOAD_H

#include <QObject>
#include <QList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QIODevice>
#include <QStringList>

class DownloadPackage;

class OODownload : public QObject
{
	Q_OBJECT

	QNetworkAccessManager m_manager;
	QList<QNetworkReply*> m_currentDownloads;
	QList<DownloadPackage*> m_currentPackages;

public:
	OODownload(QObject* parent = 0);
	QString getFilename(const QString &url);
	bool saveFile(DownloadPackage* pkg, QIODevice *data);
	bool isRunning(int);

public slots:
	void startDownload(DownloadPackage*);
	void downloadFinished(QNetworkReply*);
	void trackProgress(qint64 bytesReceived, qint64 bytesTotal);

signals:
	void downloadStarted(int);
	void downloadEnded(int);
	void downloadsComplete();
	void downloadError(int, QString);
	void fileSaveError(int, const QString&);
	void trackSonatinaProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackMaestroProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackClassicProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackAcousticProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackM7Progress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif
