//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "LSThread.h"
#include <QtGui>
#include <QDebug>
#include <lscp/client.h>
#include "config.h"
#include "gconfig.h"
#include "globals.h"

lscp_status_t lscp_client_callback ( lscp_client_t* /*_client*/, lscp_event_t /*event*/, const char * /*pchData*/, int /*cchData*/, void* /*pvData*/ )
{
	return LSCP_OK;
}

LSThread::LSThread()
: m_state(ProcessNotRunning),
  m_name("LinuxSampler")
{
	m_versionError = false;
	m_process = 0;
}

void LSThread::run()/*{{{*/
{
	m_process = new QProcess();
	connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processMessages()));
	connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processErrors()));
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(relayExit(int, QProcess::ExitStatus)));

	if(config.lsClientStartLS)
	{
		bool rv = pingLinuxsampler();
		if(debugMsg)
			qDebug("LSThread::run: Launching linuxsampler ");
		if(!rv && !m_versionError)
		{
			QString lscmd("linuxsampler");
			if(!config.lsClientLSPath.isEmpty())
				lscmd = config.lsClientLSPath;
			QStringList lsargs = lscmd.split(" ");
			lscmd = lsargs.takeFirst();
			if(lsargs.isEmpty() || !lsargs.contains("--lscp-addr"))
				lsargs << "--lscp-addr" << "127.0.0.1";
			if(lsargs.isEmpty())
			{
				m_process->start(lscmd);
			}
			else
			{
				m_process->start(lscmd, lsargs);
			}
			if(!m_process->waitForStarted())
			{
				qDebug("LSThread::run: FATAL: LinuxSampler startup FAILED");
				m_state = ProcessError;
				gSamplerStarted = false;
				emit startupFailed();
				return;
			}
			sleep(1);
			int retry = 0;
			rv = pingLinuxsampler();
			while(!rv && !m_versionError && retry < 5)
			{
				sleep(1);
				rv = pingLinuxsampler();
				++retry;
			}
		}
		else if(m_versionError)
		{//We have a 
			qDebug("LSThread::run: FATAL: Found running linuxsampler but version is unsupported");
			gSamplerStarted = false;
			emit startupFailed();
			return;
		}
		m_versionError = false;
		if(rv && debugMsg)
			qDebug("LSThread::run: LinuxSampler startup complete");
		else if(!rv)
			qDebug("LSThread::run: FATAL: LinuxSampler startup FAILED");
		if(rv)
		{
			m_state = ProcessRunning;
			emit startupSuccess();
			gSamplerStarted = true;
			exec();
			if(m_process->state() == QProcess::Running)
			{
				m_process->terminate();
				m_process->waitForFinished();
			}
			m_state = ProcessNotRunning;
		}
		else
		{
			m_process->terminate();
			m_process->waitForFinished();
			m_state = ProcessError;
			gSamplerStarted = false;
			emit startupFailed();
		}
	}
	else
	{
		m_state = ProcessNotRequested;
		gSamplerStarted = false;
		emit startupSuccess();
	}
}/*}}}*/

bool LSThread::pingLinuxsampler()/*{{{*/
{
	lscp_client_t* client = ::lscp_client_create(config.lsClientHost.toUtf8().constData(), config.lsClientPort, lscp_client_callback, NULL);
	if(client == NULL)
		return false;
	lscp_server_info_t* info = lscp_get_server_info(client);
	if(info == NULL)
	{
		return false;
	}
	else
	{
		QString version(info->version);
		qDebug("LSThread::pingLinuxsampler: Description: %s, Version: %s, Protocol Version: %s\n", info->description, info->version, info->protocol_version);
		if(version.startsWith("1.0.0.svn"))
			return true;
		else
		{
			qDebug("LSThread::pingLinuxsampler: LinuxSampler started but incorrect Version tag: %s", info->version);
			return false;
		}
	}
	return true;
}/*}}}*/

void LSThread::processMessages()/*{{{*/
{
	processMessagesByType(0);
}/*}}}*/

void LSThread::processErrors()/*{{{*/
{
	processMessagesByType(1);
}/*}}}*/

void LSThread::processMessagesByType(int type)/*{{{*/
{
	switch(type)
	{
		case 0: //Output
		{
			while(m_process->canReadLine())
			{
				QString messages = QString::fromUtf8(m_process->readLine().constData());
				messages = messages.trimmed().simplified();
				if(messages.isEmpty())
					continue;
				if(debugMsg)
					qDebug("LinuxSampler::INFO: %s", messages.toUtf8().constData());
			}
		}
		break;
		case 1: //Error
		{
			QByteArray ba = m_process->readAllStandardError();
			QTextStream in(&ba);
			QString messages = in.readLine();
			QString lastMsg("");
			while(!messages.isNull())
			{
				if(messages.isEmpty() || messages == lastMsg)
				{
					messages = in.readLine();
					continue;
				}
				qDebug("LinuxSampler::ERROR: %s", messages.toUtf8().constData());
				lastMsg = messages;
				messages = in.readLine();
			}
		}
		break;
	}
}/*}}}*/

bool LSThread::programStarted()
{
	if((m_state == LSThread::ProcessRunning) || (m_state == LSThread::ProcessNotRequested))
		return true;
	return false;
}

bool LSThread::programError()
{
	return m_state == LSThread::ProcessError;
}

LSThread::PState LSThread::programState()
{
	return  m_state;
}

void LSThread::relayExit(int code, QProcess::ExitStatus stat)
{
	Q_UNUSED(stat);
	emit processExit(code);
}


