#ifndef _LS_CLIENT_
#define _LS_CLIENT_

#include "globaldefs.h"
#include <lscp/client.h>
#include <lscp/device.h>
#include <lscp/socket.h>
#include <QList>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QEvent>
#include <QString>
#include <QMap>
#include <QQueue>

#define LSCLIENT_LSCP_EVENT  QEvent::Type(QEvent::User + 1)

#define SAMPLER_PORT 8888

typedef struct chaninfo/*{{{*/
{
	int hbank;
	int lbank;
	int program;
	QList<int> key_bindings;
	QList<int> keyswitch_bindings;
	QString instrument_filename;
	//const char* midi_mapname;
	QString instrument_name;
	QString midi_portname;
	int midi_port;
	int midi_device;
	int midi_channel;
	bool valid;;
} LSCPChannelInfo ;/*}}}*/

typedef struct lscp_keymap/*{{{*/
{
	QList<int> key_bindings;
	QList<int> keyswitch_bindings;
} LSCPKeymap ;/*}}}*/

class QTimer;
class MidiInstrument;
class MidiInstrumentList;
class Patch;
class PatchGroup;
class LSClient;

class LSClient : public QObject
{
	Q_OBJECT

public:
	enum SamplerEngine {
		GIG = 0, SFZ, SF2
	};

	LSClient(QString host = "localhost", int port = 8888, QObject *parent = 0);
	~LSClient();
	void stopClient();
	bool startClient();
	void setTimeout(int t){ _timeout = t; }
	void setRetry(int r){ _retries = r; }
	void setBankAsNumber(bool v) { _useBankNumber = v; }
	void mapInstrument(int);
	int getError();
	MidiInstrumentList* getInstruments(QList<int>);
	MidiInstrument* getInstrument(int);
	MidiInstrument* getInstrument(QString);

	bool loadInstrument(MidiInstrument*);
	bool unloadInstrument(MidiInstrument*);
	int findMidiMap(const char*);
	
	int createMidiInputDevice(char* name, const char* type = "JACK", int ports = 1);
	int createAudioOutputDevice(char* name, const char* type = "JACK", int ports = 1, int iSrate = 48000);
	bool createInstrumentChannel(const char* name, const char* engine, const char* filename, int index, int map, SamplerData** data);
	bool updateInstrumentChannel(SamplerData* data, const char* engine, const char* filename, int index, int map);
	bool removeInstrumentChannel(SamplerData*);
	bool renameMidiPort(int port, QString newName, int mdev = 0);
	bool renameAudioChannel(int chan, QString newName, int adev = 0);
	QMap<int, QString> listInstruments();
	QString getValidInstrumentName(QString nameBase);
	QString getMapName(int);
	bool resetSampler();
	bool loadSamplerCommand(QString);
	void removeLastChannel();
	bool removeMidiMap(int m);
	bool isClientStarted();
	int getFreeAudioOutputChannel(int adev);
	int getFreeMidiInputPort(int mdev);
	
private:
	const LSCPChannelInfo getKeyBindings(lscp_channel_info_t*);
	bool compare(const LSCPChannelInfo, const LSCPChannelInfo);

	lscp_client_t* _client;
	QString _hostname;
	int _port;
	bool _abort;
	int _retries;
	int _timeout;
	bool _useBankNumber;
	LSCPChannelInfo _lastInfo;
	LSCPKeymap _getKeyMapping(QString, int, int);
	QString _stripAscii(QString);
	bool _loadInstrumentFile(const char*, int, int);
	bool isFreePort(const char*);

protected:
	void customEvent(QEvent*);

signals:
	void channelInfoChanged(const LSCPChannelInfo);
	void instrumentMapped(MidiInstrument*);

public slots:
	void subscribe();
	void unsubscribe();

};	

//The following class is borrowed from qSampler
class LscpEvent : public QEvent
{
public:

	LscpEvent(lscp_event_t event, const char *pchData, int cchData)	: QEvent(LSCLIENT_LSCP_EVENT)
	{
		m_event = event;
		m_data  = QString::fromUtf8(pchData, cchData);
	}

	lscp_event_t event() { return m_event; }
	QString&     data()  { return m_data;  }

private:

	lscp_event_t m_event;
	QString      m_data;
};

#endif
