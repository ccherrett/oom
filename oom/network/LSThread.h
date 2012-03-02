//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOTHREAD_H
#define OOTHREAD_H

#include <QThread>
#include <QProcess>
#include <QHash>
#include <QStringList>
#include <QFileInfo>
#include <QUrl>

class QProcess;

class LSThread : public QThread
{
	Q_OBJECT

	QStringList m_lscpLabels;
	bool m_versionError;
	bool pingLinuxsampler();
public:
	enum PState{ProcessStarting, ProcessRunning, ProcessError, ProcessNotRunning, ProcessNotRequested};

protected:
	PState m_state;
	QString m_name;
	QProcess* m_process;

private slots:
	void processMessages();
	void processMessagesByType(int);
	void processErrors();
	void relayExit(int, QProcess::ExitStatus);
	
signals:
	void startupSuccess();
	void startupFailed();
	void processExit(int);

public:
	LSThread();
	void run();
	bool programStarted();
	bool programError();
	PState programState();
};
#endif
