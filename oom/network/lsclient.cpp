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
#include <QRegExp>
#include <QVector>
#include <QMap>
#include <QMutexLocker>
#include <QDir>

static const char* SOUND_PATH = "@@OOM_SOUNDS@@";
static const QString SOUNDS_DIR = QString(QDir::homePath()).append(QDir::separator()).append(".sounds");

LSClient::LSClient(const char* host, int p, QObject* parent) : QObject(parent)
{
	_hostname = host;
	_port = p;
	_abort = false;
	_client = NULL;
	_retries = 5;
	_timeout = 1;
}

LSClient::~LSClient()
{
	_abort = true;
	stopClient();
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
	if(_client != NULL)
	{
		::lscp_client_unsubscribe(_client, LSCP_EVENT_CHANNEL_INFO);
		//::lscp_client_destroy(_client);
	}
	//_client = NULL;
}

int LSClient::getError()
{
	if(_client != NULL)
	{
		return -1;
	}
	return lscp_client_get_errno(_client);
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
		_lastInfo.valid = false;
		return true;
		//::lscp_client_subscribe(_client, LSCP_EVENT_CHANNEL_INFO);
	}
	else
	{
		qDebug("Failed to Initialize LSCP client connection, retrying");
		_client = ::lscp_client_create(_hostname, _port, client_callback, this);
		_lastInfo.valid = false;
		if(_client != NULL)
			qDebug("Retry sucessfull");
		else
			qDebug("Retry failed!!!");
		return _client != NULL;
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
		_lastInfo.valid = false;
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
							if(!compare(_lastInfo, info))
							{
								emit channelInfoChanged(info);
								_lastInfo = info;
							}
							_lastInfo = info;
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

bool LSClient::_loadInstrumentFile(const char* fname, int nr, int chan)
{
	bool rv = false;
	if(_client != NULL)
	{
		rv = (lscp_load_instrument(_client, fname, nr, chan) == LSCP_OK);
	}

	return rv;
}

/**
 * getKeyMapping used to retrieve raw key bindings and switches for a sample
 * @param fname QString The string filename of the gig file
 * @param nr int The location in the file of the given instrument
 */
LSCPKeymap LSClient::_getKeyMapping(QString fname, int nr, int chan)/*{{{*/
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
		int tries = 0;
		if(_retries <= 0)
			_retries = 5; //default to sane state
		//Load instruments into your created channel
		bool loaded = false;
		/*while(lscp_load_instrument(_client, fname.toUtf8().constData(), nr, chan) != LSCP_OK && tries < _retries)
		{
			printf("Failed to preload instrument:\n %s into ram...retrying\n", fname.toUtf8().constData());
			if(_timeout)
				sleep(_timeout);
			++tries;
		}*/
		while(tries < _retries)
		{
			if(!loaded)
				loaded = _loadInstrumentFile(fname.toUtf8().constData(), nr, chan);
			//We need to sleep a bit here to give LS time to load the gig
			if(_timeout && !loaded)
				sleep(_timeout);
			int inner = 3;
			for(int c = 0; c < inner; c++)
			{
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
					if(found)
					{
						return rv;
					}
				}//END query check
			}
			++tries;
		}
	}//END client NULL check
	return rv;
}/*}}}*/

/**
 * Return a MidiInstrumentList of all the current instrument loaded into
 * linuxsampler
 */
MidiInstrumentList* LSClient::getInstruments(QList<int> pMaps)/*{{{*/
{
	if(_client != NULL)
	{
		//int* maps = ::lscp_list_midi_instrument_maps(_client);
		if(!pMaps.isEmpty())
		{
			MidiInstrumentList* instruments = new MidiInstrumentList;
			foreach(int m, pMaps)
			{
				MidiInstrument* instr = getInstrument(m);
				if(instr)
					instruments->push_back(instr);
			}//END for
			return instruments;
		}//end maps
	}
	return 0;
}/*}}}*/

void LSClient::mapInstrument(int in)
{
	MidiInstrument* instr = getInstrument(in);
	if(instr)
		emit instrumentMapped(instr);
}

