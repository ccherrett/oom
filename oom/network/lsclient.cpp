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
static char sChannels[9] = "CHANNELS";
static char sPorts[6] = "PORTS";
static char sDevName[13] = "LinuxSampler";
static char sName[5] = "NAME";
static char sSampleRate[11] = "SAMPLERATE";

LSClient::LSClient(QString host, int p, QObject* parent) : QObject(parent)
{
	_hostname = host;
	_port = p;
	_abort = false;
	_client = NULL;
	_retries = 5;
	_timeout = 1;
	_useBankNumber = true;
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
	qDebug("LSClient::startClient: hostname: %s, port: %d", _hostname.toUtf8().constData(),  _port);
	_client = ::lscp_client_create(_hostname.toUtf8().constData(), _port, client_callback, this);
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
		//Check if linuxsampler is running at all
		qDebug("Failed to Initialize LSCP client connection, retrying");
		_client = ::lscp_client_create(_hostname.toUtf8().constData(), _port, client_callback, this);
		_lastInfo.valid = false;
		if(_client != NULL)
			qDebug("Retry sucessfull");
		else
			qDebug("Retry failed!!!");
		return _client != NULL;
	}
}/*}}}*/

bool LSClient::isClientStarted()
{
	bool rv = false;
	if(_client != NULL)
	{//Run a test to see if the sampler is really there
		lscp_server_info_t* info = ::lscp_get_server_info(_client);
		if(info != NULL)
		{
			rv = true;
		}
	}
	return rv;
}

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
		if(pMaps >= 0)
		{
			//Create a channel
			int chan = ::lscp_add_channel(_client);
			if(chan >= 0)
			{
				::lscp_load_engine(_client, "GIG", chan);
				//Get audio channels
				int adev =  ::lscp_get_audio_devices(_client);
				int mdev = ::lscp_get_midi_devices(_client);
				if(!adev)
				{
					//qDebug("LSClient::getInstrument: No Audio Device found~~~~~~~~~~~~~~~~Creating one");
					createAudioOutputDevice(sDevName, "JACK", 1, 48000);
				}
				if(!mdev)
				{
					//qDebug("LSClient::getInstrument: No MIDI Device found~~~~~~~~~~~~~~~~Creating one");
					createMidiInputDevice(sDevName, "JACK", 1);	
				}
				if(lscp_set_channel_audio_device(_client, chan, 0) == LSCP_OK)
				{
					QString mapName = getMapName(pMaps);
					QString insName(getValidInstrumentName(mapName));
					MidiInstrument *midiInstr = new MidiInstrument(insName);
					MidiController *modCtrl = new MidiController("Modulation", CTRL_MODULATION, 0, 127, 0);
					MidiController *expCtrl = new MidiController("Expression", CTRL_EXPRESSION, 0, 127, 0);
					midiInstr->setDefaultPan(0.0);
					midiInstr->setDefaultVerb(config.minSlider);
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
							QString bname(QString(tr("Bank ")).append(QString::number(instr[in].bank+1)));
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
								if(_useBankNumber)
									pg->name = bname;
								else
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
							patch->index = insInfo->instrument_nr;
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
				else
				{
					return 0;
					//TODO: emit errorOccurrect(QString type);
				}
				//Flush the disk streams used by the channel created
				::lscp_reset_channel(_client, chan);
				//Finally remove the channel
				::lscp_remove_channel(_client, chan);
			}//end create channel
			else
			{
				return 0;
				//TODO: emit errorOccurrect(QString type);
			}
		}//end maps
	}
	return 0;
}/*}}}*/

int LSClient::findMidiMap(const char* name)/*{{{*/
{
	int rv = -1;
	if(_client != NULL)
	{
		QString sname(name);
		QMap<int, QString> mapList = listInstruments();
		QMapIterator<int, QString> iter(mapList);
		while(iter.hasNext())
		{
			iter.next();
			if(iter.value() == sname)
			{
				rv = iter.key();
				break;
			}
		}
	}
	return rv;
}/*}}}*/

