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
#include "audio.h"
#include "midiseq.h"
#include "driver/jackaudio.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "network/lsclient.h"
#include "icons.h"
#include "midimonitor.h"
#include "plugin.h"
#include "xml.h"
#include "utils.h"

VirtualTrack::VirtualTrack()
{/*{{{*/
	id = create_id();
	type = -1;
	useOutput = false;
	useInput = false;
	useGlobalInputs = false;
	useBuss = false;
	useMonitor = false;
	inputChannel = -1;
	outputChannel = -1;
	instrumentType = -1;
	autoCreateInstrument = false;
	createMidiInputDevice = false;
	createMidiOutputDevice = false;
}/*}}}*/

void VirtualTrack::write(int level, Xml& xml) const/*{{{*/
{
	QStringList tmplist;
	std::string tag = "virtualTrack";
	xml.nput(level, "<%s id=\"%lld\" ", tag.c_str(), id);
	level++;
	xml.nput("type=\"%d\" useGlobalInputs=\"%d\" name=\"%s\" useInput=\"%d\" useOutput=\"%d\" ", type, useGlobalInputs, name.toUtf8().constData(), useInput, useOutput);
	if(!instrumentName.isEmpty() && type == Track::MIDI)
	{
		xml.nput("autoCreateInstrument=\"%d\" createMidiInputDevice=\"%d\" createMidiOutputDevice=\"%d\" ", 
				autoCreateInstrument, createMidiInputDevice, createMidiOutputDevice);
		xml.nput("instrumentName=\"%s\" instrumentType=\"%d\" useMonitor=\"%d\" useBuss=\"%d\" inputChannel=\"%d\" outputChannel=\"%d\" ", 
				instrumentName.toUtf8().constData(), instrumentType, useMonitor, useBuss, inputChannel, outputChannel);
		if(useMonitor)
		{
			tmplist.clear();
			tmplist << QString::number(monitorConfig.first) << monitorConfig.second;
			xml.nput("monitorConfig=\"%s\" ", tmplist.join("@-:-@").toUtf8().constData());
		}
		if(useBuss)
		{ 
			tmplist.clear();
			tmplist << QString::number(bussConfig.first) << bussConfig.second;
			xml.nput("bussConfig=\"%s\" ", tmplist.join("@-:-@").toUtf8().constData());
		}
	}
	if(useInput && !useGlobalInputs)
	{ 
		tmplist.clear();
		tmplist << QString::number(inputConfig.first) << inputConfig.second;
		xml.nput("inputConfig=\"%s\" ", tmplist.join("@-:-@").toUtf8().constData());
	}
	if(useOutput)
	{ 
		tmplist.clear();
		tmplist << QString::number(outputConfig.first) << outputConfig.second;
		xml.nput("outputConfig=\"%s\" ", tmplist.join("@-:-@").toUtf8().constData());
	}
    xml.put("/>");
	level--;
}/*}}}*/

