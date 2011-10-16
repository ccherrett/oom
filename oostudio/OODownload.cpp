//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OODownload.h"
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

OODownload::OODownload(QObject* parent)
: QObject(parent)
{
	connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadComplete(QNetworkReply*)));
}

void OODownload::startDownload(const QUrl &url)
{
	QNetworkRequest request(url);
	QNetworkReply *reply = m_manager.get(request);
	m_currentDownloads.append(reply);
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

void OODownload::executeBatch(QStringList list)
{
	foreach(QString str, list)
	{
		QUrl url = QUrl::fromEncoded(str.toLocal8Bit());
		startDownload(url);
	}
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
		QString filename = getFilename(url);
		if(saveFile(filename, reply))
		{
			//Add message to log
		}
	}
	m_currentDownloads.removeAll(reply);
	reply->deleteLater();
	if(m_currentDownloads.isEmpty())
	{
		//Show messages
	}
}