int LSClient::createAudioOutputDevice(char* name, const char* type, int ports, int iSrate)/*{{{*/
{
	int rv = -1;
	if(_client != NULL)
	{
		QString sport = QString::number(ports);
		char cports[sport.size()+1];
		strcpy(cports, sport.toUtf8().constData());
		
		QString srate = QString::number(iSrate);
		char crate[srate.size()+1];
		strcpy(crate, srate.toUtf8().constData());

		lscp_param_t *aparams = new lscp_param_t[5];
		aparams[0].key = sName;
		aparams[0].value = name;
		aparams[1].key = (char*)"ACTIVE";
		aparams[1].value = (char*)"true";
		aparams[2].key = sChannels;
		aparams[2].value = cports;
		aparams[3].key = sSampleRate;
		aparams[3].value = crate;
		aparams[4].key = NULL;
		aparams[4].value = NULL;
		
		if(_client)
			rv = ::lscp_create_audio_device(_client, type, aparams);
		else if(startClient())
		{
			rv = ::lscp_create_audio_device(_client, type, aparams);
		}
	}
	return rv;
}/*}}}*/

int LSClient::createMidiInputDevice(char* name, const char* type, int ports)/*{{{*/
{
	int rv = -1;
	if(_client != NULL)
	{
		QString sport = QString::number(ports);
		char cports[sport.size()+1];
		strcpy(cports, sport.toUtf8().constData());
		
		lscp_param_t* params = new lscp_param_t[3];
		params[0].key = sName;
		params[0].value = name;
		params[1].key = sPorts;
		params[1].value = cports;
		params[2].key = NULL;
		params[2].value = NULL;
		
		rv = ::lscp_create_midi_device(_client, type, params); 
	}
	return rv;
}/*}}}*/

