//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOExtract.h"
#include <QDebug>

static const QString UNZIP_CMD("unzip");
static const QString UNRAR_CMD("unrar");

OOExtract::OOExtract()
{
	m_process = new QProcess(this);
	connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readOutput()));
	connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readError()));
	connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
}

void OOExtract::readOutput()
{
	//TODO: buffer these logs so they can make it back to the GUI
	qDebug() << "OOExtract::readOutput";
	while(m_process->canReadLine())
	{
		qDebug() << m_process->readLine();
	}
}

void OOExtract::readError()
{
	//TODO: buffer these logs so they can make it back to the GUI
	qDebug() << "OOExtract::readError";
	qDebug() << m_process->readAllStandardError();
	if(m_process->state() == QProcess::Running)
	{
		qDebug() << "Attempting to terminate process";
		m_process->terminate();
	}
}

void OOExtract::finished(int code, QProcess::ExitStatus status)
{
	qDebug() << "OOExtract::finished: code " << code << " - status: "<< status;
	emit extractComplete(code);
}

bool OOExtract::startExtract(ExtractJob* job)
{
	if(job)
	{
		QStringList params;
		QString cmd;
		m_process->setWorkingDirectory(job->basePath);
		switch(job->format)/*{{{*/
		{
			case TGZ:
			{
				params << "-x" << "-z" << "-f" << job->fileName;
				cmd = QString("tar");
			}
			break;
			case TBZ2:
			{
				params << "-x" << "-j" << "-f" << job->fileName;
				cmd = QString("tar");
			}
			break;
			case RAR:
			{
				params << "e" << "-o+" << "-p-" << job->fileName;
				cmd = QString(UNRAR_CMD);
			}
			break;
			case ZIP:
			{
				params << job->fileName;
				cmd = QString(UNZIP_CMD);
			}
			break;
		}/*}}}*/
		m_process->start(cmd, params);
		if(!m_process->waitForStarted(500000))
		{
			qDebug() << "Extraction of " << job->fileName << "failed to start";
			emit extractComplete(1);
			return  false;
		}
		return true;
	}
	return false;
}
