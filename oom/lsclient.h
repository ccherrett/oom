#ifndef _LS_CLIENT_
#define _LS_CLIENT_

#include <lscp/client.h>
#include <lscp/device.h>
#include <QList>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QEvent>
#include <QString>

#define LSCLIENT_LSCP_EVENT  QEvent::Type(QEvent::User + 1)

#define SAMPLER_PORT 8888

typedef struct chaninfo/*{{{*/
{
	int hbank;
	int lbank;
	int program;
	QList<int> key_bindings;
	QList<int> keyswitch_bindings;
	const char* instrument_filename;
	//const char* midi_mapname;
	const char* instrument_name;
	const char* midi_portname;
	int midi_port;
	int midi_device;
	int midi_channel;
	bool valid;;
} LSCPChannelInfo ;/*}}}*/

class QTimer;
class LSClient : public QThread
{
	Q_OBJECT

public:
	LSClient(const char* host = "localhost", int port = 8888, QObject *parent = 0);
	~LSClient();
	void stopClient();
	void startClient();
	int getError();

private:
	const LSCPChannelInfo getKeyBindings(lscp_channel_info_t*);
	bool compare(const LSCPChannelInfo, const LSCPChannelInfo);

	lscp_client_t* _client;
	const char* _hostname;
	int _port;
	bool _abort;
	QMutex mutex;
	QWaitCondition condition;
	LSCPChannelInfo lastInfo;

protected:
	void run();
	void customEvent(QEvent*);

signals:
	void channelInfoChanged(const LSCPChannelInfo);

public slots:
	void subscribe();
	void unsubscribe();

};	

class LscpEvent : public QEvent
{
public:

	// Constructor.
	LscpEvent(lscp_event_t event, const char *pchData, int cchData)
		: QEvent(LSCLIENT_LSCP_EVENT)
	{
		m_event = event;
		m_data  = QString::fromUtf8(pchData, cchData);
	}

	// Accessors.
	lscp_event_t event() { return m_event; }
	QString&     data()  { return m_data;  }

private:

	// The proper event type.
	lscp_event_t m_event;
	// The event data as a string.
	QString      m_data;
};

#endif