bool LSClient::createInstrumentChannel(const char* name, const char* engine, const char* filename, int index, int map, SamplerData** data)/*{{{*/
{
	bool rv = false;
	if(_client != NULL)
	{
		SamplerData *sd = new SamplerData;
		sd->midiDevice = 0; //TODO: Implement round robin
		sd->audioDevice = 0; //TODO: Implement round robin
		if(debugMsg)
			qDebug("LSClient::createInstrumentChannel: name: %s, engine: %s,  filename: %s, map: %d", 
					name, engine, QString(filename).replace(QString(SOUND_PATH), SOUNDS_DIR).toUtf8().constData(), map);
		//configure audio device port for instrument
		//Find the first unconfigure channels and use them
		char midiPortName[strlen(name)+1];
		strcpy(midiPortName, name);
		char audioChannelName[QString(name).append("-audio").size()+1];
		strcpy(audioChannelName, QString(name).append("-audio").toLocal8Bit().constData());
		int midiPort = getFreeMidiInputPort(0);
		int audioChannel = getFreeAudioOutputChannel(0);

		int midiPortCount = -1;
		lscp_device_info_t* mDevInfo = ::lscp_get_midi_device_info(_client, 0);
		if(mDevInfo)
		{
			lscp_param_t* mDevParams = mDevInfo->params;
			for(int i = 0; mDevParams && mDevParams[i].key && mDevParams[i].value; ++i)
			{
				if(strcmp(mDevParams[i].key, sPorts) == 0)
				{
					midiPortCount = atoi(mDevParams[i].value);
					break;
				}
			}
		}

		if(midiPort == -1)
		{
			if(debugMsg)
				qDebug("Increasing MIDI Device Port count");
			char pCount[QString::number(midiPortCount+1).size()+1];
			strcpy(pCount, QString::number(midiPortCount+1).toLocal8Bit().constData());
			lscp_param_t p;
			p.key = sPorts;
			p.value = pCount;
			if(lscp_set_midi_device_param(_client, 0, &p) == LSCP_OK)
			{
				if(debugMsg)
					qDebug("LSClient::createInstrumentChannel: Sucessfullt increased port count");
			}
			else
			{//find a free channel
				qDebug("LSClient::createInstrumentChannel: Failed to increase channel count");
			}
			midiPort = midiPortCount;
		}
		//set port name
		if(midiPort != -1)
		{
			if(!renameMidiPort(midiPort, QString(midiPortName), 0))
				qDebug("LSClient::createInstrumentChannel: Failed to renamed midi port");
		}

		//Prepare audio device chanell
		int audioChannelCount = -1;
		lscp_device_info_t* aDevInfo = ::lscp_get_audio_device_info(_client, 0);
		if(aDevInfo)
		{
			lscp_param_t* aDevParams = aDevInfo->params;
			for(int i = 0; aDevParams && aDevParams[i].key && aDevParams[i].value; ++i)
			{
				if(strcmp(aDevParams[i].key, sChannels) == 0)
				{
					audioChannelCount = atoi(aDevParams[i].value);
					break;
				}
			}
		}
		if(audioChannel == -1)
		{
			if(debugMsg)
				qDebug("LSClient::createInstrumentChannel: Increasing Audio device channel count");
			char pCount[QString::number(audioChannelCount+1).size()+1];
			strcpy(pCount, QString::number(audioChannelCount+1).toLocal8Bit().constData());
			lscp_param_t c;
			c.key = sChannels;
			c.value = pCount;
			if(lscp_set_audio_device_param(_client, 0, &c) == LSCP_OK)
			{
				if(debugMsg)
					qDebug("LSClient::createInstrumentChannel: Sucessfully increased channel count");
			}
			else
			{
				qDebug("LSClient::createInstrumentChannel: Failed to instrease channel count");
			}
			audioChannel = audioChannelCount;
		}
		if(audioChannel != -1)
		{
			if(!renameAudioChannel(audioChannel, QString(audioChannelName), 0))
				qDebug("LSClient::createInstrumentChannel: Failed to set Audio channel name");
		}
		if(debugMsg)
			qDebug("LSClient::createInstrumentChannel: midiPort: %d, audioChanel: %d", midiPort, audioChannel);
		
		if(midiPort != -1 && audioChannel != -1)
		{
			sd->midiPort = midiPort;
			sd->audioChannel = audioChannel;
			if(debugMsg)
				qDebug("LSClient::createInstrumentChannel: Creating LinuxSampler Channel");
			//Create Channels and load default map
			//ADD CHANNEL
			//LOAD ENGINE SFZ 0
			int chan = ::lscp_add_channel(_client);
			char cEngine[strlen(engine)+1];
			strcpy(cEngine, engine);
			if(chan >= 0)
			{
				sd->samplerChannel = chan;
				*data = sd;
				if(debugMsg)
				{
					qDebug("LSClient::createInstrumentChannel: Created LinuxSampler Channel: %d", chan);
					qDebug("LSClient::createInstrumentChannel: Loading Channel engine: %s", cEngine);
				}
				if(lscp_load_engine(_client, cEngine, chan) == LSCP_OK)
				{
					//Setup midi side of channel
					//SET CHANNEL MIDI_INPUT_DEVICE 0 0
					//SET CHANNEL MIDI_INPUT_PORT 0 0
					//SET CHANNEL MIDI_INPUT_CHANNEL 0 ALL
					//SET CHANNEL MIDI_INSTRUMENT_MAP 0 0
					if(lscp_set_channel_midi_device(_client, chan, 0) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel MIDI input device");
					}
					if(lscp_set_channel_midi_port(_client, chan, midiPort) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel MIDI port");
					}
					if(lscp_set_channel_midi_channel(_client, chan, LSCP_MIDI_CHANNEL_ALL) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel MIDI channels");
					}
					if(lscp_set_channel_midi_map(_client, chan, map) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel MIDI instrument map");
					}
					//Set channel default volume
					//SET CHANNEL VOLUME 0 1.0
					if(lscp_set_channel_volume(_client, chan, 1.0) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel volume");
					}
					//SET CHANNEL AUDIO_OUTPUT_DEVICE 0 0
					//SET CHANNEL AUDIO_OUTPUT_CHANNEL 0 1 0
					//Setup audio side of channel
					if(lscp_set_channel_audio_device(_client, chan, 0) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel AUDIO output device");
					}
					if(lscp_set_channel_audio_channel(_client, chan, 0, audioChannel) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel AUDIO output channel 1");
					}
					if(lscp_set_channel_audio_channel(_client, chan, 1, audioChannel) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to set channel AUDIO output channel 2");
					}
					QString file(filename);
					if(file.contains(QString(SOUND_PATH)))
					{
						file = file.replace(QString(SOUND_PATH), SOUNDS_DIR);
					}
					char cFile[file.size()+1];
					strcpy(cFile, file.toUtf8().constData());
					if(lscp_load_instrument_non_modal(_client, cFile, index, chan) != LSCP_OK)
					{
						qDebug("LSClient::createInstrumentChannel: Failed to load default sample into channel");
					}
					rv = true;
				}
				else
				{
					rv = false;
				}
			}
			else
			{
				rv = false;
			}
		}
		else
		{
			rv = false;
		}
	}
	return rv;
}/*}}}*/