MidiInstrument* LSClient::getInstrument(int pMaps)/*{{{*/
{
	if(_client != NULL)
	{
		//int* maps = ::lscp_list_midi_instrument_maps(_client);
		if(pMaps >= 0)
		{
			//Create a channel
			int chan = ::lscp_add_channel(_client);
			if(chan >= 0 && lscp_load_engine(_client, "GIG", chan) == LSCP_OK)
			{
				//Get audio channels
				int adev =  ::lscp_get_audio_devices(_client);
				if(adev != -1 && lscp_set_channel_audio_device(_client, chan, 0) == LSCP_OK)
				{
					QString mapName = getMapName(pMaps);
					QString insName(getValidInstrumentName(mapName));
					MidiInstrument *midiInstr = new MidiInstrument(insName);
					MidiController *modCtrl = new MidiController("Modulation", CTRL_MODULATION, 0, 127, 0);
					MidiController *expCtrl = new MidiController("Expression", CTRL_EXPRESSION, 0, 127, 0);
					midiInstr->controller()->add(modCtrl);
					midiInstr->controller()->add(expCtrl);
					midiInstr->setOOMInstrument(true);
					QString path = oomUserInstruments;
					path += QString("/%1.idf").arg(insName);
					midiInstr->setFilePath(path);
					PatchGroupList *pgl = midiInstr->groups();
					lscp_midi_instrument_t* instr = ::lscp_list_midi_instruments(_client, pMaps);
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
							QString fname = _stripAscii(finfo.baseName()).simplified();
							PatchGroup *pg = 0;
							for(iPatchGroup pi = pgl->begin(); pi != pgl->end(); ++pi)
							{
								if((*pi)->id == instr[in].bank)
								{
									pg = (PatchGroup*)*pi;
									break;
								}
							}
							if(!pg)
							{
								pg = new PatchGroup();
								pg->name = fname;
								pg->id = instr[in].bank;
								pgl->push_back(pg);
							}
							//If the map name is unknown then set it to the first instrument found in it
							if(in == 0 && mapName.startsWith("Untitled"))
							{
								QString tmpName(getValidInstrumentName(fname.replace(" ","_")));
								path = oomUserInstruments;
								path += QString("/%1.idf").arg(tmpName);
								midiInstr->setFilePath(path);
								midiInstr->setIName(tmpName);
							}
							QString patchName(_stripAscii(QString(insInfo->instrument_name)));
							if(patchName.isEmpty())
								patchName = _stripAscii(QString(insInfo->name));
							//Setup the patch
							Patch* patch = new Patch;
							patch->name = patchName;
							patch->hbank = 0;
							patch->lbank = instr[in].bank;
							patch->prog = instr[in].prog;
							patch->typ = -1;
							patch->drum = false;
							patch->engine = QString(insInfo->engine_name);
							patch->filename = QString(insInfo->instrument_file);
							patch->loadmode = (int)insInfo->load_mode;
							patch->volume = insInfo->volume;
							if(lscp_load_engine(_client, insInfo->engine_name, chan) == LSCP_OK)
							{
								LSCPKeymap kmap = _getKeyMapping(QString(insInfo->instrument_file), insInfo->instrument_nr, chan);
								patch->keys = kmap.key_bindings;
								patch->keyswitches = kmap.keyswitch_bindings;
							}
							pg->patches.push_back(patch);
						}
					}
					//Flush the disk streams used by the channel created
					::lscp_reset_channel(_client, chan);
					//Finally remove the channel
					::lscp_remove_channel(_client, chan);
					return midiInstr;
					//instruments->push_back(midiInstr);
				}//end load audio dev
			}//end create channel
		}//end maps
	}
	return 0;
}/*}}}*/

int LSClient::findMidiMap(const char* name)
{
	QString sname(name);
	QMap<int, QString> mapList = listInstruments();
	QMapIterator<int, QString> iter(mapList);
	while(iter.hasNext())
	{
		iter.next();
		if(iter.value() == sname)
			return iter.key();
	}
	return -1;
}

