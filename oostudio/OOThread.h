//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOTHREAD_H
#define OOTHREAD_H

#include <QThread>

class QProcess;

class OOThread : public QThread
{
	Q_OBJECT

	QProcess* m_process;

public:
	OOThread(QProcess* p, QObject* parent = 0);
	void run();

public slot:
	bool startJob();

signals:
	void startupSuccess();
	void startupFailed();
};

#endif
