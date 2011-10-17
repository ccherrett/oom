//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOTHREAD_H
#define OOTHREAD_H

#include <QThread>

class QProcess;
struct LogInfo;

class OOThread : public QThread
{
	Q_OBJECT

protected:
	QProcess* m_process;

public:
	OOThread(QObject* parent = 0);
	void run();

public slots:
	bool startJob();

signals:
	void startupSuccess();
	void startupFailed();
	void logMessage(LogInfo*);
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