bool LSClient::loadInstrument(MidiInstrument* instrument)
{
	if(_client != NULL && instrument && instrument->isOOMInstrument())
	{
		int mapCount = ::lscp_get_midi_instrument_maps(_client);
		int mdev = ::lscp_get_midi_devices(_client);
		int mdevId = 0;
		int adev = ::lscp_get_audio_devices(_client);
		int adevId = 0;
		char sChannels[9] = "CHANNELS";
		char sPorts[6] = "PORTS";
		char sDevName[13] = "LinuxSampler";
		char sName[5] = "NAME";
		qDebug("Client connected ready to load instrument, mdev: %d, adev: %d", mdev, adev);
		if(!mapCount)
		{// Create a midi input device
			//CREATE MIDI_INPUT_DEVICE JACK NAME='LinuxSampler'
			qDebug("No MIDI device found creating one");
			lscp_param_t* params = new lscp_param_t[2];
			
			params[0].key = sName;
			params[0].value = sDevName;
			
			//params[1].key = sPorts;
			//params[1].value = (char*)"35";

			params[1].key = NULL;
			params[1].value = NULL;
			
			mdevId = ::lscp_create_midi_device(_client, "JACK", params); 
		
			//SET MIDI_INPUT_DEVICE_PARAMETER 0 PORTS=21
			if(mdevId >= 0)
			{
				qDebug("Created MIDI device with id: %d", mdevId);
			}
		
			//Create an audio output device
			//CREATE AUDIO_OUTPUT_DEVICE JACK ACTIVE=true CHANNELS=35 SAMPLERATE=48000 NAME='LinuxSampler'
			qDebug("No Audio device found creating one");
			lscp_param_t *aparams = new lscp_param_t[5];
			aparams[0].key = sName;
			aparams[0].value = sDevName;
			aparams[1].key = (char*)"ACTIVE";
			aparams[1].value = (char*)"true";
			aparams[2].key = (char*)"CHANNELS";
			aparams[2].value = (char*)"35";
			aparams[3].key = (char*)"SAMPLERATE";
			aparams[3].value = (char*)"48000";
			aparams[4].key = NULL;
			aparams[4].value = NULL;
			adevId = ::lscp_create_audio_device(_client, "JACK", aparams);
			if(adevId >= 0)
			{
				qDebug("Created audio channel with id: %d", adevId);
			}
		}
		if(mdevId != -1 && adevId != -1)
		{
			qDebug("Found MIDI and audio devices");
			QString channelFile;
			QString channelEngine;
			
			int map = findMidiMap(instrument->iname().toUtf8().constData());;
			if(map == -1)
			{
				qDebug("MIDI Map not found for %s, Creating MIDI Map ", instrument->iname().toUtf8().constData());
				//Create midiMap with the patchgroup name
				map = ::lscp_add_midi_instrument_map(_client, instrument->iname().toUtf8().constData());
				if(map >= 0)
				{
					PatchGroupList *pgl = instrument->groups();
					for(iPatchGroup ipg = pgl->begin(); ipg != pgl->end(); ++ipg)
					{
						const PatchList& patches = (*ipg)->patches;
						int index = 0;
						for(ciPatch ip = patches.begin(); ip != patches.end(); ++ip)
						{//Add instrument map for each patch
							Patch* p = *ip;
							lscp_midi_instrument_t* ins = (lscp_midi_instrument_t*)malloc(sizeof(lscp_midi_instrument_t));
							ins[0].map = map;
							ins[0].bank = p->lbank;
							ins[0].prog = p->prog;
							QString filename(p->filename);
							if(filename.contains(QString(SOUND_PATH)))
							{
								filename = filename.replace(QString(SOUND_PATH), SOUNDS_DIR);
							}
							qDebug("Loading patch: %s, engine: %s, filename: %s", p->name.toUtf8().constData(), p->engine.toUtf8().constData(), filename.toUtf8().constData());
							if(::lscp_map_midi_instrument(_client, ins, p->engine.toUtf8().constData(), filename.toUtf8().constData(), index, p->volume, (lscp_load_mode_t)p->loadmode, p->name.toUtf8().constData()) == LSCP_OK)
							{//
								qDebug("Loaded");
								//Select the first instrument to load into the channel
								if(ipg == pgl->begin() || channelFile.isEmpty() || channelEngine.isEmpty())
								{
									if(channelFile.isEmpty())
										channelFile = filename;
									if(channelEngine.isEmpty())
										channelEngine = p->engine;
								}
							}
							else
							{
								qDebug("Failed");
							}
							++index;
						}
					}
					//configure audio device port for instrument/*{{{*/
					//Find the first unconfigure channels and use them
					char midiPortName[instrument->iname().size()+1];
					strcpy(midiPortName, instrument->iname().toLocal8Bit().constData());
					char audioChannelName[QString(instrument->iname()).append("-audio").size()+1];
					strcpy(audioChannelName, QString(instrument->iname()).append("-audio").toLocal8Bit().constData());
					int midiPort = -1;
					int audioChannel = -1;

					if(mapCount)
					{
						qDebug("Increasing MIDI Device Port count");
						char pCount[QString::number(mapCount+1).size()+1];
						strcpy(pCount, QString::number(mapCount+1).toLocal8Bit().constData());
						lscp_param_t p;
						p.key = sPorts;
						p.value = pCount;
						if(lscp_set_midi_device_param(_client, mdevId, &p))
						{
							qDebug("Sucessfullt increased port count");
							midiPort = mapCount;
						}
						else
						{//find a free channel
							qDebug("Failed");
							midiPort = mapCount;
						}
					}
					else
					{
						midiPort = mapCount;
					}
					//set port name
					if(midiPort != -1)
					{
						lscp_param_t portName;
						portName.key = sName;
						portName.value = midiPortName;
						qDebug("mdevId: %d, midiPort: %d", mdevId, midiPort);
						if(lscp_set_midi_port_param(_client, mdevId, midiPort, &portName) != LSCP_OK)
						{
							qDebug("Failed to renamed midi port");
						}
					}
					else
					{
					}

					if(mapCount)
					{
						qDebug("Increasing Audio device channel count");
						audioChannel = mapCount;
						/*char pCount[QString::number(mapCount+1).size()+1];
						strcpy(pCount, QString::number(mapCount+1).toLocal8Bit().constData());
						lscp_param_t c;
						c.key = sChannels;
						c.value = pCount;
						if(lscp_set_audio_device_param(_client, adevId, &c) == LSCP_OK)
						{
							qDebug("Sucessfully increased count");
							audioChannel = mapCount;
						}
						else
						{
							qDebug("Failed");
						}*/
					}
					else
					{
						audioChannel = mapCount;
					}
					if(audioChannel != -1)
					{
						lscp_param_t chanName;
						chanName.key = sName;
						chanName.value = audioChannelName;
						if(lscp_set_audio_channel_param(_client, adevId, audioChannel, &chanName) != LSCP_OK)
						{
							qDebug("Failed to set Audio channel name");
						}
					}
					
					if(midiPort != -1 && audioChannel != -1)
					{
						qDebug("Creating LinuxSampler Channel");
						//Create Channels and load default map
						//ADD CHANNEL
						//LOAD ENGINE SFZ 0
						int chan = ::lscp_add_channel(_client);
						char cEngine[channelEngine.size()+1];
						strcpy(cEngine, channelEngine.toLocal8Bit().constData());
						if(chan >= 0)
						{
							qDebug("Created LinuxSampler Channel: %d", chan);
							qDebug("Loading Channel engine: %s", cEngine);
							if(lscp_load_engine(_client, cEngine, chan) == LSCP_OK)
							{
								//Setup midi side of channel
								//SET CHANNEL MIDI_INPUT_DEVICE 0 0
								//SET CHANNEL MIDI_INPUT_PORT 0 0
								//SET CHANNEL MIDI_INPUT_CHANNEL 0 ALL
								//SET CHANNEL MIDI_INSTRUMENT_MAP 0 0
								if(lscp_set_channel_midi_device(_client, chan, mdevId) != LSCP_OK)
								{
									qDebug("Failed to set channel MIDI input device");
								}
								if(lscp_set_channel_midi_port(_client, chan, midiPort) != LSCP_OK)
								{
									qDebug("Failed to set channel MIDI port");
								}
								if(lscp_set_channel_midi_channel(_client, chan, LSCP_MIDI_CHANNEL_ALL) != LSCP_OK)
								{
									qDebug("Failed to set channel MIDI channels");
								}
								if(lscp_set_channel_midi_map(_client, chan, map) != LSCP_OK)
								{
									qDebug("Failed to set channel MIDI instrument map");
								}
								//Set channel default volume
								//SET CHANNEL VOLUME 0 1.0
								if(lscp_set_channel_volume(_client, chan, 1.0) != LSCP_OK)
								{
									qDebug("Failed to set channel volume");
								}
								//SET CHANNEL AUDIO_OUTPUT_DEVICE 0 0
								//SET CHANNEL AUDIO_OUTPUT_CHANNEL 0 1 0
								//Setup audio side of channel
								if(lscp_set_channel_audio_device(_client, chan, adevId) != LSCP_OK)
								{
									qDebug("Failed to set channel AUDIO output device");
								}
								if(lscp_set_channel_audio_channel(_client, chan, 0, chan) != LSCP_OK)
								{
									qDebug("Failed to set channel AUDIO output channel 1");
								}
								if(lscp_set_channel_audio_channel(_client, chan, 1, chan) != LSCP_OK)
								{
									qDebug("Failed to set channel AUDIO output channel 2");
								}
								if(!channelFile.isEmpty())
								{
									char cFile[channelFile.size()+1];
									strcpy(cFile, channelFile.toLocal8Bit().constData());
									if(lscp_load_instrument_non_modal(_client, cFile, 0, chan) != LSCP_OK)
									{
										qDebug("Failed to load default sample into channel");
									}
								}
							}
						}
					}/*}}}*/
				}
			}//END if(map)
			return true;
		}
	}
	return false;
}

