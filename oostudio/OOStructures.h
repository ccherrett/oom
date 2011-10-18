struct OOSession/*{{{*/
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
};/*}}}*/