void VirtualTrack::read(Xml &xml)/*{{{*/
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				break;
			case Xml::Attribut:
				if(tag == "id")
				{
					id = xml.s2().toLongLong();
				}
				else if(tag == "name")
				{
					name = xml.s2();
				}
				else if(tag == "type")
				{
					type = xml.s2().toInt();
				}
				else if(tag == "instrumentName")
				{
					instrumentName = xml.s2();
				}
				else if(tag == "instrumentType")
				{
					instrumentType = xml.s2().toInt();
				}
				else if(tag == "autoCreateInstrument")
				{
					autoCreateInstrument = xml.s2().toInt();
				}
				else if(tag == "createMidiInputDevice")
				{
					createMidiInputDevice = xml.s2().toInt();
				}
				else if(tag == "createMidiOutputDevice")
				{
					createMidiOutputDevice = xml.s2().toInt();
				}
				else if (tag == "useMonitor")
				{
					useMonitor = xml.s2().toInt();
				}
				else if(tag == "useGlobalInputs")
				{
					useGlobalInputs = xml.s2().toInt();
				}
				else if(tag == "useInput")
				{
					useInput = xml.s2().toInt();
				}
				else if(tag == "useOutput")
				{
					useOutput = xml.s2().toInt();
				}
				else if(tag == "useBuss")
				{
					useBuss = xml.s2().toInt();
				}
				else if(tag == "inputChannel")
				{
					inputChannel = xml.s2().toInt();
				}
				else if(tag == "outputChannel")
				{
					outputChannel = xml.s2().toInt();
				}
				else if(tag == "inputConfig")
				{
					QStringList list = xml.s2().split("@-:-@");
					if(list.size() && list.size() == 2)
						inputConfig = qMakePair(list[0].toInt(), list[1]);
				}
				else if(tag == "outputConfig")
				{
					QStringList list = xml.s2().split("@-:-@");
					if(list.size() && list.size() == 2)
						outputConfig = qMakePair(list[0].toInt(), list[1]);
				}
				else if(tag == "monitorConfig")
				{
					QStringList list = xml.s2().split("@-:-@");
					if(list.size() && list.size() == 2)
						monitorConfig = qMakePair(list[0].toInt(), list[1]);
				}
				else if(tag == "monitorConfig2")
				{
					QStringList list = xml.s2().split("@-:-@");
					if(list.size() && list.size() == 2)
						monitorConfig2 = qMakePair(list[0].toInt(), list[1]);
				}
				else if(tag == "bussConfig")
				{
					QStringList list = xml.s2().split("@-:-@");
					if(list.size() && list.size() == 2)
						bussConfig = qMakePair(list[0].toInt(), list[1]);
				}
				break;
			case Xml::TagEnd:
				if(tag == "virtualTrack")
					return;
			default:
				break;
		}
	}
}/*}}}*/

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
qint64 TrackManager::addTrack(VirtualTrack* vtrack, int index)/*{{{*/
{
	m_insertPosition = index;
	qint64 rv = 0;
	if(!vtrack || vtrack->name.isEmpty())
		return rv;
	
	Track::TrackType type = (Track::TrackType)vtrack->type;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			//Load up the instrument first
			if(vtrack->autoCreateInstrument)
				loadInstrument(vtrack);
			Track* track =  song->addTrackByName(vtrack->name, Track::MIDI, m_insertPosition, false);
			if(track)
			{
				MidiTrack* mtrack = (MidiTrack*)track;
				//Process Input connections
				if(vtrack->useInput)
				{
					if(vtrack->useGlobalInputs)
					{
						//Get the current port(s) linked with the global inputs
						for(int i = 0; i < gInputListPorts.size(); ++i)
						{
							MidiDevice* indev = 0;
							m_midiInPort = gInputListPorts.at(i);
							qDebug("Using global MIDI Input port: %i", m_midiInPort);

							MidiPort* inport = &midiPorts[m_midiInPort];
							if(inport)
								indev = inport->device();
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
					}
					else
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
							if(vtrack->instrumentType == SYNTH_INSTRUMENT)
							{
                                mtrack->setWantsAutomation(true);
							}
							else
							{
								for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
								{
									if ((*i)->iname() == instrumentName)
									{
										outport->setInstrument(*i);
										break;
									}
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
				emit trackAdded(track->id());
				rv = track->id();
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
				emit trackAdded(track->id());
				rv = track->id();
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
					{//FIXME: Change this to support more than two system playback devices
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
				emit trackAdded(track->id());
				rv = track->id();
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
				emit trackAdded(track->id());
				rv = track->id();
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
				emit trackAdded(track->id());
				rv = track->id();
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
				emit trackAdded(track->id());
				rv = track->id();
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

bool TrackManager::removeTrack(VirtualTrack* vtrack)/*{{{*/
{
	bool rv = false;
	Q_UNUSED(vtrack);
	/*if(!vtrack || vtrack->name.isEmpty())
		return rv;
	
	Track::TrackType type = (Track::TrackType)vtrack->type;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			//Load up the instrument first
			//loadInstrument(vtrack);
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
	}*/
	return rv;
}/*}}}*/

bool TrackManager::loadInstrument(VirtualTrack *vtrack)/*{{{*/
{
	bool rv = false;
	Track::TrackType type = (Track::TrackType)vtrack->type;
	if(type == Track::MIDI)
	{
		QString instrumentName = vtrack->instrumentName;
		QString trackName = vtrack->name;
		switch(vtrack->instrumentType)
		{
			case LS_INSTRUMENT:
			{
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
			break;
			case SYNTH_INSTRUMENT://SYNTH instrument, falkTx do your synth on the fly creation hooks here
			{
                int portIdx = getFreeMidiPort();
                
                if (portIdx >= 0)
                {
                    for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
                    {
                        if ((*i)->isSynthPlugin() && (*i)->name() == instrumentName)
                        {
							QString devName(QString("O-").append(vtrack->name));
                            SynthPluginDevice* oldSynth = (SynthPluginDevice*)(*i);
                            SynthPluginDevice* synth = oldSynth->clone(devName);
                            synth->open();

                            midiSeq->msgSetMidiDevice(&midiPorts[portIdx], synth);

                            QString selectedInput  = synth->getAudioOutputPortName(0);
                            QString selectedInput2 = synth->getAudioOutputPortName(1);

							m_midiOutPort = portIdx;
							qDebug("Port Names: left: %s, right: %s", selectedInput.toUtf8().constData(), selectedInput2.toUtf8().constData());
							vtrack->outputConfig = qMakePair(portIdx, devName);
                            vtrack->monitorConfig  = qMakePair(0, selectedInput);
                            vtrack->monitorConfig2 = qMakePair(0, selectedInput2);

                            break;
                        }
                    }
                }
			}
			break;
			case GM_INSTRUMENT:  //Regular idf no linuxsampler
			{
			}
			break;
		}
	}
	return rv;
}/*}}}*/

bool TrackManager::unloadInstrument(VirtualTrack *vtrack)/*{{{*/
{
	bool rv = false;
	Q_UNUSED(vtrack);
	/*Track::TrackType type = (Track::TrackType)vtrack->type;
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
	}*/
	return rv;
}/*}}}*/

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
        QString selectedInput2;

        if (vtrack->monitorConfig2.second.isEmpty())
            selectedInput2 = selectedInput;
        else
            selectedInput2 = vtrack->monitorConfig2.second;
		qDebug("Port Names: left: %s, right: %s", selectedInput.toUtf8().constData(), selectedInput2.toUtf8().constData());

		//Route world to input
		QString jackCapture("system:capture");
		if(selectedInput.startsWith(jackCapture))
		{//FIXME: Change this to support  more than 2 capture devices
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
			Route srcRoute2(selectedInput2, false, -1, Route::JACK_ROUTE);
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

void TrackManager::removeMonitorInputTracks(VirtualTrack* vtrack)/*{{{*/
{
	Q_UNUSED(vtrack);
/*
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
	if(input)
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
		}

		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
	*/
}/*}}}*/

void TrackManager::write(int level, Xml& xml) const/*{{{*/
{
	std::string tag = "trackManager";
	xml.put(level, "<%s tracks=\"%d\">",tag.c_str() , m_virtualTracks.size());
	level++;
	foreach(VirtualTrack* vt, m_virtualTracks)
	{
		vt->write(level, xml);
	}
    xml.put(--level, "</%s>", tag.c_str());
}/*}}}*/

void TrackManager::read(Xml& xml)/*{{{*/
{
	for (;;)
	{
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if(tag == "virtualTrack")
				{
					VirtualTrack* vt = new VirtualTrack;
					vt->read(xml);
					m_virtualTracks.insert(vt->id, vt);
				}
				break;
			case Xml::Attribut:
				if(tag == "tracks")
				{//We'll do something with this later
					xml.s2().toInt();
				}
				break;
			case Xml::TagEnd:
				if(tag == "trackManager")
					return;
			default:
				break;
		}
	}
}/*}}}*/