bool LSClient::isFreePort(const char* val)
{
	QString text(val);
	bool test;
	text.toInt(&test);
	if(test || text.startsWith("midi_in"))
		return true;
	return false;
}

bool LSClient::unloadInstrument(MidiInstrument*)
{
	if(_client != NULL)
	{
	}
	return false;
}

QMap<int, QString> LSClient::listInstruments()/*{{{*/
{
	QMap<int, QString> rv;
	if(_client != NULL)
	{
		int* maps = ::lscp_list_midi_instrument_maps(_client);
		if(maps != NULL)
		{
			for(int m = 0; maps[m] >= 0; ++m)
			{
				rv.insert(maps[m], getMapName(maps[m]));
			}
		}
	}
	return rv;
}/*}}}*/

/**
 * Lookup a midi map name by ID
 * @param int ID of the midi instrument map
 */
QString LSClient::getMapName(int mid)/*{{{*/
{
	QString mapName("Untitled");
	if(_client == NULL)
		return mapName;
	for(int i = 0; i < _retries && mapName == QString("Untitled"); ++i)
	{
		const char* cname = ::lscp_get_midi_instrument_map_name(_client, mid);
		if(cname != NULL)
		{
			mapName = QString(cname);
		}
	}
	return mapName;
}/*}}}*/

