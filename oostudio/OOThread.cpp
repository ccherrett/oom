//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "OOThread.h"
#include "OOClient.h"
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <jack/jack.h>
#include <lscp/client.h>
#include "config.h"

lscp_status_t lscp_client_callback ( lscp_client_t* /*_client*/, lscp_event_t /*event*/, const char * /*pchData*/, int /*cchData*/, void* /*pvData*/ )
{
	return LSCP_OK;
}

static const char* MAP_STRING = "MAP MIDI_INSTRUMENT";
static const char* SOUND_PATH = "@@OOM_SOUNDS@@";
static const QString SOUNDS_DIR = QString(QDir::homePath()).append(QDir::separator()).append(".sounds");

OOThread::OOThread(OOSession* session)
: m_state(ProcessNotRunning),
  m_name("Custom"),
  m_session(session)
{
	m_process = 0;
}

void OOThread::run()/*{{{*/
{
	if(m_session)
	{
		m_process = new QProcess();
		connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processMessages()));
		connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processErrors()));
		qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
		connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SIGNAL(processExit(int)));

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
			OOProcess* process = new OOProcess(info.fileName());
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
				emit startupFailed();
			}
		}
		if(fired)
		{
			m_state = ProcessRunning;
			emit startupSuccess();
			exec();
			QList<long> keys = m_procMap.keys();
			foreach(long key, keys)
			{
				OOProcess* p = m_procMap.take(key);
				if(p)
				{
					p->terminate();
					p->waitForFinished();
					delete p;
				}
			}
		}
		else
		{
			m_state = ProcessNotRequested;
			emit startupSuccess();
		}
	}
}/*}}}*/

void OOThread::processCustomMessages(QString name, long pid)/*{{{*/
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

void OOThread::processCustomErrors(QString name, long pid)/*{{{*/
{
	if(!m_procMap.isEmpty() && m_procMap.contains(pid))
	{
		OOProcess* process = m_procMap.value(pid);
		if(process)
		{
			QByteArray ba = process->readAllStandardError();
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
				LogInfo* info = new LogInfo;
				info->name = name;
				info->message = messages;
				info->codeString = QString("ERROR");
				emit logMessage(info);
				lastMsg = messages;
				messages = in.readLine();
			}
		}
	}
}/*}}}*/

void OOThread::processMessages()/*{{{*/
{
	processMessagesByType(0);
}/*}}}*/

void OOThread::processErrors()/*{{{*/
{
	processMessagesByType(1);
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
					info->name = m_name;
					info->message = messages;
					info->codeString = QString("INFO");
					emit logMessage(info);
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
					LogInfo* info = new LogInfo;
					info->name = m_name;
					info->message = messages;
					info->codeString = QString("ERROR");
					emit logMessage(info);
					lastMsg = messages;
					messages = in.readLine();
				}
			}
			break;
		}
	}
}/*}}}*/

bool OOThread::programStarted()
{
	if((m_state == OOThread::ProcessRunning) || (m_state == OOThread::ProcessNotRequested))
		return true;
	return false;
}

bool OOThread::programError()
{
	return m_state == OOThread::ProcessError;
}

OOThread::PState OOThread::programState()
{
	return  m_state;
}

void OOThread::relayExit(int code, QProcess::ExitStatus stat)
{
	Q_UNUSED(stat);
	emit processExit(code);
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/**
 * OOMProcessThread class for starting our jack process
 * @param session The session to take our values from
 */
OOMProcessThread::OOMProcessThread(OOSession* session)
: OOThread(session)
{
	m_name = QString("OOMidi");
}

void OOMProcessThread::run()/*{{{*/
{
	if(m_session)
	{
		m_process = new QProcess();
		connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processMessages()));
		connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processErrors()));
		qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
		connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(relayExit(int, QProcess::ExitStatus)));

		//showMessage("Loading OOMidi...");
		printf("Loading OOMidi ");
		QStringList args;
		args << m_session->songfile;
		m_process->start(QString(OOM_INSTALL_BIN), args);
		bool rv = m_process->waitForStarted(50000);
		printf("%s\n", rv ? "Complete" : "FAILED");
		//showMessage(rv ? "Loading OOMidi Complete" : "Loading OOMidi FAILED");
		if(rv)
		{
			m_state = ProcessRunning;
			emit startupSuccess();
			exec();
			if(m_process->state() == QProcess::Running)
			{
				OOClient* oomclient = new OOClient("127.0.0.1", 8415);
				oomclient->callShutdown();
				m_process->waitForFinished(120000);
			}
			m_state =  ProcessNotRunning;
		}
		else
		{
			m_process->terminate();
			m_process->waitForFinished();
			m_state = ProcessError;
			emit startupFailed();
		}
	}
}/*}}}*/


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


