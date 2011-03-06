#include "config.h"
#ifdef LSCP_SUPPORT
#include "lsclient.h"
#include <ctype.h>
#include <QStringListIterator>
#include <QStringList>

LSClient::LSClient(const char* host, int p)
{
	_hostname = host;
	_port = p;
}

LSClient::~LSClient()
{
}

lscp_status_t client_callback ( lscp_client_t* /*_client*/, lscp_event_t event, const char *pchData, int cchData, void* /*pvData*/ )
{
	lscp_status_t ret = LSCP_FAILED;

	char *pszData = (char *) malloc(cchData + 1);
	if (pszData) {
		memcpy(pszData, pchData, cchData);
		pszData[cchData] = (char) 0;
		printf("client_callback: event=%s (0x%04x) [%s]\n", lscp_event_to_text(event), (int) event, pszData);
		free(pszData);
		ret = LSCP_OK;
	}

	return ret;
}

bool LSClient::initClient()
{
	_client = ::lscp_client_create(_hostname, _port, client_callback, this);
	if(_client == NULL)
	{
		return false;
	}
	//lscp_client_subscribe(_client, LSCP_EVENT_MISCELLANEOUS);
	lscp_client_subscribe(_client, LSCP_EVENT_CHANNEL_INFO);
	//lscp_client_subscribe(_client, LSCP_EVENT_MIDI_INSTRUMENT_MAP_INFO);
	//lscp_client_subscribe(_client, LSCP_EVENT_MIDI_INSTRUMENT_INFO);
	return true;
}

int LSClient::client_test_status ( lscp_status_t s )
{
	const char *pszStatus;

	switch (s) {
	case LSCP_OK:         pszStatus = "OK";       break;
	case LSCP_FAILED:     pszStatus = "FAILED";   break;
	case LSCP_ERROR:      pszStatus = "ERROR";    break;
	case LSCP_WARNING:    pszStatus = "WARNING";  break;
	case LSCP_TIMEOUT:    pszStatus = "TIMEOUT";  break;
	case LSCP_QUIT:       pszStatus = "QUIT";     break;
	default:              pszStatus = "NONE";     break;
	}
	printf("%s\n", pszStatus);
	return (s == LSCP_OK ? 0 : 1);
}

int LSClient::client_test_channel_info ( lscp_channel_info_t *pChannelInfo )
{
	if (pChannelInfo == NULL) {
		printf("(nil)\n");
		return 1;
	}
	printf("{\n");
	printf("    channel_info.engine_name       = %s\n", pChannelInfo->engine_name);
	printf("    channel_info.audio_device      = %d\n", pChannelInfo->audio_device);
	printf("    channel_info.audio_channels    = %d\n", pChannelInfo->audio_channels);
//	printf("    channel_info.audio_routing     = %d\n", pChannelInfo->audio_routing);
	printf("    channel_info.instrument_file   = %s\n", pChannelInfo->instrument_file);
	printf("    channel_info.instrument_nr     = %d\n", pChannelInfo->instrument_nr);
	printf("    channel_info.instrument_name   = %s\n", pChannelInfo->instrument_name);
	printf("    channel_info.instrument_status = %d\n", pChannelInfo->instrument_status);
	printf("    channel_info.midi_device       = %d\n", pChannelInfo->midi_device);
	printf("    channel_info.midi_port         = %d\n", pChannelInfo->midi_port);
	printf("    channel_info.midi_channel      = %d\n", pChannelInfo->midi_channel);
	printf("    channel_info.midi_map          = %d\n", pChannelInfo->midi_map);
	printf("    channel_info.volume            = %g\n", pChannelInfo->volume);
	printf("    channel_info.mute              = %d\n", pChannelInfo->mute);
	printf("    channel_info.solo              = %d\n", pChannelInfo->solo);
	printf("  }\n");
	return 0;
}


void LSClient::testClient()
{
}

/**
 * Returns a pair of list the first contain all the KEY_BINDINGS
 * the second contains a list of all the KEYSWITCH_BINDINGS for 
 * the queried loaded instrument.
 * The list will be zero length if there is an error or NULL is returned
 * by linuxsampler
 */
QList<QList<int>*>* LSClient::getKeyBindings(QString fname, int instrId)/*{{{*/
{
	QList<int> *keys = new QList<int>();
	QList<int> *switched = new QList<int>();
	QList<QList<int>*> *list = new QList<QList<int>*>();
	QString keyStr = "KEY_BINDINGS:";
	QString keySwitchStr = "KEYSWITCH_BINDINGS:";
	char query[1024];
	sprintf(query, "GET FILE INSTRUMENT INFO '%s' %d\r\n", fname.toUtf8().constData(), instrId);
	if (lscp_client_query(_client, query) == LSCP_OK)
	{
		const char* ret = lscp_client_get_result(_client);
		QString values(ret);
		printf("Server Returned:\n %s\n",ret);
		QStringList arrayVal = values.split("\r\n", QString::SkipEmptyParts);
		QStringListIterator vIter(arrayVal);
		while(vIter.hasNext())
		{
			QString i = vIter.next().trimmed();
			if(i.startsWith(keyStr, Qt::CaseSensitive))
			{
				i = i.replace(keyStr, "").trimmed();
				if(i.contains(","))
				{
					QStringList sl = i.split(",", QString::SkipEmptyParts);
					QStringListIterator iter(sl);
					while(iter.hasNext())
					{
						keys->append(iter.next().toInt());
					}
					list->insert(0, keys);
				}
			}
			else if(i.startsWith(keySwitchStr, Qt::CaseSensitive))
			{
				i = i.replace(keySwitchStr, "").trimmed();
				if(i.contains(","))
				{
					QStringList sl = i.split(",", QString::SkipEmptyParts);
					QStringListIterator iter(sl);
					while(iter.hasNext())
					{
						switched->append(iter.next().toInt());
					}
					list->insert(1, switched);
				}
			}
		}
	}

	return list;
}/*}}}*/
#endif
