//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

struct OOSession
{
	QString path;
	QString name;
	bool istemplate;
	bool loadJack;
	bool loadls;
	QString jackcommand;
	QString lscommand;
	QStringList commands;
	QStringList postCommands;
	QString lscpPath;
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

struct DownloadPackage
{
	QString name;
	QString filename;
	int format;
	QFileInfo install_path;
	QUrl path;
	QUrl homepage;
};