/**
 * JackProcessThread class for starting our jack process
 * @param session The session to take our values from
 */
JackProcessThread::JackProcessThread(OOSession* session)
: OOThread(session)
{
	m_name = QString("Jackd");
}

void JackProcessThread::run()/*{{{*/
{
	if(m_session)
	{
		m_process = new QProcess();
		connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processMessages()));
		connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processErrors()));
		qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
		connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(relayExit(int, QProcess::ExitStatus)));

		if(m_session->loadJack)
		{
			//showMessage("Launching jackd");
			printf("Launching jackd: ");
			QString jackCmd = m_session->jackcommand;
			QStringList args = jackCmd.split(" ");
			jackCmd = args.takeFirst();
			if(args.isEmpty())
				m_process->start(jackCmd);
			else
				m_process->start(jackCmd, args);
			m_process->waitForStarted(50000);
			bool rv = pingJack();
			printf("%s\n", rv ? " Complete" : " FAILED");
			//showMessage(rv ? "Launching jackd Complete" : "Launching jackd FAILED");
			//showMessage("Launching jackd");
			if(rv)
			{
				m_state = ProcessRunning;
				emit startupSuccess();
				exec();
				if(m_process->state() == QProcess::Running)
				{
					m_process->kill();
					m_process->waitForFinished();
				}
				m_state = ProcessNotRunning;
			}
			else
			{
				m_process->kill();
				m_process->waitForFinished();
				m_state = ProcessError;
				emit startupFailed();
			}
		}
		else
		{
			m_state = ProcessNotRequested;
			emit startupSuccess();
		}
	}
}/*}}}*/

bool JackProcessThread::pingJack()/*{{{*/
{
	bool rv = true;
	jack_status_t status;
	jack_client_t *client;
	int retry = 0;
	sleep(1);
	while((client = jack_client_open("OOStudio", JackNoStartServer, &status, NULL)) == 0 && retry < 3)
	{
		rv = false;
		++retry;
		sleep(1);
	}
	
	if(client != 0)
	{
		switch(status)
		{
			case JackServerStarted:
				printf(" Server Startup");
				rv = true;
			break;
			case JackFailure:
				printf(" General Failure");
				rv = false;
			break;
			case JackServerError:
				printf(" Server Error");
				rv = false;
			case JackClientZombie:
			break;
			case JackServerFailed:
				printf(" Server Failed");
				rv = false;
			break;
			case JackNoSuchClient:
				printf(" No Such Client");
				rv = false;
			break;
			case JackInitFailure:
				printf(" Init Failure");
				rv = false;
			break;
			case JackNameNotUnique:
				printf(" Name Not Unique");
				rv = false;
			break;
			case JackInvalidOption:
				printf(" Invalid Option");
				rv = false;
			break;
			case JackLoadFailure:
				printf(" Load Failure");
				rv = false;
			break;
			case JackVersionError:
				printf(" Version Error");
				rv = false;
			break;
			case JackBackendError:
				printf(" Backend Error");
				rv = false;
			break;
			default:
				rv = true;
			break;
		}
		//printf(" %d", status);
		jack_client_close(client);
	}
	else
	{
		printf("jack server not running?");
	}
	client = NULL;
	return rv;
}/*}}}*/


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


/**
 * LinuxSamplerProcessThread class for starting our jack process
 * @param session The session to take our values from
 */
LinuxSamplerProcessThread::LinuxSamplerProcessThread(OOSession* session)
: OOThread(session)
{
	m_name = QString("LinuxSampler");
	m_lscpLabels = (QStringList() << "Default" << "ON_DEMAND" << "ON_DEMAND_HOLD" << "PERSISTENT");
}

