// This file is a part of the OOMidi project.
// Copyright 2011 Andrew Williams
// License: GPL v2

#include <QtGui>
#include <QtNetwork>
#include "config.h"
#include <stdlib.h>
#include "samplertest.h"

SamplerConnectionTest::SamplerConnectionTest(QString host, int p, QWidget* parent) :QWidget(parent)
{
	hostname = host;
	port = p;
	_connected = false;
	socket = new QTcpSocket(this);
	connect(socket, SIGNAL(ready()), this, SLOT(displayStatus()));
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
}

void SamplerConnectionTest::displayStatus()
{
	//get the sampler engine info and display it
	socket->abort();
	socket->connectToHost(hostname, port);
	QString statusText = "GET SERVER INFO\r\n";
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setVersion(QDataStream::Qt_4_0);
	out << (quint16)0;
	out << statusText;
	out.device()->seek(0);
	out << (quint16)(ba.size() - sizeof(quint16));
	socket->write(ba);

	_connected = true;

	QDataStream in(socket);
	in.setVersion(QDataStream::Qt_4_0);
	if (blockSize == 0) {
		if (socket->bytesAvailable() < (int)sizeof(quint16))
			return;
		in >> blockSize;
	}
	if (socket->bytesAvailable() < blockSize)
		return;
	QString statusResponse;
	in >> statusResponse;
	printf("LinuxSampler Server Info:\n%s\n", statusResponse.toUtf8().constData());
}

void SamplerConnectionTest::displayError(QAbstractSocket::SocketError socketError)
{
	switch (socketError)
	{
		case QAbstractSocket::RemoteHostClosedError:
		    break;
		case QAbstractSocket::HostNotFoundError:
		    QMessageBox::information(this, tr("LinuxSampler Error"),
		                             tr("The host was not found. Please check the host name and port settings."));
		    break;
		case QAbstractSocket::ConnectionRefusedError:
		    QMessageBox::information(this, tr("LinuxSampler Error"),
		                             tr("The connection was refused by the peer. "
		                                "Make sure the LinuxSampler server is running, "
		                                "and check that the host name and port "
		                                "settings are correct."));
		    break;
		default:
		    QMessageBox::information(this, tr("LinuxSampler Error"),
		                             tr("The following error occurred: %1.")
		                             .arg(socket->errorString()));
	}
	_connected = false;
}
