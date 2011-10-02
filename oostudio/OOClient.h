#include <QtNetwork>
#include <QObject>
#include <QString>

class OOClient: public QObject
{
	Q_OBJECT

public:
	OOClient(QString host, quint16 port, QObject* parent = 0);
	~OOClient();
	void callShutdown();
	bool isConnected();
	void connectClient();

private slots:
	void errorHandler(QAbstractSocket::SocketError);
	void hostFound();
	void clientConnect();
	void clientDisconnect();

private:
	  QTcpSocket m_client;
	  QString m_host;
	  quint16 m_port;
	  bool m_connected;
};

