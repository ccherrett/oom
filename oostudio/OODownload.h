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
	QString getFilename(const QUrl &url);
	bool saveFile(QString name, QIODevice *data);

public slots:
	void startDownload(DownloadPackage*);
	void downloadComplete(QNetworkReply*);
	void trackProgress(qint64 bytesReceived, qint64 bytesTotal);
};

#endif
