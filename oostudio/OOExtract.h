//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOEXTRACT_H
#define OOEXTRACT_H

#include <QObject>
#include <QProcess>
#include <QThread>

class DownloadPackage;

enum ExtractFormat {
	TGZ = 0,
	TBZ2,
	RAR,
	ZIP
};

struct ExtractJob  {
	QString fileName;
	QString basePath;
	ExtractFormat format;
	int type;
};

class OOExtract : public QThread
{
private:
	Q_OBJECT
	QProcess* m_process;
	ExtractJob* m_job;
	void run();

private slots:
	void finished(int);
	void readError();
	void readOutput();

signals:
	void extractComplete(int);
	void extractFailed(int, QString);

public:
	OOExtract(ExtractJob* job);
};

#endif
