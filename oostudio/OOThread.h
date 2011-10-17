//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOTHREAD_H
#define OOTHREAD_H

#include <QThread>
#include <QHash>

class QProcess;
struct LogInfo;

class OOThread : public QThread
{
	Q_OBJECT
	QHash<long, OOProcess*> m_procMap;

public:
	OOThread(OOSession* s = 0, QObject* parent = 0);
	void run();

private slots:
	void processMessages();
	void processMessagesByType(int);
	void processErrors();
	
public slots:
	bool startJob();
	enum PState{ProcessStarting, ProcessRunning, ProcessError, ProcessNotRunning}

signals:
	void startupSuccess();
	void startupFailed();
	void logMessage(LogInfo*);

protected:
	PState m_state;
	QString m_name;
	OOSession* m_session;
	QProcess* m_process;
};

class OOMProcessThread : public OOThread
{
	Q_OBJECT
public:
	OOMProcessThread(QObject* parent = 0);
	void run();

public slots:
	virtual bool startJob();

};

class JackProcessThread : public QThread
{
	Q_OBJECT
public:
	JackProcessThread(QObject* parent = 0);
	void run();

public slots:
	virtual bool startJob();
};

class LinuxSamplerThread : public QThread
{
	Q_OBJECT

public:
	LinuxSamplerThread(QObject* parent = 0);
	void run();

public slot:
	virtual bool startJob();
};
#endif
