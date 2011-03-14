#include "config.h"
#include "globals.h"
#include "gconfig.h"
#ifdef LSCP_SUPPORT
#include "audio.h"
#include "instruments/minstrument.h"
#include "midictrl.h"
#include "lsclient.h"
#include <ctype.h>
#include <QStringListIterator>
#include <QStringList>
#include <QCoreApplication>
#include <QTimer>
#include <QFileInfo>

LSClient::LSClient(const char* host, int p, QObject* parent) : QThread(parent)
{
	_hostname = host;
	_port = p;
	_abort = false;
	_client = NULL;
}

LSClient::~LSClient()
{
	//mutex.lock();
	_abort = true;
	//condition.wakeOne();
	//mutex.unlock();
	//wait();
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
		::lscp_client_unsubscribe(_client, LSCP_EVENT_CHANNEL_INFO);
		//unsubscribe();
		//startClient();
	}
}

void LSClient::unsubscribe()
{
	mutex.lock();
	if(_client != NULL)
	{
		::lscp_client_unsubscribe(_client, LSCP_EVENT_CHANNEL_INFO);
		//::lscp_client_destroy(_client);
	}
	//_client = NULL;
	mutex.unlock();
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
	startClient();
	exec();
}

bool LSClient::startClient()/*{{{*/
{
	if(_client != NULL)
		::lscp_client_destroy(_client);
	_client = ::lscp_client_create(_hostname, _port, client_callback, this);
	if(_client != NULL)
	{
		printf("Initialized LSCP client connection\n");
		::lscp_client_set_timeout(_client, 1000);
		lastInfo.valid = false;
		return true;
		//::lscp_client_subscribe(_client, LSCP_EVENT_CHANNEL_INFO);
	}
	else
	{
		printf("Failed to Initialize LSCP client connection\n");
		return false;
	}
}/*}}}*/

void LSClient::stopClient()/*{{{*/
{
	if(_client != NULL)
	{
		//TODO: create a list of subscribed events so we can reference it here and unsubscribe 
		//to any subscribed event.
		//if(_client->cmd->iState == 0)
		//::lscp_client_unsubscribe(_client, LSCP_EVENT_CHANNEL_INFO);
		lastInfo.valid = false;
		::lscp_client_destroy(_client);

	}
	_client = NULL;
}/*}}}*/

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
	if(process)/*{{{*/
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
	}/*}}}*/

	info.valid = process;
	printf("Leaving LSClient::getKeyBindings()\n");
	return info;
}/*}}}*/

bool LSClient::compare(LSCPChannelInfo info1, LSCPChannelInfo info2)/*{{{*/
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
}/*}}}*/

void LSClient::customEvent(QEvent* event)/*{{{*/
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
}/*}}}*/

/**
 * getKeyMapping used to retrieve raw key bindings and switches for a sample
 * @param fname QString The string filename of the gig file
 * @param nr int The location in the file of the given instrument
 */
LSCPKeymap LSClient::getKeyMapping(QString fname, int nr, QString engine)/*{{{*/
{
	//printf("Starting key binding processing\n");
	QList<int> keys;
	QList<int> switched;
	QString keyStr = "KEY_BINDINGS:";
	QString keySwitchStr = "KEYSWITCH_BINDINGS:";
	char query[1024];
	LSCPKeymap rv;
	if(_client != NULL)
	{
		//Try to load the instrument into RAM on channel 0 so we can get the keymaps for the instrument
		//Linuxsampler will not give key info until the gig is loaded into ram, we will retry 5 times
		int retry = 0;
		//Create a channel
		int chan = ::lscp_add_channel(_client);
		if(chan >= 0 && lscp_load_engine(_client, engine.toUtf8().constData(), chan) == LSCP_OK)
		{
			//Get audio channels
			int adev =  ::lscp_get_audio_devices(_client);
			if(adev != -1 && lscp_set_channel_audio_device(_client, chan, 0) == LSCP_OK)
			{
				//sleep(1);
				//Load instruments into your created channel
				while(lscp_load_instrument(_client, fname.toUtf8().constData(), nr, chan) != LSCP_OK && retry < 5)
				{
					printf("Failed to preload instrument:\n %s into ram...retrying\n", fname.toUtf8().constData());
					sleep(1);
					++retry;
				}
				
				//We need to sleep a bit here to give LS time to load the gig
				sleep(1);
again:
				sprintf(query, "GET FILE INSTRUMENT INFO '%s' %d\r\n", fname.toAscii().constData(), nr);
				if (lscp_client_query(_client, query) == LSCP_OK)
				{
					const char* ret = lscp_client_get_result(_client);
					QString values(ret);
					printf("Server Returned:\n %s\n", ret);
					QStringList arrayVal = values.split("\r\n", QString::SkipEmptyParts);
					QStringListIterator vIter(arrayVal);
					bool found = false;
					while(vIter.hasNext())
					{
						QString i = vIter.next().trimmed();
						if(i.startsWith(keyStr, Qt::CaseSensitive))
						{
							found = true;
							i = i.replace(keyStr, "").trimmed();
							if(i.contains(","))
							{
								QStringList sl = i.split(",", QString::SkipEmptyParts);
								QStringListIterator iter(sl);
								while(iter.hasNext())
								{
									keys.append(iter.next().toInt());
								}
								rv.key_bindings = keys;
							}
						}
						else if(i.startsWith(keySwitchStr, Qt::CaseSensitive))
						{
							found = true;
							i = i.replace(keyStr, "").trimmed();
							i = i.replace(keySwitchStr, "").trimmed();
							if(i.contains(","))
							{
								QStringList sl = i.split(",", QString::SkipEmptyParts);
								QStringListIterator iter(sl);
								while(iter.hasNext())
								{
									switched.append(iter.next().toInt());
								}
								rv.keyswitch_bindings = switched;
							}
						}
					}
					if(!found)
						goto again; //I cant believe i'm doing this, ick!!
				}//END query check
				else
				{
					printf("Failed to lookup instrument file info\n");
				}
			}
			//Flush the disk streams used by the channel created
			::lscp_reset_channel(_client, chan);
			//Finally remove the channel
			::lscp_remove_channel(_client, chan);
		}//END channel check
	}//END client NULL check
	return rv;
}/*}}}*/

