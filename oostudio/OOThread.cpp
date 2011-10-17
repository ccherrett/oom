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
}

void OOThread::run()/*{{{*/
{
	if(m_session)
	{
		//QStringList commands = post ? session->postCommands : session->commands;
		QStringList commands = m_session->commands;
		foreach(QString command, commands)
		{
			qDebug() << "Running command: " << command;
			QString cmd(command);
			QStringList args = cmd.split(" ");
			cmd = args.takeFirst();
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
			}
		}
	}
	exec();
}/*}}}*/

bool OOThread::startJob()
{
	return false;
}


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