void LSClient::startThread()
{
	lsp().queueClient(this);
}

bool LSClient::resetSampler()
{
	bool rv = false;
	return rv;
}

bool LSClient::loadSamplerCommand(QString)
{
	return true;
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
//The following routines is borrowed from the qSampler project

static int _hexToNumber(char hex_digit) {/*{{{*/
    switch (hex_digit) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;

        case 'a': return 10;
        case 'b': return 11;
        case 'c': return 12;
        case 'd': return 13;
        case 'e': return 14;
        case 'f': return 15;

        case 'A': return 10;
        case 'B': return 11;
        case 'C': return 12;
        case 'D': return 13;
        case 'E': return 14;
        case 'F': return 15;

        default:  return 0;
    }
}/*}}}*/

static int _hexsToNumber(char hex_digit0, char hex_digit1) {
    return _hexToNumber(hex_digit1)*16 + _hexToNumber(hex_digit0);
}

QString LSClient::_stripAscii(QString str)/*{{{*/
{
	QRegExp regexp(QRegExp::escape("\\x") + "[0-9a-fA-F][0-9a-fA-F]");
	for(int c = 0; c < 4; ++c)
	{
		for (int i = str.indexOf(regexp); i >= 0; i = str.indexOf(regexp, i + 4))
		{
			const QString hexVal = str.mid(i+2, 2).toLower();
			char asciiTxt = _hexsToNumber(hexVal.at(1).toLatin1(), hexVal.at(0).toLatin1());
			str.replace(i, 4, asciiTxt);
		}
	}
	return str;
}/*}}}*/
//EnD qSampler

LSProcessor& lsp()
{
	static LSProcessor lsp;
	return lsp;
}

LSProcessor::LSProcessor()
{
	m_lsthread = new LSThread(this);
	m_taskRunning = false;
	m_runningClient = 0;
	
	moveToThread(m_lsthread);
	
	m_lsthread->start();

	connect(this, SIGNAL(newClientTask()), this, SLOT(startClientTask()), Qt::QueuedConnection);
}

LSProcessor::~LSProcessor()
{
	m_lsthread->exit(0);
	if(!m_lsthread->wait(1000))
	{
		m_lsthread->terminate();
	}
	delete m_lsthread;
}

void LSProcessor::startClientTask()
{
//TODO: Implement a task based system that we can later use to send channel resets on demand
}

void LSProcessor::queueClient(LSClient* c)
{
	QMutexLocker locker(&m_mutex);
	m_queue.enqueue(c);

	if(!m_taskRunning)
	{
		dequeueClient();
	}
}

void LSProcessor::dequeueClient()
{
	if(!m_queue.isEmpty())
	{
		m_taskRunning = true;
		m_runningClient = m_queue.dequeue();
		emit newClientTask();
	}
}

void LSProcessor::freeClient(LSClient* c)
{
	m_mutex.lock();
	m_queue.removeAll(c);
	if(c == m_runningClient)
	{
		m_wait.wait(&m_mutex);
		dequeueClient();
		return;
	}
	m_mutex.unlock();
	delete c;
}

LSThread::LSThread(LSProcessor* lsp)
{
	m_lsp = lsp;
}

void LSThread::run()
{
	exec();
}
#endif