bool LSClient::updateInstrumentChannel(SamplerData* data, const char* engine, const char* filename, int index, int map)/*{{{*/
{
	bool rv = false;
	if(_client != NULL && data)
	{
		if(debugMsg)
			qDebug("LSClient::updateInstrumentChannel: engine: %s,  filename: %s, map: %d", 
				engine, QString(filename).replace(QString(SOUND_PATH), SOUNDS_DIR).toUtf8().constData(), map);
		//configure audio device port for instrument
		//Find the first unconfigure channels and use them
		int midiPort = data->midiPort;
		int audioChannel = data->audioChannel;

		if(midiPort != -1 && audioChannel != -1)
		{
			//Create Channels and load default map
			//ADD CHANNEL
			//LOAD ENGINE SFZ 0

			int chan = data->samplerChannel;
			lscp_channel_info_t* chanInfo = ::lscp_get_channel_info(_client, chan);
			if(!chanInfo)
				return false;
			char cEngine[strlen(engine)+1];
			strcpy(cEngine, engine);
			if(chan >= 0)
			{
				if(debugMsg)
				{
					qDebug("LSClient::updateInstrumentChannel: Updating LinuxSampler Channel: %d", chan);
					qDebug("LSClient::updateInstrumentChannel: Loading Channel engine: %s", cEngine);
				}
				if(lscp_load_engine(_client, cEngine, chan) == LSCP_OK)
				{
					//Setup midi side of channel
					if(lscp_set_channel_midi_map(_client, chan, map) != LSCP_OK)
					{
						qDebug("LSClient::updateInstrumentChannel: Failed to set channel MIDI instrument map");
					}
					QString file(filename);
					if(file.contains(QString(SOUND_PATH)))
					{
						file = file.replace(QString(SOUND_PATH), SOUNDS_DIR);
					}
					char cFile[file.size()+1];
					strcpy(cFile, file.toUtf8().constData());
					if(lscp_load_instrument_non_modal(_client, cFile, index, chan) != LSCP_OK)
					{
						qDebug("LSClient::updateInstrumentChannel: Failed to load default sample into channel");
					}
					rv = true;
				}
				else
				{
					rv = false;
				}
			}
			else
			{
				rv = false;
			}
		}
		else
		{
			rv = false;
		}
	}
	return rv;
}/*}}}*/

bool LSClient::removeInstrumentChannel(SamplerData* data)/*{{{*/
{
	bool rv = false;
	if(_client != NULL && data)
	{
		if(debugMsg)
			qDebug("LSClient::removeInstrumentChannel: Channel: %d", data->samplerChannel);
		//configure audio device port for instrument
		//Find the first unconfigure channels and use them
		int midiPort = data->midiPort;
		int audioChannel = data->audioChannel;

		if(midiPort != -1 && audioChannel != -1)
		{
			int chan = data->samplerChannel;
			if(lscp_remove_channel(_client, chan) == LSCP_OK)
			{
				//Reset the midi port and audio channel
				renameMidiPort(midiPort, QString("midi_in_").append(QString::number(midiPort)), data->midiDevice);
				renameAudioChannel(audioChannel, QString::number(audioChannel), data->audioDevice);
			}
			else
			{
				rv = false;
			}
		}
		else
		{
			rv = false;
		}
	}
	return rv;
}/*}}}*/

