
#ifndef OOPROCESS_H
#define OOPROCESS_H

#include <QProcess>

class OOProcess : public QProcess
{
	Q_OBJECT
	QString name;
	
public:
	OOProcess(QString name, QObject* parent = 0);

private slots:
	void catchReadyOut();
	void catchReadyError();

signals:
	void readyReadStandardOutput(QString name, long pid);
	void readyReadStandardError(QString name, long pid);
};
#endif
