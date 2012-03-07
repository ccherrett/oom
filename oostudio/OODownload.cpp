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
#include <QThreadPool>
#include <QDebug>
#include "OOStructures.h"
#include "OOExtract.h"
#include "config.h"

OODownload::OODownload(QObject* parent)
: QObject(parent)
{
	connect(&m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
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
	emit displayMessage(QString("Fetching library from url: ").append(pkg->path.toString()));
	m_currentDownloads[pkg->type] = reply;
	m_currentPackages[pkg->type] = pkg;
	m_redirectUrl[pkg->type] = pkg->path;
	emit downloadStarted(pkg->type);
}

void OODownload::trackProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	Q_UNUSED(bytesReceived);
	Q_UNUSED(bytesTotal);
}

QUrl OODownload::detectRedirect(const QUrl& possibleRedir, const QUrl& oldUrl) const
{
	QUrl redirUrl;
	if(!possibleRedir.isEmpty() && possibleRedir != oldUrl)
	{
		redirUrl = possibleRedir;
	}
	return redirUrl;
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

bool OODownload::processDownload(DownloadPackage* pkg, QIODevice* data)
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
		ExtractJob* job = new ExtractJob;
		job->basePath = pkg->install_path.absoluteFilePath();
		job->fileName = temp;
		job->format = (ExtractFormat)pkg->format;
		job->type = pkg->type;
		m_currentJobs[pkg->type] = job;
		OOExtract* extract = new OOExtract(job);
		connect(extract, SIGNAL(extractComplete(int)), this, SLOT(extractComplete(int)));
		connect(extract, SIGNAL(extractFailed(int, QString)), this, SLOT(extractFailed(int, QString)));
		extract->start();
		//QThreadPool::globalInstance()->start(extract);
		return true;
	}
	savefile.close();
	return false;
}

void OODownload::downloadFinished(QNetworkReply* reply)
{
	int index = m_currentDownloads.key(reply);
	DownloadPackage* pkg = m_currentPackages.value(index);
	int id = pkg->type;
	QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	QUrl redirUrl = detectRedirect(possibleRedirectUrl.toUrl(), m_redirectUrl.value(id));
	if(!redirUrl.isEmpty())
	{//Try again to follow the redirect
		QNetworkRequest request(redirUrl);
		request.setRawHeader("User-Agent", "OOStudio "VERSION);
		QNetworkReply *reply = m_manager.get(request);/*{{{*/
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
		}/*}}}*/
		m_currentDownloads[pkg->type] = reply;
		m_redirectUrl[pkg->type] = redirUrl;
		emit displayMessage(QString("Following redirect url: ").append(redirUrl.toString()));
	}
	else
	{//Complete the process
		QUrl url = reply->url();
		if(reply->error())
		{
			qDebug() << "OODownload::downloadFinished Download finished with error: " << pkg->name;
			if(reply->error() == QNetworkReply::OperationCanceledError)
			{
				emit downloadCanceled(id);
				cleanup(id);
				if(m_currentDownloads.isEmpty())
				{
					emit downloadsComplete();
				}
			}
			else
			{
				emit downloadError(id, QString(reply->errorString()));
				cleanup(id);
				if(m_currentDownloads.isEmpty())
				{
					emit downloadsComplete();
				}
			}
		}
		else
		{
			if(pkg)
			{
				//QString filename = pkg->filename; //getFilename(url);
				if(!processDownload(pkg, reply))
				{
					cleanup(pkg->type);
					emit downloadError(pkg->type, QString(reply->errorString()));
					qDebug() << "OODownload::downloadFinished Download finished: " << pkg->name;
					//Add message to log
					//TODO: fire extract thread
					//emit downloadEnded(pkg->type);
				}
			}
		}
	}
	/*m_currentDownloads.remove(index);
	reply->deleteLater();
	if(m_currentDownloads.isEmpty())
	{
		emit downloadsComplete();
	}*/
}

void OODownload::cancelDownload(int id)
{
	if(isRunning(id) && !isExtracting(id))
	{
		QNetworkReply* reply = m_currentDownloads.value(id);
		if(reply)
		{
			reply->abort();
		}
	}
}

bool OODownload::isExtracting(int id)
{
	return m_currentJobs.contains(id);
}

bool OODownload::isRunning(int id)
{
	return m_currentPackages.contains(id);
}

void OODownload::cleanup(int id)
{
	DownloadPackage* pkg = m_currentPackages.take(id);
	Q_UNUSED(pkg);
	QNetworkReply* reply = m_currentDownloads.take(id);
	if(m_currentJobs.contains(id))
	{
		ExtractJob* job = m_currentJobs.take(id);
		if(job)
		{
			QFile archive(job->fileName);
			if(archive.exists())
			{
				archive.remove();
			}
		}
	}
	reply->deleteLater();
}

void OODownload::extractComplete(int id)
{
	DownloadPackage* pkg = m_currentPackages.value(id);
	if(id == ClassicGuitar)
	{
		QString sfz(SHAREDIR);
		sfz = sfz.append(QDir::separator()).append("templates").append(QDir::separator()).append("Classical_Guitar.sfz");
		QString dest(pkg->install_path.absoluteFilePath());
		dest = dest.append(QDir::separator()).append("ClassicFREE").append(QDir::separator()).append("ClassicGuitar Samples").append(QDir::separator()).append("Classical_Guitar.sfz");
		QFile::copy(sfz, dest);
	}
	else if(id == AcousticGuitar)
	{
		QString sfz(SHAREDIR);
		sfz = sfz.append(QDir::separator()).append("templates").append(QDir::separator()).append("AcousticGuitar.sfz");
		QString dest(pkg->install_path.absoluteFilePath());
		dest = dest.append(QDir::separator()).append("AcousticGuitarFREE").append(QDir::separator()).append("AcousticGuitarFREE Samples").append(QDir::separator()).append("AcousticGuitar.sfz");
		QFile::copy(sfz, dest);
	}
	cleanup(id);

	emit downloadEnded(id);
	if(m_currentDownloads.isEmpty())
	{
		emit downloadsComplete();
	}
}

void OODownload::extractFailed(int id, const QString& msg)
{
	cleanup(id);

	emit downloadError(id, msg);
	if(m_currentDownloads.isEmpty())
	{
		emit downloadsComplete();
	}
}
