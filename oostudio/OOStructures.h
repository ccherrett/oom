//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================
class QFileInfo;
class QUrl;

struct OOSession
{
	QString path;
	QString name;
	bool istemplate;
	bool fromTemplate;
	bool loadJack;
	bool loadls;
	QString jackcommand;
	QString lscommand;
	QStringList commands;
	QStringList postCommands;
	QString lscpPath;
	QString notes;
	QString createdFrom;
	int lscpMode;
	QString songfile;
	QString lshostname;
	int lsport;
};

struct LogInfo
{
	QString name;
	QString message;
	QString codeString;
};

enum SamplePack
{
	Sonatina = 0,
	Maestro,
	ClassicGuitar,
	AcousticGuitar,
	M7IR44,
	M7IR48,
	ALL //Should only be used to pass download all the the OOStudio::download(SamplePack) command
};
struct DownloadPackage
{
	QString name;
	QString filename;
	int format;
	QFileInfo install_path;
	QUrl path;
	QUrl homepage;
	SamplePack type;
};

