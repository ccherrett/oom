// This file is a part of the OOMidi project.
// Copyright 2011 Andrew Williams
// License: GPL v2

#ifndef _OOMIDI_SAMPLER_TEST_
#define _OOMIDI_SAMPLER_TEST_

#include <QtGui>
#include <QTcpSocket>

class QTcpSocket;

class SamplerConnectionTest : public QWidget
{
	Q_OBJECT
public:
	SamplerConnectionTest(QString host = "localhost", int port = 8888, QWidget *parent = 0);
	bool isSamplerRunning()
	{
		return _connected;
	}
private:
	QString hostname;
	int port;
	QTcpSocket *socket;
	bool _connected;
	quint16 blockSize;
private slots:
	void displayError(QAbstractSocket::SocketError socketError);
	void displayStatus();
};

#endif
