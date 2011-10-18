//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OODownload.h"
#include "OOStructures.h"
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

OODownload::OODownload(QObject* parent)
: QObject(parent)
{
	connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
}

void OODownload::startDownload(DownloadPackage* pkg)
{
	QNetworkRequest request(pkg->path);
	QNetworkReply *reply = m_manager.get(request);
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(trackProgress(qint64, qint64)));
	m_currentDownloads.append(reply);
	m_currentPackage.append(pkg);
}

void OODownload::trackProgress(qint64 bytesReceived, qint64 bytesTotal)
{
}

QString OODownload::getFilename(const QUrl &url)
{
	QString path = url.path();
	QString filename = QFileInfo(path).fileName();

	if(filename.isEmpty())
		filename = "oom-download";

	if(QFile::exists(filename)) 
	{
		int i = 0;
		filename += '.';
		while (QFile::exists(filename + QString::number(i)))
			++i;
		filename += QString::number(i);
	}
	return filename;
}

bool OODownload::saveFile(QString name, QIODevice* data)
{
	QFile savefile(name);
	if(!savefile.open(QIODevice::WriteOnly))
	{
		QString msg(tr("Unable to save file: %1\nErrors: %2"));
		QMessageBox::critical(
			0,
			tr("File Save Error"),
			msg.arg(name).arg(savefile.errorString())
		);
		return false;
	}
	savefile.write(data->readAll());
	savefile.close();
	return true;
}

void OODownload::downloadComplete(QNetworkReply* reply)
{
	QUrl url = reply->url();
	if(reply->error())
	{
		QString msg(tr("Download of %1 failed: \nErrors: %2"));
		QMessageBox::critical(
			0,
			tr("Download Failed"),
			msg.arg(url.toEncoded().constData()).arg(reply->errorString())
		);
	}
	else
	{
		int index = m_currentDownloads.indexOf(reply);
		DownloadPackage* pkg = m_currentPackages.takeAt(index);
		if(pkg)
		{
			QString filename = pkg->filename; //getFilename(url);
			if(saveFile(filename, reply))
			{
				//Add message to log
				//TODO: fire extract thread
			}
		}
	}
	m_currentDownloads.removeAll(reply);
	reply->deleteLater();
	if(m_currentDownloads.isEmpty())
	{
		//TODO: Show messages no more downloads
	}
}