bool LSClient::loadInstrument(MidiInstrument* instrument)/*{{{*/
{
	bool rv = false;
	if(_client != NULL && instrument && instrument->isOOMInstrument())
	{
		int mapCount = ::lscp_get_midi_instrument_maps(_client);
		//int mdev = ::lscp_get_midi_devices(_client);
		int mdevId = 0;
		//int adev = ::lscp_get_audio_devices(_client);
		int adevId = 0;
		//qDebug("Client connected ready to load instrument, mdev: %d, adev: %d", mdev, adev);
		if(!mapCount)
		{// Create a midi input device
			//CREATE MIDI_INPUT_DEVICE JACK NAME='LinuxSampler'
			//qDebug("No MIDI device found creating one");
			mdevId = createMidiInputDevice(sDevName, "JACK", 1);	
			//SET MIDI_INPUT_DEVICE_PARAMETER 0 PORTS=21
		
			//Create an audio output device
			//CREATE AUDIO_OUTPUT_DEVICE JACK ACTIVE=true CHANNELS=35 SAMPLERATE=48000 NAME='LinuxSampler'
			adevId = createAudioOutputDevice(sDevName, "JACK", 1, 48000);
		}
		if(mdevId != -1 && adevId != -1)
		{
			if(debugMsg)
				qDebug("LSClient::loadInstrument: Found MIDI and audio devices");
			QString channelFile;
			QString channelEngine;
			
			int map = findMidiMap(instrument->iname().toUtf8().constData());;
			if(map == -1)
			{
				if(debugMsg)
					qDebug("LSClient::loadInstrument: MIDI Map not found for %s, Creating MIDI Map ", instrument->iname().toUtf8().constData());
				//Create midiMap with the patchgroup name
				map = ::lscp_add_midi_instrument_map(_client, instrument->iname().toUtf8().constData());
				if(map >= 0)
				{
					PatchGroupList *pgl = instrument->groups();/*{{{*/
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
							if(debugMsg)
								qDebug("LSClient::loadInstrument: Loading patch: %s, engine: %s, filename: %s", p->name.toUtf8().constData(), p->engine.toUtf8().constData(), filename.toUtf8().constData());
							if(::lscp_map_midi_instrument(_client, ins, p->engine.toUtf8().constData(), filename.toUtf8().constData(), p->index, p->volume, (lscp_load_mode_t)p->loadmode, p->name.toUtf8().constData()) == LSCP_OK)
							{//
								if(debugMsg)
									qDebug("LSClient::loadInstrument: Patch Loaded");
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
								if(debugMsg)
									qDebug("LSClient::loadInstrument: Patch Load Failed");
							}
							++index;
						}
					}/*}}}*/
				}
			}//END if(map)
			rv = true;
		}
	}
	return rv;
}/*}}}*/

//Used by the add track mechanism to flush stuff created with the 
//Instrument combo box
void LSClient::removeLastChannel()/*{{{*/
{
	//This function is dangerous and is deprecated
	return;
	if(_client != NULL)
	{
		int channelCount = ::lscp_get_channels(_client);
		if(channelCount)
		{
			int chan = channelCount-1;
			//Remove the last one created
			if(lscp_remove_channel(_client, chan) == LSCP_OK)
			{
				//Reduce the MIDI and audio device port and channel count or rename if 0
				if(chan)
				{
					char mpCount[QString::number(chan).size()+1];
					strcpy(mpCount, QString::number(chan).toLocal8Bit().constData());
					lscp_param_t p;
					p.key = sPorts;
					p.value = mpCount;
					if(lscp_set_midi_device_param(_client, 0, &p) != LSCP_OK)
						qDebug("Failed to reduce MIDI device port count");
					
					char apCount[QString::number(chan).size()+1];
					strcpy(apCount, QString::number(chan).toLocal8Bit().constData());
					lscp_param_t c;
					c.key = sChannels;
					c.value = apCount;
					if(lscp_set_audio_device_param(_client, 0, &c) != LSCP_OK)
						qDebug("Failed to reduce audio device channel count");
				}
				else
				{//Just rename the first ports
					//TODO: maybe we should just reset the sample here instead
					resetSampler();
					/*lscp_param_t portName;
					portName.key = sName;
					portName.value = (char*)"midi_in_0";
					if(lscp_set_midi_port_param(_client, 0, chan, &portName) != LSCP_OK)
						qDebug("Failed to rename midi port");
					
					lscp_param_t chanName;
					chanName.key = sName;
					chanName.value = (char*)"0";
					if(lscp_set_audio_channel_param(_client, 0, chan, &chanName) != LSCP_OK)
						qDebug("Faled to rename audio channel");
					*/
				}
			}
		}
	}
}/*}}}*/

bool LSClient::renameMidiPort(int port, QString newName, int mdev)/*{{{*/
{
	bool rv = false;
	if(_client != NULL)
	{
		lscp_device_info_t* mDevInfo = ::lscp_get_midi_device_info(_client, mdev);
		if(mDevInfo)
		{
			lscp_device_port_info_t* portInfo = ::lscp_get_midi_port_info(_client, mdev, port);
			if(portInfo)
			{
				char pName[newName.size()+1];
				strcpy(pName, newName.toLocal8Bit().constData());
				lscp_param_t portName;
				portName.key = sName;
				portName.value = pName;
				if(lscp_set_midi_port_param(_client, mdev, port, &portName) == LSCP_OK)
				{
					if(debugMsg)
						qDebug("LSClient::renameMidiPort: Sucessfully renamed midi port");
					rv = true;
				}
			}
		}
	}
	return rv;
}/*}}}*/

