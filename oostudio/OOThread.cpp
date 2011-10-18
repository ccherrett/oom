//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOThread.h"
#include <QProcess>

OOThread::OOThread(OOSession* session, QObject* parent)
: QThread(parent),
  m_state(ProcessNotRunning),
  m_name("Custom"),
  m_session(session)
{
	m_process = new QProcess(this);
	connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processMessages()));
	connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processErrors()));
	connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SIGNAL(processExit(int, QProcess::ExitStatus)));
}

void OOThread::run()/*{{{*/
{
	if(m_session)
	{
		//QStringList commands = post ? session->postCommands : session->commands;
		QStringList commands = m_session->commands;
		bool fired = false;
		foreach(QString command, commands)
		{
			fired = true;
			qDebug() << "Running command: " << command;
			QString cmd(command);
			QStringList args = cmd.split(" ");
			cmd = args.takeFirst();
			QFileInfo info(cmd);
			OOProcess* process = new OOProcess(cmd, this);
			connect(process, SIGNAL(readyReadStandardOutput(QString, long)), this, SLOT(processCustomMessages(QString, long)));
			connect(process, SIGNAL(readyReadStandardError(QString, long)), this, SLOT(processCustomErrors(QString, long)));
			if(args.isEmpty())
			{
				process->start(cmd);
			}
			else
			{
				process->start(cmd, args);
			}
			if(process->waitForStarted())
			{
				long pid = process->pid();
				m_procMap[pid] = process;
				m_state = ProcessRunning;
			}
			else
			{
				m_state = ProcessError;
				emit startupFalied();
			}
		}
		if(fired)
		{
			m_state = ProcessRunning;
			emit startupSuccess();
			exec();
		}
		else
		{
			emit startupSuccess();
		}
	}
}/*}}}*/

void OOStudio::processCustomMessages(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		OOProcess* process = m_procMap.value(pid);
		if(process)
		{
			while(process->canReadLine())
			{
				QString messages = QString::fromUtf8(process->readLine().constData());
				messages = messages.trimmed().simplified();
				if(messages.isEmpty())
					continue;
				LogInfo* info = new LogInfo;
				info->name = name;
				info->message = messages;
				info->codeString = QString("INFO");
				emit logMessage(info);
			}
		}
	}
}/*}}}*/

void OOStudio::processCustomErrors(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		OOProcess* process = m_procMap.value(pid);
		if(process)
		{
			QString messages = QString::fromUtf8(process->readAllStandardError().constData());
			if(messages.isEmpty())
				return;
			LogInfo* info = new LogInfo;
			info->name = name;
			info->message = messages;
			info->codeString = QString("ERROR");
			emit logMessage(info);
		}
	}
}/*}}}*/

void OOThread::processMessages()/*{{{*/
{
	processMessages(0);
}/*}}}*/

void OOThread::processErrors()/*{{{*/
{
	processMessages(1);
}/*}}}*/

void OOThread::processMessagesByType(int type)/*{{{*/
{
	if(m_process)
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
					LogInfo* info = new LogInfo;
					info->name = name;
					info->message = messages;
					info->codeString = QString("INFO");
					emit logMessage(info);
				}
			}
			break;
			case 1: //Error
			{
				QString messages = QString::fromUtf8(m_process->readAllStandardError().constData());
				if(messages.isEmpty())
					return;
				LogInfo* info = new LogInfo;
				info->name = m_name;
				info->message = messages;
				info->codeString = QString("ERROR");
				emit logMessage(info);
			}
			break;
		}
	}
}/*}}}*/

bool OOThread::programStarted()
{
	return m_state == ProcessRunning;
}

bool OOThread::programError()
{
	return m_state == ProcessError;
}

PState OOThread::programState()
{
	return  m_state;
}

OOMProcessThread::OOMProcessThread(OOSession* session, QObject* parent)
: OOThread(session, parent),
  m_state(ProcessNotRunning),
  m_name("OOMidi"),
{
}


void OOMProcessThread::run()/*{{{*/
{
	if(m_session)
	{
		showMessage("Loading OOMidi...");
		printf("Loading OOMidi ");
		m_oomProcess = new QProcess(this);
		connect(m_oomProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOOMMessages()));
		connect(m_oomProcess, SIGNAL(readyReadStandardError()), this, SLOT(processOOMErrors()));
		connect(m_oomProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processOOMExit(int, QProcess::ExitStatus)));
		QStringList args;
		args << session->songfile;
		m_oomProcess->start(QString(OOM_INSTALL_BIN), args);
		bool rv = m_oomProcess->waitForStarted(50000);
		printf("%s\n", rv ? "Complete" : "FAILED");
		//showMessage(rv ? "Loading OOMidi Complete" : "Loading OOMidi FAILED");
		//m_oomRunning = rv;
		if(rv)
		{
			m_state = ProcessRunning;
			emit startupSuccess();
			exec();
		}
		else
		{
			m_state = ProcessError;
			emit startupFailed();
		}
	}
}/*}}}*/