/**
 * Return a MidiInstrumentList of all the current instrument loaded into
 * linuxsampler
 */
MidiInstrumentList* LSClient::getInstruments()/*{{{*/
{
	if(_client != NULL)
	{
		int* maps = ::lscp_list_midi_instrument_maps(_client);
		if(maps != NULL)
		{
			MidiInstrumentList* instruments = new MidiInstrumentList;
			for(int m = 0; maps[m] >= 0; ++m)
			{
				QString mapName = getMapName(maps[m]);
				if(!mapName.isEmpty())
				{
					QString insName(getValidInstrumentName(mapName));
					MidiInstrument *midiInstr = new MidiInstrument(insName);
					QString path = oomUserInstruments;
					path += QString("/%1.idf").arg(insName);
					midiInstr->setFilePath(path);
					PatchGroupList *pgl = midiInstr->groups();
					lscp_midi_instrument_t* instr = ::lscp_list_midi_instruments(_client, maps[m]);
					for (int in = 0; instr && instr[in].map >= 0; ++in)
					{
						lscp_midi_instrument_t tmp;
						tmp.map = instr[in].map;
						tmp.bank = instr[in].bank;
						tmp.prog = instr[in].prog;
						lscp_midi_instrument_info_t* insInfo = ::lscp_get_midi_instrument_info(_client, &tmp);
						if(insInfo != NULL)
						{
							QString ifname(insInfo->instrument_file);
							QFileInfo finfo(ifname);
							QString fname = finfo.baseName().simplified().replace(" ", "_");
							PatchGroup *pg = 0;
							for(iPatchGroup pi = pgl->begin(); pi != pgl->end(); ++pi)
							{
								if((*pi)->id == instr[in].bank)
								{
									pg = (PatchGroup*)*pi;
								}
							}
							if(!pg)
							{
								pg = new PatchGroup();
								pg->name = fname;
								pg->id = instr[in].bank;
								pgl->push_back(pg);
							}
							
							QString patchName(insInfo->instrument_name);
							if(patchName.isEmpty())
								patchName = QString(insInfo->name);
							//Setup the patch
							Patch* patch = new Patch;
							patch->name = patchName;
							patch->hbank = 0;
							patch->lbank = instr[in].bank;
							patch->prog = instr[in].prog;
							patch->typ = -1;
							patch->drum = false;
							LSCPKeymap kmap = getKeyMapping(QString(insInfo->instrument_file), insInfo->instrument_nr, QString(insInfo->engine_name));
							patch->keys = kmap.key_bindings;
							patch->keyswitches = kmap.keyswitch_bindings;
							pg->patches.push_back(patch);
						}
					}
					instruments->push_back(midiInstr);
				}
			}
			return instruments;
		}
	}
	return 0;
}/*}}}*/

/**
 * Lookup a midi map name by ID
 * @param int ID of the midi instrument map
 */
QString LSClient::getMapName(int mid)
{
	QString mapName;
	if(_client == NULL)
		return mapName;
	const char* cname = ::lscp_get_midi_instrument_map_name(_client, mid);
	if(cname != NULL)
	{
		mapName = QString(cname);
	}
	return mapName;
}

/**
 * Get a single instrument from the sampler by name
 * @param QString name
 */
MidiInstrument* LSClient::getInstrument(QString)
{
	return new MidiInstrument("place_holder");
}

QString LSClient::getValidInstrumentName(QString nameBase)/*{{{*/
{
	bool found = false;
	for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
	{
		if (nameBase == (*i)->iname())
		{
			found = true;
			break;
		}
	}
	if(!found)
	{//Use the name given
		return nameBase;
	}
	else
	{//Randomize the name
		for (int i = 1;; ++i)
		{
			QString s = QString("%1-%2").arg(nameBase).arg(i);
			found = false;
			for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
			{
				if (s == (*i)->iname())
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				return s;
			}
		}
	}

}/*}}}*/
#endif
