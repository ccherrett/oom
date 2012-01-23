//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "TrackManager.h"
#include "globals.h"
#include "gconfig.h"
#include "track.h"
#include "song.h"
#include "app.h"
#include "mididev.h"
#include "midiport.h"
#include "minstrument.h"
#include "synth.h"
#include "audio.h"
#include "midiseq.h"
#include "driver/jackaudio.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "network/lsclient.h"
#include "icons.h"
#include "midimonitor.h"
#include "plugin.h"


TrackManager::TrackManager()
{
	m_insertPosition = -1;
	m_midiInPort = -1;
	m_midiOutPort = -1;

	m_allChannelBit = (1 << MIDI_CHANNELS) - 1;
}

void TrackManager::setPosition(int v)
{
	m_insertPosition = v;
}

//Add button slot
bool TrackManager::addTrack(VirtualTrack* vtrack)/*{{{*/
{
	bool rv = false;
	if(!vtrack || vtrack->name.isEmpty())
		return rv;
	
	/*int inputIndex = cmbInput->currentIndex();
	int outputIndex = cmbOutput->currentIndex();
	int instrumentIndex = cmbInstrument->currentIndex();
	int monitorIndex = cmbMonitor->currentIndex();
	int inChanIndex = cmbInChannel->currentIndex();
	int outChanIndex = cmbOutChannel->currentIndex();*/

	Track::TrackType type = (Track::TrackType)vtrack->type;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			//Load up the instrument first
			loadInstrument(vtrack);
			Track* track =  song->addTrackByName(vtrack->name, Track::MIDI, m_insertPosition, false);
			if(track)
			{
				MidiTrack* mtrack = (MidiTrack*)track;
				//Process Input connections
				if(vtrack->useInput)
				{
					QString devname = vtrack->inputConfig.second;
					MidiPort* inport = 0;
					MidiDevice* indev = 0;
					QString inputDevName(QString("I-").append(track->name()));
					if(vtrack->createMidiInputDevice)
					{
						m_midiInPort = getFreeMidiPort();
						qDebug("createMidiInputDevice is set: %i", m_midiInPort);
						inport = &midiPorts[m_midiInPort];
						int devtype = vtrack->inputConfig.first;
						if(devtype == MidiDevice::ALSA_MIDI)
						{
							indev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
							if(indev)
							{
								qDebug("Found MIDI input device: ALSA_MIDI");
								int openFlags = 0;
								openFlags ^= 0x2;
								indev->setOpenFlags(openFlags);
								midiSeq->msgSetMidiDevice(inport, indev);
							}
						}
						else if(devtype == MidiDevice::JACK_MIDI)
						{
							indev = MidiJackDevice::createJackMidiDevice(inputDevName, 3);
							if(indev)
							{
								qDebug("Created MIDI input device: JACK_MIDI");
								int openFlags = 0;
								openFlags ^= 0x2;
								indev->setOpenFlags(openFlags);
								midiSeq->msgSetMidiDevice(inport, indev);
							}
						}
						if(indev && indev->deviceType() == MidiDevice::JACK_MIDI)
						{
							qDebug("MIDI input device configured, Adding input routes to MIDI port");
							Route srcRoute(devname, false, -1, Route::JACK_ROUTE);
							Route dstRoute(indev, -1);

							audio->msgAddRoute(srcRoute, dstRoute);

							audio->msgUpdateSoloStates();
							song->update(SC_ROUTE);
						}
					}
					else
					{
						m_midiInPort = vtrack->inputConfig.first;
						qDebug("Using existing MIDI port: %i", m_midiInPort);
						inport = &midiPorts[m_midiInPort];
						if(inport)
							indev = inport->device();
					}
					if(inport && indev)
					{
						qDebug("MIDI Input port and MIDI devices found, Adding final input routing");
						int chanbit = vtrack->inputChannel;
						Route srcRoute(m_midiInPort, chanbit);
						Route dstRoute(track, chanbit);

						audio->msgAddRoute(srcRoute, dstRoute);

						audio->msgUpdateSoloStates();
						song->update(SC_ROUTE);
					}
				}
				
				//Process Output connections
				if(vtrack->useOutput)
				{
					MidiPort* outport= 0;
					MidiDevice* outdev = 0;
					QString devname = vtrack->outputConfig.second;
					QString outputDevName(QString("O-").append(track->name()));
					if(vtrack->createMidiOutputDevice)
					{
						m_midiOutPort = getFreeMidiPort();
						qDebug("m_createMidiOutputDevice is set: %i", m_midiOutPort);
						outport = &midiPorts[m_midiOutPort];
						int devtype = vtrack->outputConfig.first;
						if(devtype == MidiDevice::ALSA_MIDI)
						{
							outdev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
							if(outdev)
							{
								qDebug("Found MIDI output device: ALSA_MIDI");
								int openFlags = 0;
								openFlags ^= 0x1;
								outdev->setOpenFlags(openFlags);
								midiSeq->msgSetMidiDevice(outport, outdev);
							}
						}
						else if(devtype == MidiDevice::JACK_MIDI)
						{
							outdev = MidiJackDevice::createJackMidiDevice(outputDevName, 3);
							if(outdev)
							{
								qDebug("Created MIDI output device: JACK_MIDI");
								int openFlags = 0;
								openFlags ^= 0x1;
								outdev->setOpenFlags(openFlags);
								midiSeq->msgSetMidiDevice(outport, outdev);
								oom->changeConfig(true);
								song->update();
							}
						}
						if(outdev && outdev->deviceType() == MidiDevice::JACK_MIDI)
						{
							qDebug("MIDI output device configured, Adding output routes to MIDI port");
							Route srcRoute(outdev, -1);
							Route dstRoute(devname, true, -1, Route::JACK_ROUTE);

							qDebug("Device name from combo: %s, from dev: %s", devname.toUtf8().constData(), outdev->name().toUtf8().constData());

							audio->msgAddRoute(srcRoute, dstRoute);

							audio->msgUpdateSoloStates();
							song->update(SC_ROUTE);
						}
					}
					else
					{
						m_midiOutPort = vtrack->outputConfig.first;
						qDebug("Using existing MIDI output port: %i", m_midiOutPort);
						outport = &midiPorts[m_midiOutPort];
						if(outport)
							outdev = outport->device();
					}
					if(outport && outdev)
					{
						qDebug("MIDI output port and MIDI devices found, Adding final output routing");
						audio->msgIdle(true);
						mtrack->setOutPortAndUpdate(m_midiOutPort);
						int outChan = vtrack->outputChannel;
						mtrack->setOutChanAndUpdate(outChan);
						audio->msgIdle(false);
						if(vtrack->createMidiOutputDevice)
						{
							QString instrumentName = vtrack->instrumentName;
							for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
							{
								if ((*i)->iname() == instrumentName)
								{
									outport->setInstrument(*i);
									break;
								}
							}
						}
						song->update(SC_MIDI_TRACK_PROP);
					}
				}

				if(vtrack->useMonitor)
				{
					createMonitorInputTracks(vtrack);
				}

				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
				rv = true;
			}
		}
		break;
		case Track::WAVE:
		{
			Track* track =  song->addTrackByName(vtrack->name, Track::WAVE, m_insertPosition, !vtrack->useOutput);
			if(track)
			{
				if(vtrack->useInput)
				{
					QString inputName = QString("i").append(track->name());
					QString selectedInput = vtrack->inputConfig.second;
					bool addNewRoute = vtrack->inputConfig.first;
					Track* input = 0;
					if(addNewRoute)
						input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false);
					else
						input = song->findTrack(selectedInput);
					if(input)
					{
						if(addNewRoute)
						{
							input->setMute(false);
							//Connect jack port to Input track
							Route srcRoute(selectedInput, false, -1, Route::JACK_ROUTE);
							Route dstRoute(input, 0);
							srcRoute.channel = 0;
							
							Route srcRoute2(selectedInput, false, -1, Route::JACK_ROUTE);
							srcRoute2.channel = 1;
							Route dstRoute2(input, 1);

							audio->msgAddRoute(srcRoute, dstRoute);
							audio->msgAddRoute(srcRoute2, dstRoute2);

							audio->msgUpdateSoloStates();
							song->update(SC_ROUTE);
						}
						
						//Connect input track to audio track
						Route srcRoute(input->name(), true, -1);
						Route dstRoute(track, 0, track->channels());

						audio->msgAddRoute(srcRoute, dstRoute);

						audio->msgUpdateSoloStates();
						song->update(SC_ROUTE);
					}
				}
				if(vtrack->useOutput)
				{
					//Route to the Output or Buss
					QString selectedOutput = vtrack->outputConfig.second;
					Route srcRoute(track, 0, track->channels());
					Route dstRoute(selectedOutput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
				rv = true;
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			Track* track = song->addTrackByName(vtrack->name, Track::AUDIO_OUTPUT, -1, false);
			if(track)
			{
				if(vtrack->useInput)
				{
					QString selectedInput = vtrack->inputConfig.second;
					Route dstRoute(track, 0, track->channels());
					Route srcRoute(selectedInput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}

				if(vtrack->useOutput)
				{
					QString jackPlayback("system:playback");
					QString selectedOutput = vtrack->outputConfig.second;
					bool systemOutput = false;
					if(selectedOutput.startsWith(jackPlayback))
					{
						systemOutput = true;
					
						//Route channel 1
						Route srcRoute(track, 0);
						Route dstRoute(QString(jackPlayback).append("_1"), true, -1, Route::JACK_ROUTE);
						dstRoute.channel = 0;

						audio->msgAddRoute(srcRoute, dstRoute);

						//Route channel 2
						Route srcRoute2(track, 1);
						Route dstRoute2(QString(jackPlayback).append("_2"), true, -1, Route::JACK_ROUTE);
						dstRoute2.channel = 1;
						
						audio->msgAddRoute(srcRoute2, dstRoute2);
					}
					else
					{
						//Route channel 1
						Route srcRoute(track, 0);
						Route dstRoute(selectedOutput, true, -1, Route::JACK_ROUTE);
						dstRoute.channel = 0;

						audio->msgAddRoute(srcRoute, dstRoute);

						//Route channel 2
						Route srcRoute2(track, 1);
						Route dstRoute2(selectedOutput, true, -1, Route::JACK_ROUTE);
						dstRoute2.channel = 1;

						audio->msgAddRoute(srcRoute2, dstRoute2);
					}

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
				rv = true;
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			Track* track = song->addTrackByName(vtrack->name, Track::AUDIO_INPUT, -1, false);
			if(track)
			{
				track->setMute(false);
				if(vtrack->useInput)
				{
					QString selectedInput = vtrack->inputConfig.second;

					QString jackCapture("system:capture");
					if(selectedInput.startsWith(jackCapture))
					{
						Route srcRoute(QString(jackCapture).append("_1"), false, -1, Route::JACK_ROUTE);
						Route dstRoute(track, 0);
						srcRoute.channel = 0;
						audio->msgAddRoute(srcRoute, dstRoute);

						Route srcRoute2(QString(jackCapture).append("_2"), false, -1, Route::JACK_ROUTE);
						Route dstRoute2(track, 1);
						srcRoute2.channel = 1;
						audio->msgAddRoute(srcRoute2, dstRoute2);
					}
					else
					{
						Route srcRoute(selectedInput, false, -1, Route::JACK_ROUTE);
						Route dstRoute(track, 0);
						srcRoute.channel = 0;
						audio->msgAddRoute(srcRoute, dstRoute);

						Route srcRoute2(selectedInput, false, -1, Route::JACK_ROUTE);
						Route dstRoute2(track, 1);
						srcRoute2.channel = 1;
						audio->msgAddRoute(srcRoute2, dstRoute2);
					}

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				if(vtrack->useOutput)
				{
					QString selectedOutput = vtrack->outputConfig.second;

					Route srcRoute(track, 0, track->channels());
					Route dstRoute(selectedOutput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
				rv = true;
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			Track* track = song->addTrackByName(vtrack->name, Track::AUDIO_BUSS, -1, false);
			if(track)
			{
				if(vtrack->useInput)
				{
					QString selectedInput = vtrack->inputConfig.second;
					Route srcRoute(selectedInput, true, -1);
					Route dstRoute(track, 0, track->channels());

					audio->msgAddRoute(srcRoute, dstRoute);

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				if(vtrack->useOutput)
				{
					QString selectedOutput = vtrack->outputConfig.second;
					Route srcRoute(track, 0, track->channels());
					Route dstRoute(selectedOutput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
				rv = true;
			}
		}
		break;
		case Track::AUDIO_AUX:
		{
			Track* track = song->addTrackByName(vtrack->name, Track::AUDIO_AUX, -1, false);
			if(track)
			{
				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
				rv = true;
			}
		}
		break;
		case Track::AUDIO_SOFTSYNTH:
        {
			//Just add the track type and rename it
        }
        break;
	}
	return rv;
}/*}}}*/

bool TrackManager::loadInstrument(VirtualTrack *vtrack)
{
	bool rv = false;
	Track::TrackType type = (Track::TrackType)vtrack->type;
	if(type == Track::MIDI)
	{
		QString instrumentName = vtrack->instrumentName;
		QString trackName = vtrack->name;
		for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
		{
			if ((*i)->iname() == instrumentName && (*i)->isOOMInstrument())
			{
				if(!lsClient)
				{
					lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
					lsClientStarted = lsClient->startClient();
					if(config.lsClientResetOnStart && lsClientStarted)
					{
						lsClient->resetSampler();
					}
				}
				else if(!lsClientStarted)
				{
					lsClientStarted = lsClient->startClient();
					if(config.lsClientResetOnStart && lsClientStarted)
					{
						lsClient->resetSampler();
					}
				}
				if(lsClientStarted)
				{
					qDebug("Loading Instrument to LinuxSampler");
					if(lsClient->loadInstrument(*i))
					{
						rv = true;
						qDebug("Instrument Map Loaded");
						if(vtrack->autoCreateInstrument)
						{
							int map = lsClient->findMidiMap((*i)->iname().toUtf8().constData());
							Patch* p = (*i)->getDefaultPatch();
							if(p && map >= 0)
							{
								if(lsClient->createInstrumentChannel(vtrack->name.toUtf8().constData(), p->engine.toUtf8().constData(), p->filename.toUtf8().constData(), p->index, map))
								{
									qDebug("Created Channel for track");
									/*QString prefix("LinuxSampler:");
									QString postfix("-audio");
									QString audio(QString(prefix).append(trackName).append(postfix));
									QString midi(QString(prefix).append(trackName));*/
								}
								else
								{
									rv = false;
								}
							}
						}
					}
				}
				break;
			}
		}
	}
	return rv;
}

void TrackManager::createMonitorInputTracks(VirtualTrack* vtrack)/*{{{*/
{
	bool newBuss = vtrack->bussConfig.first;
	QString name(vtrack->name);

	QString inputName = QString("i").append(name);
	QString bussName = QString("B").append(name);
	//QString audioName = QString("A-").append(name);
	Track* input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false);
	Track* buss = 0;
	if(vtrack->useBuss)
	{
		if(newBuss)
			buss = song->addTrackByName(bussName, Track::AUDIO_BUSS, -1, true);
		else
			buss = song->findTrack(vtrack->bussConfig.second);
	}
	//Track* audiot = song->addTrackByName(audioName, Track::WAVE, -1, false);
	if(input/* && audiot*/)
	{
		input->setMute(false);
		QString selectedInput = vtrack->monitorConfig.second;
		
		//Route world to input
		QString jackCapture("system:capture");
		if(selectedInput.startsWith(jackCapture))
		{
			//Route channel 1
			Route srcRoute(QString(jackCapture).append("_1"), false, -1, Route::JACK_ROUTE);
			Route dstRoute(input, 0);
			srcRoute.channel = 0;
			audio->msgAddRoute(srcRoute, dstRoute);

			//Route channel 2
			Route srcRoute2(QString(jackCapture).append("_2"), false, -1, Route::JACK_ROUTE);
			Route dstRoute2(input, 1);
			srcRoute2.channel = 1;
			audio->msgAddRoute(srcRoute2, dstRoute2);
		}
		else
		{
			//Route channel 1
			Route srcRoute(selectedInput, false, -1, Route::JACK_ROUTE);
			Route dstRoute(input, 0);
			srcRoute.channel = 0;
			audio->msgAddRoute(srcRoute, dstRoute);

			//Route channel 2
			Route srcRoute2(selectedInput, false, -1, Route::JACK_ROUTE);
			Route dstRoute2(input, 1);
			srcRoute2.channel = 1;
			audio->msgAddRoute(srcRoute2, dstRoute2);
		}

		//Route input to buss
		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
		
		if(vtrack->useBuss && buss)
		{
			Route srcRoute(input, 0, input->channels());
			Route dstRoute(buss->name(), true, -1);

			audio->msgAddRoute(srcRoute, dstRoute);
		}

		//Route input to audio
		/*Route srcRoute2(input, 0, input->channels());
		Route dstRoute2(audiot->name(), true, -1);
		
		audio->msgAddRoute(srcRoute2, dstRoute2);*/

		//Route audio to master
		Track* master = song->findTrack("Master");
		if(master)
		{
			//Route buss track to master
			if(vtrack->useBuss && buss)
			{
				Route srcRoute3(buss, 0, buss->channels());
				Route dstRoute3(master->name(), true, -1);
		
				audio->msgAddRoute(srcRoute3, dstRoute3);
			}
			else
			{//Route input directly to Master
				Route srcRoute3(input, 0, input->channels());
				Route dstRoute3(master->name(), true, -1);
		
				audio->msgAddRoute(srcRoute3, dstRoute3);
			}

			//Route audio track to master
			/*Route srcRoute4(audiot, 0, audiot->channels());
			Route dstRoute4(master->name(), true, -1);
		
			audio->msgAddRoute(srcRoute4, dstRoute4);*/
		}

		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
}/*}}}*/


/*void TrackManager::importOutputs()
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbInput->addItem(*i, 1);
		}
	}
}

void TrackManager::importInputs()
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->inputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbOutput->addItem(*i, 1);
		}
	}
}

void TrackManager::populateMonitorList()
{
	while(cmbMonitor->count())
		cmbMonitor->removeItem(cmbMonitor->count()-1);
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbMonitor->addItem(*i, 1);
		}
	}
}

void TrackManager::populateNewInputList()
{
	//while(cmbInput->count())
	//	cmbInput->removeItem(cmbInput->count()-1);
	//m_createMidiInputDevice = true;
	for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
	{
		if ((*i)->deviceType() == MidiDevice::ALSA_MIDI)
		{
			cmbInput->addItem((*i)->name(), MidiDevice::ALSA_MIDI);
		}
	}
	if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
		return;
	std::list<QString> sl = audioDevice->outputPorts(true, m_showJackAliases);
	for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
	{
		cmbInput->addItem(*ip, MidiDevice::JACK_MIDI);
	}
}

void TrackManager::populateNewOutputList()
{
	//while(cmbOutput->count())
	//	cmbOutput->removeItem(cmbOutput->count()-1);
	//m_createMidiOutputDevice = true;
	if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
		return;
	std::list<QString> sl = audioDevice->inputPorts(true, m_showJackAliases);
	for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
	{
		cmbOutput->addItem(*ip, MidiDevice::JACK_MIDI);
	}
}*/

int TrackManager::getFreeMidiPort()/*{{{*/
{
	int rv = -1;
	for (int i = 0; i < MIDI_PORTS; ++i)
	{
		MidiPort* mp = &midiPorts[i];
		MidiDevice* md = mp->device();
		
		//Use the first unconfigured port
		if (!md)
		{
			rv = i;
			break;
		}
	}
	return rv;
}/*}}}*/


