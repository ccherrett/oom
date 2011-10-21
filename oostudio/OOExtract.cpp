//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOExtract.h"
#include <QDir>
#include <QDebug>

static const QString UNZIP_CMD("unzip");
static const QString UNRAR_CMD("unrar");

OOExtract::OOExtract(ExtractJob* job)
{
	m_job = job;
}

void OOExtract::readOutput()
{
	//TODO: buffer these logs so they can make it back to the GUI
	while(m_process->canReadLine())
	{
		QString message(m_process->readLine());
		message = message.trimmed().simplified();
		if(!message.isEmpty())
		{
			//qDebug() << "OOExtract::readOutput" << message;
		}
	}
}

void OOExtract::readError()
{
	//TODO: buffer these logs so they can make it back to the GUI
	QString message(m_process->readAllStandardError());
	message = message.trimmed().simplified();
	if(!message.isEmpty())
	{
		//qDebug() << "OOExtract::readError" << message;
	}
	/*if(m_process->state() == QProcess::Running)
	{
		qDebug() << "Attempting to terminate process";
		m_process->terminate();
	}*/
}

void OOExtract::finished(int code)
{
	qDebug() << "OOExtract::finished: code " << code;
	if(code)
		emit extractFailed(m_job->type, QString("Extracting Error"));
	else
		emit extractComplete(m_job->type);
	quit();
}

void OOExtract::run()
{
	m_process = new QProcess();
	connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int)));
	QStringList params;
	QString cmd;
	QDir baseDir(m_job->basePath);
	if(!baseDir.exists())
	{
		baseDir.mkpath(m_job->basePath);
	}
	m_process->setWorkingDirectory(m_job->basePath);
	switch(m_job->format)
	{
		case TGZ:
		{
			params << "-x" << "-z" << "-f" << m_job->fileName;
			cmd = QString("tar");
		}
		break;
		case TBZ2:
		{
			params << "-x" << "-j" << "-f" << m_job->fileName;
			cmd = QString("tar");
		}
		break;
		case RAR:
		{
			params << "x" << "-o+" << "-p-" << m_job->fileName;
			cmd = QString(UNRAR_CMD);
		}
		break;
		case ZIP:
		{
			params << m_job->fileName;
			cmd = QString(UNZIP_CMD);
		}
		break;
	}
	m_process->start(cmd, params);
	//m_process->waitForFinished(-1);
	exec();
	/*if(!m_process->waitForFinished(-1))
	{
		qDebug() << "Extraction of " << m_job->fileName << "failed to start";
		emit extractFailed(m_job->type, "Failed to start extraction");
	}*/
}