void LinuxSamplerProcessThread::run()/*{{{*/
{
	if(m_session)
	{
		m_process = new QProcess();
		connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processMessages()));
		connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(processErrors()));
		qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
		connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(relayExit(int, QProcess::ExitStatus)));

		if(m_session->loadls)
		{
			//showMessage("Launching linuxsampler... ");
			printf("Launching linuxsampler ");
			QString lscmd = m_session->lscommand;
			QStringList lsargs = lscmd.split(" ");
			lscmd = lsargs.takeFirst();
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
				//showMessage("LinuxSampler startup FAILED");
				printf("FAILED\n");
				m_state = ProcessError;
				emit startupFailed();
				return;
			}
			sleep(1);
			int retry = 0;
			bool rv = pingLinuxsampler();
			while(!rv && retry < 5)
			{
				sleep(1);
				rv = pingLinuxsampler();
				++retry;
			}
			printf("%s\n", rv ? "Complete" : "FAILED");
			//showMessage(rv ? "Launching linuxsampler Complete" : "Launching linuxsampler FAILED");
			if(rv)
			{
				retry = 0;
				rv = loadLSCP();
				//while(!loadLSCP() && retry < 6)
				while(!rv && retry < 5)
				{
					sleep(1);
					rv = loadLSCP();
					++retry;
				}
				if(rv)
				{
					m_state = ProcessRunning;
					emit startupSuccess();
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
					emit startupFailed();
				}
			}
			else
			{
				m_process->terminate();
				m_process->waitForFinished();
				m_state = ProcessError;
				emit startupFailed();
			}
		}
		else
		{
			m_state = ProcessNotRequested;
			emit startupSuccess();
		}
	}
}/*}}}*/

bool LinuxSamplerProcessThread::pingLinuxsampler()/*{{{*/
{
	if(m_session)
	{
		lscp_client_t* client = ::lscp_client_create(m_session->lshostname.toUtf8().constData(), m_session->lsport, lscp_client_callback, NULL);
		if(client == NULL)
			return false;
		//printf("Query LS info: ");
		lscp_server_info_t* info = lscp_get_server_info(client);
		if(info == NULL)
		{
			//printf("FAILED!!\n");
			return false;
		}
		else
		{
			//printf("Description: %s, Version: %s, Protocol Version: %s\n", info->description, info->version, info->protocol_version);
			return true;
		}
	}
	return true;
}/*}}}*/

bool LinuxSamplerProcessThread::loadLSCP()/*{{{*/
{
	if(m_session)
	{
		QString audioStr("SET AUDIO_OUTPUT_CHANNEL_PARAMETER");
		QString startStr("SET MIDI_INPUT_PORT_PARAMETER");
		QString endStr("JACK_BINDINGS=NONE");
		//showMessage("Loading LSCP File into linuxsampler engine");
		printf("Loading LSCP ");
		lscp_client_t* client = ::lscp_client_create(m_session->lshostname.toUtf8().constData(), m_session->lsport, lscp_client_callback, NULL);
		if(client == NULL)
			return false;
		QFile lsfile(m_session->lscpPath);
		if(!lsfile.open(QIODevice::ReadOnly))
		{
			//TODO:We need to error here
			lscp_client_destroy(client);
			printf("FAILED to open file\n");
			return false;
		}
		QTextStream in(&lsfile);
		QString command = in.readLine();
		bool skip = false;
		while(!command.isNull())
		{
			if((command.startsWith(audioStr) && command.endsWith(endStr)) || (command.startsWith(audioStr) && command.endsWith(endStr)))
			{
				skip = true;
			}
			if(!command.startsWith("#") && !command.isEmpty() && !skip)
			{
				QString cmd = command.append("\r\n");
				if(m_session->lscpMode && cmd.startsWith(QString(MAP_STRING)))
				{
					QString rep(m_lscpLabels.at(m_session->lscpMode));
					int count = 0;
					foreach(QString str, m_lscpLabels)
					{
						if(count && str != rep)
							cmd.replace(str, rep);
						++count;
					}
				}
				if(cmd.contains(QString(SOUND_PATH)))
				{
					cmd = cmd.replace(QString(SOUND_PATH), SOUNDS_DIR);
				}
				//::lscp_client_query(client, cmd.toUtf8().constData());
				//return true;
				if(::lscp_client_query(client, cmd.toUtf8().constData()) != LSCP_OK)
				{
					//Try once more to run the same command
					sleep(1);
					if(::lscp_client_query(client, cmd.toUtf8().constData()) != LSCP_OK)
					{
						lsfile.close();
						lscp_client_destroy(client);
						printf("FAILED to perform query\n");
						printf("%s\n", command.toUtf8().constData());
						return false;
						QString error("FAILED to perform query: ");
						error.append(command);
						LogInfo* info = new LogInfo;
						info->name = "OOStudio";
						info->message = error;
						info->codeString = QString("ERROR");
						emit logMessage(info);
					}
				}
			}
			command = in.readLine();
			skip = false;
		}
		lsfile.close();
		lscp_client_destroy(client);
		printf("Complete\n");
		return true;
	}
	return false;
}/*}}}*/


