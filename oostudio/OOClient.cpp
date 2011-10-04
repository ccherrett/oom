#include "OOClient.h"
#include <QHostAddress>

OOClient::OOClient(QString host, quint16 port, QObject* parent)
: QObject(parent),
 m_host(host),
 m_port(port),
 m_connected(false)
{
	connect(&m_client, SIGNAL(connected()), this, SLOT(clientConnect()));
	connect(&m_client, SIGNAL(disconnect()), this, SLOT(clientDisconnect()));
	//connect(&m_client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorHandler(QAbstractSocket::SocketError)));
	//connect(&m_client, SIGNAL(hostFound()), this, SLOT(hostFound()));
}

OOClient::~OOClient()
{
	m_client.close();
}

bool OOClient::isConnected()
{
	return m_connected;
}

void OOClient::clientConnect()
{
	printf("OOCLient::isConnected - Host: %s, Port: %d\n", m_host.toUtf8().constData(), m_port);
	m_connected = true;
}

void OOClient::clientDisconnect()
{
	printf("Disconnected command client\n");
	m_connected = false;
}

void OOClient::connectClient()
{
	QHostAddress addr(m_host);
	m_client.connectToHost(addr, m_port);
	m_client.connectToHost(QHostAddress::LocalHost, 8415, QIODevice::ReadWrite);
	
	m_client.waitForConnected(1000);
}

void OOClient::errorHandler(QAbstractSocket::SocketError type)
{
	//printf("An ERROR has occurred: %d\n", type);
}

void OOClient::callShutdown()
{
	if(!isConnected())
		connectClient();
	QTextStream out(&m_client);
	
	out << "save_and_exit" << endl;
	
	m_client.waitForBytesWritten();
	
	m_client.disconnectFromHost();
	
	m_client.waitForDisconnected();
}

void OOClient::hostFound()
{
	printf("Found hostname \n");
}
