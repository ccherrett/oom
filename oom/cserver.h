#ifndef _OOM_COMMAND_SERVER_
#define _OOM_COMMAND_SERVER_

#include <QTcpServer>

class OOMCommandServer : public QTcpServer
{
	Q_OBJECT

	public:
		OOMCommandServer(QObject *parent = 0);

	protected:
		void incomingConnection(int socket);
	
	public slots:
		void saveAsTriggered();
		void saveTriggered();
		void pipelineStateChanged(int);
};

#endif
