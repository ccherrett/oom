#include "config.h"
#include "globals.h"
#include "gconfig.h"
#ifdef LSCP_SUPPORT
#include "audio.h"
#include "lsclient.h"
#include <ctype.h>
#include <QStringListIterator>
#include <QStringList>
#include <QCoreApplication>
#include <QTimer>

LSClient::LSClient(const char* host, int p, QObject* parent) : QThread(parent)
{
	_hostname = host;
	_port = p;
	_abort = false;
}

LSClient::~LSClient()
{
	mutex.lock();
	_abort = true;
	//condition.wakeOne();
	mutex.unlock();
	wait();
}

lscp_status_t client_callback ( lscp_client_t* /*_client*/, lscp_event_t event, const char *pchData, int cchData, void* pvData )
{
	lscp_status_t ret = LSCP_FAILED;
	LSClient* lsc = (LSClient*)pvData;

	if(lsc == NULL)
		return ret;

	switch(event)
	{
		case LSCP_EVENT_CHANNEL_INFO:
			//printf("Recieved CHANNEL_INFO event\n");
			QCoreApplication::postEvent(lsc, new LscpEvent(event, pchData, cchData));
		break;
		default:
			return LSCP_OK;
		break;
	}
	return LSCP_OK;
}

void LSClient::subscribe()
{
	if(_client == NULL)
	{
		startClient();
	}
	else
	{
		unsubscribe();
		startClient();
	}
}

void LSClient::unsubscribe()
{
	if(_client != NULL)
	{
		::lscp_client_unsubscribe(_client, LSCP_EVENT_CHANNEL_INFO);
		::lscp_client_destroy(_client);
	}
	_client = NULL;
}

int LSClient::getError()
{
	if(_client != NULL)
	{
		return -1;
	}
	return lscp_client_get_errno(_client);
}

void LSClient::run()
{
	/*_client = ::lscp_client_create(_hostname, _port, client_callback, this);
	if(_client != NULL)
	{
		printf("Initialized LSCP client connection\n");
		::lscp_client_set_timeout(_client, 1000);
		::lscp_client_subscribe(_client, LSCP_EVENT_CHANNEL_INFO);
	}
	else
	{
		printf("Failed to Initialize LSCP client connection\n");
		//
	}*/
	//heartBeatTimer->start(1000 / config.guiRefresh);
	startClient();
	exec();
}

void LSClient::startClient()
{
	_client = ::lscp_client_create(_hostname, _port, client_callback, this);
	if(_client != NULL)
	{
		printf("Initialized LSCP client connection\n");
		::lscp_client_set_timeout(_client, 1000);
		lastInfo.valid = false;
		::lscp_client_subscribe(_client, LSCP_EVENT_CHANNEL_INFO);
	}
	else
	{
		printf("Failed to Initialize LSCP client connection\n");
		//
	}
}

void LSClient::stopClient()
{
	if(_client != NULL)
	{
		//TODO: create a list of subscribed events so we can reference it here and unsubscribe 
		//to any subscribed event.
		::lscp_client_unsubscribe(_client, LSCP_EVENT_CHANNEL_INFO);
		lastInfo.valid = false;
		::lscp_client_destroy(_client);

		if (!wait(1000))
		{
			terminate();
		}
	}
}

/**
 * Returns a pair of list the first contain all the KEY_BINDINGS
 * the second contains a list of all the KEYSWITCH_BINDINGS for 
 * the queried loaded instrument.
 * The list will be zero length if there is an error or NULL is returned
 * by linuxsampler
 */
