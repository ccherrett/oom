//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOTHREAD_H
#define OOTHREAD_H

#include <QThread>
#include <QHash>
#include <QStringList>
#include <QFileInfo>
#include <QUrl>
#include "OOProcess.h"
#include "OOStructures.h"

class QProcess;

class OOThread : public QThread
{
	Q_OBJECT
	QHash<long, OOProcess*> m_procMap;

public:
	enum PState{ProcessStarting, ProcessRunning, ProcessError, ProcessNotRunning, ProcessNotRequested};

protected:
	PState m_state;
	QString m_name;
	OOSession* m_session;
	QProcess* m_process;

public:
	OOThread(OOSession* s = 0);
	void run();
	bool programStarted();
	bool programError();
	PState programState();

private slots:
	void processMessages();
	void processMessagesByType(int);
	void processErrors();
	void processCustomMessages(QString, long);
	void processCustomErrors(QString, long);
	void relayExit(int, QProcess::ExitStatus);
	
signals:
	void startupSuccess();
	void startupFailed();
	void logMessage(LogInfo*);
	void processExit(int);

};

class OOMProcessThread : public OOThread
{
	Q_OBJECT

public:
	OOMProcessThread(OOSession* session);
	void run();
};

class JackProcessThread : public OOThread
{
	Q_OBJECT

	bool pingJack();

public:
	JackProcessThread(OOSession* session);
	void run();
};

class LinuxSamplerProcessThread : public OOThread
{
	Q_OBJECT
	QStringList m_lscpLabels;
	bool pingLinuxsampler();
	bool loadLSCP();
public:
	LinuxSamplerProcessThread(OOSession* session);
	void run();

};
#endif
