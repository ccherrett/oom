
#include "OOProcess.h"

OOProcess::OOProcess(QString n, QObject* parent)
: QProcess(parent),
name(n)
{
	connect(this, SIGNAL(readyReadStandardOutput()), this, SLOT(catchReadyOut()));
	connect(this, SIGNAL(readyReadStandardError()), this, SLOT(catchReadyError()));
}

void OOProcess::catchReadyOut()
{
	long id = pid();
	emit readyReadStandardOutput(name, id);
}

void OOProcess::catchReadyError()
{
	long id = pid();
	emit readyReadStandardError(name, id);
}