const LSCPChannelInfo LSClient::getKeyBindings(lscp_channel_info_t* chanInfo)/*{{{*/
{
	printf("\nEntering LSClient::getKeyBindings()\n");
	LSCPChannelInfo info;
	if(chanInfo == NULL)
	{
		printf("Channel Info is NULL\n");
		info.valid = false;
		return info;
	}
	else
	{
		printf("Found Channel\n");
	}	
	QList<int> keys;
	QList<int> switched;
	QString keyStr = "KEY_BINDINGS:";
	QString keySwitchStr = "KEYSWITCH_BINDINGS:";
	char query[1024];
	bool process = false;
	int nr = chanInfo->instrument_nr;
	QString chanfname(chanInfo->instrument_file);
	
	//Lookup the instrument map
	lscp_midi_instrument_t* mInstrs = ::lscp_list_midi_instruments(_client, chanInfo->midi_map);
	for (int iInstr = 0; mInstrs && mInstrs[iInstr].map >= 0; iInstr++)
	{
		lscp_midi_instrument_info_t *instrInfo = ::lscp_get_midi_instrument_info(_client, &mInstrs[iInstr]);
		if(instrInfo)
		{
			//printf("Found instrument to process\n");
			printf("Instrument - file: %s, nr:%d, Channel - file: %s, nr: %d\n", instrInfo->instrument_file, instrInfo->instrument_nr, chanInfo->instrument_file, chanInfo->instrument_nr);
			if(instrInfo->instrument_nr == nr)
			{
				printf("Found matching nr\n");
				QString insfname(instrInfo->instrument_file);

				//if(instrInfo->instrument_file == chanInfo->instrument_file)
				if(chanfname == insfname)
				{
					printf("Found Correct instrument !!!!\n");
					info.instrument_name = QString(instrInfo->instrument_name);
					info.instrument_filename = QString(instrInfo->instrument_file);
					info.hbank = 0;
					info.lbank = mInstrs[iInstr].bank;
					info.program = mInstrs[iInstr].prog;
					info.midi_port = chanInfo->midi_port;
					info.midi_device = chanInfo->midi_device;
					info.midi_channel = chanInfo->midi_channel;
					char iquery[1024];
					sprintf(iquery, "GET MIDI_INPUT_PORT INFO %d %d\r\n",info.midi_device, info.midi_port);
					printf("Query for MIDI_INPPUT_PORT\n%s\n", iquery);
					if (lscp_client_query(_client, iquery) == LSCP_OK)
					{
						const char* ret = lscp_client_get_result(_client);
						printf("Return value of MIDI_INPPUT_PORT\n%s\n", ret);
						QString midiInputPort(ret);
						QStringList sl = midiInputPort.split("\r\n", QString::SkipEmptyParts);
						QStringListIterator iter(sl);
						while(iter.hasNext())
						{
							QString tmp = iter.next().trimmed();
							if(tmp.startsWith("NAME", Qt::CaseSensitive))
							{
								printf(" Found midi port - %s\n", tmp.toUtf8().constData());
								QStringList tmp2 = tmp.split(":", QString::SkipEmptyParts);
								if(tmp2.size() > 1)
								{
									printf(" Processing input port\n");
									//info.midi_portname = tmp2.at(1).trimmed().toUtf8().constData();
									QString portname = tmp2.at(1).trimmed();
									portname = portname.remove("'");
									info.midi_portname = portname;//.toUtf8().constData();
																																													
									printf("info midi port - %s\n", info.midi_portname.toAscii().constData());
									process = true;
									break;
								}
							}
						}
					}
					else
					{
						printf("Bad LSCP command \n%d\n", lscp_client_get_errno(_client));
					}
					break;
				}
			}	

		}
	}
	if(process)
	{
		printf("Starting key binding processing\n");
		sprintf(query, "GET FILE INSTRUMENT INFO '%s' %d\r\n", info.instrument_filename.toAscii().constData(), nr);
		if (lscp_client_query(_client, query) == LSCP_OK)
		{
			const char* ret = lscp_client_get_result(_client);
			QString values(ret);
			printf("Server Returned:\n %s\n", ret);
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
							keys.append(iter.next().toInt());
						}
						info.key_bindings = keys;
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
							switched.append(iter.next().toInt());
						}
						info.keyswitch_bindings = switched;
					}
				}
			}
		}
	}

	info.valid = process;
	printf("Leaving LSClient::getKeyBindings()\n");
	return info;
}/*}}}*/

bool LSClient::compare(LSCPChannelInfo info1, LSCPChannelInfo info2)
{
	if(!info1.valid)
		return false;
	if(info1.instrument_filename == info2.instrument_filename)
	{
		if(info1.instrument_name == info2.instrument_name)
		{
			if(info1.midi_portname == info2.midi_portname)
			{
				if(info1.midi_channel == info2.midi_channel)
				{
					if(info1.hbank == info2.hbank)
					{
						if(info1.lbank == info2.lbank)
						{
							if(info1.program == info2.program)
							{
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

void LSClient::customEvent(QEvent* event)
{
	LscpEvent *lscpEvent = static_cast<LscpEvent *> (event);
	if(lscpEvent)
	{
		switch(lscpEvent->event())
		{
			case LSCP_EVENT_CHANNEL_INFO:
			{
				if(!audio->isPlaying())
				{
					int channel = lscpEvent->data().toInt();
					lscp_channel_info_t* chanInfo = ::lscp_get_channel_info(_client, channel);
					if(chanInfo != NULL)
					{
						LSCPChannelInfo info = getKeyBindings(chanInfo);
						if(info.valid)
						{
							if(!compare(lastInfo, info))
							{
								emit channelInfoChanged(info);
								lastInfo = info;
							}
							lastInfo = info;
						}
					}
				}
				break;
			}	
			default:
				return;
			break;
		}
	}
}
#endif
