#include "cthread.h"
#include "cserver.h"
#include "app.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include <stdlib.h>

OOMCommandServer::OOMCommandServer(QObject* parent) : QTcpServer(parent)
{
}

void OOMCommandServer::incomingConnection(int socket)
{
	//Start the thread to process the connection
	printf("Recieved incoming connection\n");
	OOMClientThread *thread = new OOMClientThread(socket,  this);
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	connect(thread, SIGNAL(saveSong()), SLOT(saveTriggered()));
	connect(thread, SIGNAL(saveSongAs()), SLOT(saveAsTriggered()));
	thread->start();
}

void OOMCommandServer::saveTriggered()
{
	oom->save();
}

void OOMCommandServer::saveAsTriggered()
{
	oom->saveAs();
}
