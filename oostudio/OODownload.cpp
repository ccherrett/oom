//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OODownload.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include "OOStructures.h"
#include "config.h"

OODownload::OODownload(QObject* parent)
: QObject(parent)
{
	connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

void OODownload::startDownload(DownloadPackage* pkg)
{
	QNetworkRequest request(pkg->path);
	request.setRawHeader("User-Agent", "OOStudio "VERSION);
	QNetworkReply *reply = m_manager.get(request);
	switch(pkg->type)
	{
		case Sonatina:
			connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(trackSonatinaProgress(qint64, qint64)));
		break;
		case Maestro:
			connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(trackMaestroProgress(qint64, qint64)));
		break;
		case ClassicGuitar:
			connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(trackClassicProgress(qint64, qint64)));
		break;
		case AcousticGuitar:
			connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(trackAcousticProgress(qint64, qint64)));
		break;
		case M7IR44:
		case M7IR48:
			connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(trackM7Progress(qint64, qint64)));
		break;
		default:
			qDebug() << "OODownload::startDownload Unknown progress";
		break;
	}
	m_currentDownloads.append(reply);
	m_currentPackages.append(pkg);
	emit downloadStarted(pkg->type);
}

void OODownload::trackProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	Q_UNUSED(bytesReceived);
	Q_UNUSED(bytesTotal);
}

QString OODownload::getFilename(const QString &name)
{
	QString filename(name);
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

bool OODownload::saveFile(DownloadPackage* pkg, QIODevice* data)
{
	QString temp = QDir::tempPath();
	temp = temp.append(QDir::separator()).append(pkg->filename);
	temp = getFilename(temp);
	QFile savefile(temp);
	if(!savefile.open(QIODevice::WriteOnly))
	{
		qDebug() << "Failed to open file for writing: " << temp ;
		emit downloadError(pkg->type, QString(savefile.errorString()));
		return false;
	}
	if(!savefile.write(data->readAll()))
	{
		qDebug() << "Failed to write file: " << temp ;
		emit downloadError(pkg->type, QString(savefile.errorString()));
	}
	else
	{
		savefile.close();
		//TODO: extract package and put it inplace
		return true;
	}
	savefile.close();
	return false;
}

void OODownload::downloadFinished(QNetworkReply* reply)
{
	QUrl url = reply->url();
	int index = m_currentDownloads.indexOf(reply);
	DownloadPackage* pkg = m_currentPackages.takeAt(index);
	if(reply->error())
	{
		qDebug() << "OODownload::downloadFinished Download finished with error: " << pkg->name;
		emit downloadError(pkg->type, QString(reply->errorString()));
	}
	else
	{
		if(pkg)
		{
			//QString filename = pkg->filename; //getFilename(url);
			if(saveFile(pkg, reply))
			{
				qDebug() << "OODownload::downloadFinished Download finished: " << pkg->name;
				//Add message to log
				//TODO: fire extract thread
				emit downloadEnded(pkg->type);
			}
		}
	}
	m_currentDownloads.removeAll(reply);
	reply->deleteLater();
	if(m_currentDownloads.isEmpty())
	{
		emit downloadsComplete();
	}
}

bool OODownload::isRunning(int id)
{
	SamplePack pack = (SamplePack)id;
	foreach(DownloadPackage* pkg, m_currentPackages)
	{
		if(pkg->type == pack)
		{
			return true;
		}
	}
	return false;
}
