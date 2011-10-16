//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef OOEXTRACT_H
#define OOEXTRACT_H

#include <QObject>
#include <QProcess>

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
};

class OOExtract : public QObject
{
private:
	Q_OBJECT
	QProcess* m_process;

private slots:
	void finished(int, QProcess::ExitStatus);
	void readError();
	void readOutput();

public slots:
	bool startExtract(ExtractJob* job);

signals:
	void extractComplete(int);

public:
	OOExtract();
};

#endif
