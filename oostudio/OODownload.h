//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OODOWNLOAD_H
#define OODOWNLOAD_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QIODevice>
#include <QStringList>

class ExtractJob;
class DownloadPackage;

class OODownload : public QObject
{
	Q_OBJECT

	QNetworkAccessManager m_manager;
	//QList<QNetworkReply*> m_currentDownloads;
	QMap<int, ExtractJob*> m_currentJobs;
	QMap<int, QNetworkReply*> m_currentDownloads;
	QMap<int, DownloadPackage*> m_currentPackages;
	QMap<int, QUrl> m_redirectUrl;
	void cleanup(int);
	QUrl detectRedirect(const QUrl&, const QUrl&) const;

public:
	OODownload(QObject* parent = 0);
	QString getFilename(const QString &url);
	bool processDownload(DownloadPackage* pkg, QIODevice *data);
	bool isRunning(int);
	bool isExtracting(int);

public slots:
	void startDownload(DownloadPackage*);
	void downloadFinished(QNetworkReply*);
	void trackProgress(qint64 bytesReceived, qint64 bytesTotal);
	void cancelDownload(int);
	void extractComplete(int);
	void extractFailed(int, const QString&);

signals:
	void downloadStarted(int);
	void downloadEnded(int);
	void downloadCanceled(int);
	void downloadsComplete();
	void downloadError(int, QString);
	void fileSaveError(int, const QString&);
	void trackSonatinaProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackMaestroProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackClassicProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackAcousticProgress(qint64 bytesReceived, qint64 bytesTotal);
	void trackM7Progress(qint64 bytesReceived, qint64 bytesTotal);
	void displayMessage(QString);
};

#endif