bool LSClient::renameAudioChannel(int chan, QString newName, int adev)/*{{{*/
{
	bool rv = false;
	if(_client != NULL)
	{
		lscp_device_info_t* aDevInfo = ::lscp_get_audio_device_info(_client, adev);
		if(aDevInfo)
		{
			lscp_device_port_info_t* portInfo = ::lscp_get_audio_channel_info(_client, adev, chan);
			if(portInfo)
			{
				char cName[newName.size()+1];
				strcpy(cName, newName.toLocal8Bit().constData());
				lscp_param_t chanName;
				chanName.key = sName;
				chanName.value = cName;
				if(lscp_set_audio_channel_param(_client, adev, chan, &chanName) == LSCP_OK)
				{
					rv = true;
					if(debugMsg)
						qDebug("Sucessfullt renamed audio channel");
				}
			}
		}
	}
	return rv;
}/*}}}*/

int LSClient::getFreeMidiInputPort(int mdev)/*{{{*/
{
	int rv = -1;
	if(_client)
	{
		lscp_device_info_t* mDevInfo = ::lscp_get_midi_device_info(_client, mdev);
		int midiPortCount = 0;
		if(mDevInfo)
		{
			lscp_param_t* mDevParams = mDevInfo->params;
			for(int i = 0; mDevParams && mDevParams[i].key && mDevParams[i].value; ++i)
			{
				if(strcmp(mDevParams[i].key, sPorts) == 0)
				{
					midiPortCount = atoi(mDevParams[i].value);
					break;
				}
			}
			bool found = false;
			for(int c = 0; c < midiPortCount; c++)
			{
				lscp_device_port_info_t* portInfo = ::lscp_get_midi_port_info(_client, mdev, c);
				if(portInfo)
				{
					lscp_param_t* mPortParams = portInfo->params;
					for(int i = 0; mPortParams && mPortParams[i].key && mPortParams[i].value; ++i)
					{
						if(strcmp(mPortParams[i].key, sName) == 0)
						{
							//if(strcmp(mPortParams[i].value, "midi_in_0") == 0)
							QString value(mPortParams[i].value);
							if(value.startsWith("midi_in_"))
							{
								rv = c;
								found = true;
							}
							break;
						}
					}
				}
				if(found)
					break;
			}
		}
	}
	if(debugMsg)
		qDebug("LSClient::getFreeMidiInputPort: midiPort: %d", rv);
	return rv;
}/*}}}*/

int LSClient::getFreeAudioOutputChannel(int adev)/*{{{*/
{
	int rv = -1;
	if(_client)
	{
		int channelCount = 0;
		lscp_device_info_t* aDevInfo = ::lscp_get_audio_device_info(_client, adev);
		if(aDevInfo)
		{
			lscp_param_t* aDevParams = aDevInfo->params;
			for(int i = 0; aDevParams && aDevParams[i].key && aDevParams[i].value; ++i)
			{
				if(strcmp(aDevParams[i].key, sChannels) == 0)
				{
					channelCount = atoi(aDevParams[i].value);
					break;
				}
			}
			bool found = false;
			for(int c = 0; c < channelCount; ++c)
			{
				lscp_device_port_info_t* portInfo = ::lscp_get_audio_channel_info(_client, adev, c);
				if(portInfo)
				{
					lscp_param_t* aPortParams = portInfo->params;
					for(int i = 0; aPortParams && aPortParams[i].key && aPortParams[i].value; ++i)
					{
						if(strcmp(aPortParams[i].key, sName) == 0)
						{
							QString value(aPortParams[i].value);
							bool ok;
							value.toInt(&ok);
							if(ok)
							{
								rv = c;
								found = true;
							}
							else if(strcmp(aPortParams[i].value, "0") == 0)
							{
								rv = c;
								found = true;
							}
							break;
						}
					}
				}
				if(found)
					break;
			}
		}
	}
	if(debugMsg)
		qDebug("LSClient::getFreeAudioOutputChannel: channel: %d ", rv);
	return rv;
}/*}}}*/

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

bool LSClient::removeMidiMap(int map)
{
	if(_client != NULL)
	{
		return (lscp_remove_midi_instrument_map(_client, map) == LSCP_OK);
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

bool LSClient::resetSampler()
{
	bool rv = false;
	if(_client != NULL)
	{
		rv = (lscp_reset_sampler(_client) == LSCP_OK);
	}
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

#endif
