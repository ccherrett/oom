//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "TrackManager.h"
#include "globaldefs.h"
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

#include "QMessageBox"

VirtualTrack::VirtualTrack()
{/*{{{*/
	id = create_id();
	type = -1;
	useOutput = false;
	useInput = false;
	useGlobalInputs = false;
	useBuss = false;
	useMonitor = false;
	inputChannel = 0;
	outputChannel = 0;
	instrumentType = -1;
	instrumentPan = 0.0;
	instrumentVerb = 0.0;
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
		xml.nput("instrumentName=\"%s\" instrumentType=\"%d\" instrumentPan=\"%f\" instrumentVerb=\"%f\" ",
				instrumentName.toUtf8().constData(), instrumentType, instrumentPan, instrumentVerb);
		
		xml.nput("useMonitor=\"%d\" useBuss=\"%d\" inputChannel=\"%d\" outputChannel=\"%d\" ", 
				useMonitor, useBuss, inputChannel, outputChannel);
		if(useMonitor && instrumentType != TrackManager::SYNTH_INSTRUMENT)
		{
			tmplist.clear();
			tmplist << QString::number(monitorConfig.first) << monitorConfig.second;
			xml.nput("monitorConfig=\"%s\" ", tmplist.join("@-:-@").toUtf8().constData());
			tmplist.clear();
			tmplist << QString::number(monitorConfig2.first) << monitorConfig2.second;
			xml.nput("monitorConfig2=\"%s\" ", tmplist.join("@-:-@").toUtf8().constData());
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
				else if(tag == "instrumentPan")
				{
					instrumentPan = xml.s2().toDouble();
				}
				else if(tag == "instrumentVerb")
				{
					instrumentVerb = xml.s2().toDouble();
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
	m_track = 0;

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
			song->startUndo();
			m_track =  song->addTrackByName(vtrack->name, Track::MIDI, m_insertPosition, false, false);
			if(m_track)
			{
				m_track->setMasterFlag(true);
				//if(vtrack->instrumentType == SYNTH_INSTRUMENT)
				//m_track->setHeight(MIN_TRACKHEIGHT);
				if(vtrack->autoCreateInstrument)
				{
					if(!loadInstrument(vtrack))
					{
						qDebug("TrackManager::addTrack: Failed to load instrument");
						//QMessageBox::critical(this, tr("Instrument Load Failed"), tr("An error has occurred.\nFailed to load instrument"));
						song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
						song->startUndo();
						song->removeTrack(m_track);
						song->endUndo(SC_TRACK_REMOVED);
						m_track = 0;
						return 0;
					}
				}

				song->undoOp(UndoOp::AddTrack, m_insertPosition, m_track);
				MidiTrack* mtrack = (MidiTrack*)m_track;
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
							if(debugMsg)
								qDebug("TrackManager::addTrack: Using global MIDI Input port: %i", m_midiInPort);

							MidiPort* inport = &midiPorts[m_midiInPort];
							if(inport)
								indev = inport->device();
							if(inport && indev)
							{
								if(debugMsg)
									qDebug("TrackManager::addTrack: MIDI Input port and MIDI devices found, Adding final input routing");
								int chanbit = vtrack->inputChannel;
								Route srcRoute(m_midiInPort, chanbit);
								Route dstRoute(m_track, chanbit);

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
						QString inputDevName(QString("I-").append(m_track->name()));
						if(vtrack->createMidiInputDevice)
						{
							m_midiInPort = getFreeMidiPort();
							if(debugMsg)
								qDebug("TrackManager::addTrack: createMidiInputDevice is set: %i", m_midiInPort);
							inport = &midiPorts[m_midiInPort];
							int devtype = vtrack->inputConfig.first;
							if(devtype == MidiDevice::ALSA_MIDI)
							{
								indev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
								if(indev)
								{
									if(debugMsg)
										qDebug("TrackManager::addTrack: Found MIDI input device: ALSA_MIDI");
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
									if(debugMsg)
										qDebug("TrackManager::addTrack: Created MIDI input device: JACK_MIDI");
									int openFlags = 0;
									openFlags ^= 0x2;
									indev->setOpenFlags(openFlags);
									midiSeq->msgSetMidiDevice(inport, indev);
								}
							}
							if(indev && indev->deviceType() == MidiDevice::JACK_MIDI)
							{
								if(debugMsg)
									qDebug("TrackManager::addTrack: MIDI input device configured, Adding input routes to MIDI port");
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
							if(debugMsg)
								qDebug("TrackManager::addTrack: Using existing MIDI port: %i", m_midiInPort);
							inport = &midiPorts[m_midiInPort];
							if(inport)
								indev = inport->device();
						}
						if(inport && indev)
						{
							if(debugMsg)
								qDebug("TrackManager::addTrack: MIDI Input port and MIDI devices found, Adding final input routing");
							int chanbit = vtrack->inputChannel;
							Route srcRoute(m_midiInPort, chanbit);
							Route dstRoute(m_track, chanbit);

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
					QString outputDevName(QString("O-").append(m_track->name()));
					if(vtrack->createMidiOutputDevice)
					{
						m_midiOutPort = getFreeMidiPort();
						if(debugMsg)
							qDebug("TrackManager::addTrack: m_createMidiOutputDevice is set: %i", m_midiOutPort);
						outport = &midiPorts[m_midiOutPort];
						int devtype = vtrack->outputConfig.first;
						if(devtype == MidiDevice::ALSA_MIDI)
						{
							outdev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
							if(outdev)
							{
								if(debugMsg)
									qDebug("TrackManager::addTrack: Found MIDI output device: ALSA_MIDI");
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
								if(debugMsg)
									qDebug("TrackManager::addTrack: Created MIDI output device: JACK_MIDI");
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
							if(debugMsg)
								qDebug("TrackManager::addTrack: MIDI output device configured, Adding output routes to MIDI port");
							Route srcRoute(outdev, -1);
							Route dstRoute(devname, true, -1, Route::JACK_ROUTE);

							if(debugMsg)
								qDebug("TrackManager::addTrack: Device name from combo: %s, from dev: %s", devname.toUtf8().constData(), outdev->name().toUtf8().constData());

							audio->msgAddRoute(srcRoute, dstRoute);

							audio->msgUpdateSoloStates();
							song->update(SC_ROUTE);
						}
					}
					else
					{
						if(vtrack->instrumentType == SYNTH_INSTRUMENT)
						{//Get the config from our internal list
							m_midiOutPort = m_synthConfigs.at(0).first;
						}
						else
						{
							m_midiOutPort = vtrack->outputConfig.first;
						}
						if(debugMsg)
							qDebug("TrackManager::addTrack: Using existing MIDI output port: %i", m_midiOutPort);
						outport = &midiPorts[m_midiOutPort];
						if(outport)
							outdev = outport->device();
					}
					if(outport && outdev)
					{
						if(debugMsg)
							qDebug("TrackManager::addTrack: MIDI output port and MIDI devices found, Adding final output routing");
						audio->msgIdle(true);
						mtrack->setOutPortAndUpdate(m_midiOutPort);
						int outChan = vtrack->outputChannel;
						mtrack->setOutChanAndUpdate(outChan);
						audio->msgIdle(false);
						if(vtrack->createMidiOutputDevice || vtrack->instrumentType == SYNTH_INSTRUMENT)
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

				song->deselectTracks();
				m_track->setSelected(true);
				emit trackAdded(m_track->id());
				rv = m_track->id();
			}
			song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
			song->updateTrackViews();
		}
		break;
		case Track::WAVE:
		{
			song->startUndo();
			m_track =  song->addTrackByName(vtrack->name, Track::WAVE, m_insertPosition, false, !vtrack->useOutput);
			if(m_track)
			{
				m_track->setMasterFlag(true);
				song->undoOp(UndoOp::AddTrack, m_insertPosition, m_track);
				if(vtrack->useInput)
				{
					QString inputName = QString("i").append(m_track->name());
					QString selectedInput = vtrack->inputConfig.second;
					bool addNewRoute = vtrack->inputConfig.first;
					Track* input = m_track->inputTrack();
					/*if(addNewRoute)
					{
						input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false, false);
						if(input)
						{
							input->setMasterFlag(false);
							input->setChainMaster(m_track->id());
							m_track->addManagedTrack(input->id());
						}
					}
					else
						input = song->findTrack(selectedInput);*/
					if(input)
					{
						song->undoOp(UndoOp::AddTrack, -1, input);
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
						//Already done in song->addTrackByName
						/*Route srcRoute(input->name(), true, -1);
						Route dstRoute(m_track, 0, m_track->channels());

						audio->msgAddRoute(srcRoute, dstRoute);

						audio->msgUpdateSoloStates();
						song->update(SC_ROUTE);*/
					}
				}
				if(vtrack->useOutput)
				{
					//Route to the Output or Buss
					QString selectedOutput = vtrack->outputConfig.second;
					Route srcRoute(m_track, 0, m_track->channels());
					Route dstRoute(selectedOutput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				song->deselectTracks();
				m_track->setSelected(true);
				emit trackAdded(m_track->id());
				rv = m_track->id();
				
			}
			song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
			song->updateTrackViews();
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			song->startUndo();
			m_track = song->addTrackByName(vtrack->name, Track::AUDIO_OUTPUT, -1, false, false);
			if(m_track)
			{
				m_track->setMasterFlag(true);
				song->undoOp(UndoOp::AddTrack, -1, m_track);
				if(vtrack->useInput)
				{
					QString selectedInput = vtrack->inputConfig.second;
					Route dstRoute(m_track, 0, m_track->channels());
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
						Route srcRoute(m_track, 0);
						Route dstRoute(QString(jackPlayback).append("_1"), true, -1, Route::JACK_ROUTE);
						dstRoute.channel = 0;

						audio->msgAddRoute(srcRoute, dstRoute);

						//Route channel 2
						Route srcRoute2(m_track, 1);
						Route dstRoute2(QString(jackPlayback).append("_2"), true, -1, Route::JACK_ROUTE);
						dstRoute2.channel = 1;
						
						audio->msgAddRoute(srcRoute2, dstRoute2);
					}
					else
					{
						//Route channel 1
						Route srcRoute(m_track, 0);
						Route dstRoute(selectedOutput, true, -1, Route::JACK_ROUTE);
						dstRoute.channel = 0;

						audio->msgAddRoute(srcRoute, dstRoute);

						//Route channel 2
						Route srcRoute2(m_track, 1);
						Route dstRoute2(selectedOutput, true, -1, Route::JACK_ROUTE);
						dstRoute2.channel = 1;

						audio->msgAddRoute(srcRoute2, dstRoute2);
					}

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				song->deselectTracks();
				m_track->setSelected(true);
				emit trackAdded(m_track->id());
				rv = m_track->id();
			}
			song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
			song->updateTrackViews();
		}
		break;
		case Track::AUDIO_INPUT:
		{
			song->startUndo();
			m_track = song->addTrackByName(vtrack->name, Track::AUDIO_INPUT, -1, false, false);
			if(m_track)
			{
				m_track->setMasterFlag(true);
				song->undoOp(UndoOp::AddTrack, -1, m_track);
				m_track->setMute(false);
				if(vtrack->useInput)
				{
					QString selectedInput = vtrack->inputConfig.second;

					QString jackCapture("system:capture");
					if(selectedInput.startsWith(jackCapture))
					{
						Route srcRoute(QString(jackCapture).append("_1"), false, -1, Route::JACK_ROUTE);
						Route dstRoute(m_track, 0);
						srcRoute.channel = 0;
						audio->msgAddRoute(srcRoute, dstRoute);

						Route srcRoute2(QString(jackCapture).append("_2"), false, -1, Route::JACK_ROUTE);
						Route dstRoute2(m_track, 1);
						srcRoute2.channel = 1;
						audio->msgAddRoute(srcRoute2, dstRoute2);
					}
					else
					{
						Route srcRoute(selectedInput, false, -1, Route::JACK_ROUTE);
						Route dstRoute(m_track, 0);
						srcRoute.channel = 0;
						audio->msgAddRoute(srcRoute, dstRoute);

						Route srcRoute2(selectedInput, false, -1, Route::JACK_ROUTE);
						Route dstRoute2(m_track, 1);
						srcRoute2.channel = 1;
						audio->msgAddRoute(srcRoute2, dstRoute2);
					}

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				if(vtrack->useOutput)
				{
					QString selectedOutput = vtrack->outputConfig.second;

					Route srcRoute(m_track, 0, m_track->channels());
					Route dstRoute(selectedOutput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				song->deselectTracks();
				m_track->setSelected(true);
				emit trackAdded(m_track->id());
				rv = m_track->id();
			}
			song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
			song->updateTrackViews();
		}
		break;
		case Track::AUDIO_BUSS:
		{
			song->startUndo();
			m_track = song->addTrackByName(vtrack->name, Track::AUDIO_BUSS, -1, false, false);
			if(m_track)
			{
				m_track->setMasterFlag(true);
				song->undoOp(UndoOp::AddTrack, -1, m_track);
				if(vtrack->useInput)
				{
					QString selectedInput = vtrack->inputConfig.second;
					Route srcRoute(selectedInput, true, -1);
					Route dstRoute(m_track, 0, m_track->channels());

					audio->msgAddRoute(srcRoute, dstRoute);

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				if(vtrack->useOutput)
				{
					QString selectedOutput = vtrack->outputConfig.second;
					Route srcRoute(m_track, 0, m_track->channels());
					Route dstRoute(selectedOutput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				song->deselectTracks();
				m_track->setSelected(true);
				emit trackAdded(m_track->id());
				rv = m_track->id();
			}
			song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
			song->updateTrackViews();
		}
		break;
		case Track::AUDIO_AUX:
		{
			song->startUndo();
			m_track = song->addTrackByName(vtrack->name, Track::AUDIO_AUX, -1, false, false);
			if(m_track)
			{
				m_track->setMasterFlag(true);
				song->undoOp(UndoOp::AddTrack, -1, m_track);
				if(vtrack->useOutput)
				{
					QString selectedOutput = vtrack->outputConfig.second;
					Route srcRoute((AudioTrack*)m_track, -1);
					Route dstRoute(selectedOutput, true, -1);

					//audio->msgAddRoute(Route((AudioTrack*) m_track, -1), Route(ao, -1));
					audio->msgAddRoute(srcRoute, dstRoute);
					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				song->deselectTracks();
				m_track->setSelected(true);
				emit trackAdded(m_track->id());
				rv = m_track->id();
			}
			song->endUndo(SC_TRACK_INSERTED | SC_TRACK_MODIFIED);
			song->updateTrackViews();
		}
		break;
		default:
        break;
	}
	m_track = 0;
	m_synthConfigs.clear();
	return rv;
}/*}}}*/

void TrackManager::setTrackInstrument(qint64 tid, const QString& instrument, int type)/*{{{*/
{
	Track *t = song->findTrackById(tid);

	if(!t || instrument.isEmpty())
		return;
	m_track = t;
	MidiTrack* track = (MidiTrack*) m_track;
	MidiPort *mp = oomMidiPorts.value(track->outPortId());
	if(mp)
	{
		MidiInstrument* oldins = mp->instrument();
		if(!oldins || oldins->iname() == instrument)
			return;//No change
		MidiDevice* md = mp->device();
		bool isSynth = (md && md->isSynthPlugin());
		switch(type)
		{
			case LS_INSTRUMENT:
			{
				if(!lsClient)
				{
					lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
					lsClientStarted = lsClient->startClient();
				}
				else if(!lsClientStarted)
				{
					lsClientStarted = lsClient->startClient();
					if(config.lsClientResetOnStart && lsClientStarted)
					{
						lsClient->resetSampler();
					}
				}
				if(!lsClientStarted)
				{
					return;//TODO: announce error condition
				}
				MidiInstrument* ins = 0;
				for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
				{
					if ((*i)->iname() == instrument)
					{
						ins = *i;
						break;
					}
				}
				if(!ins)
				{//Reset GUI widgets that made this call and return
					return;
				}
				if(isSynth || !track->samplerData())
				{//We need to deal with closing the synth devices first
					if(isSynth)
					{
						Track* input = 0;/*{{{*/
						QList<qint64> *chain = track->audioChain();
						if(chain && !chain->isEmpty())
						{
							for(int c = 0; c < chain->size(); c++)
							{
								input = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_INPUT);
								if(input)
									break;
							}
						}
						if(input)
						{//Remove route
							int channels = input->channels();
							for(int i = 0; i < channels; i++)
							{
								if(debugMsg)
									qDebug("TrackManager::setTrackInstrument: Removing route for channel: %d", i);
								AudioInput* itrack = (AudioInput*)input;
								if(itrack)
								{
									RouteList* rl = itrack->inRoutes();
									for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
									{
										if (irl->channel != i)
											continue;
										QString name = irl->name();
										QByteArray ba = name.toLatin1();
										const char* portName = ba.constData();
										if(name.contains(QString("O-").append(m_track->name()).append(":output")))
										{
											Route srcRoute(portName, false, i, Route::JACK_ROUTE);
											Route dstRoute(itrack, i);
											if(debugMsg)
											{
												qDebug("TrackManager::setTrackInstrument: srcRoute: %s, %d, %d\n", portName, true, Route::JACK_ROUTE);
												qDebug("TrackManager::setTrackInstrument: dstRoute: %s, %d, %d\n", itrack->name().toUtf8().constData(), true, i);
											}
											audio->msgRemoveRoute(srcRoute, dstRoute);
											break;
										}
									}		
								}
							}
						}/*}}}*/
						if(debugMsg)
							qDebug("TrackManager::setTrackInstrument: Found Synth Instrument, Removing midi Device: %s", md->name().toUtf8().constData());
						SynthPluginDevice* synth = (SynthPluginDevice*)md;
						if (synth->duplicated())
						{
							midiDevices.remove(md);
							synth->close();
						}
						if(debugMsg)
							qDebug("TrackManager::setTrackInstrument: Synth cleanup complete");
					}
					mp->setInstrument(ins);
					song->update();
					int map = lsClient->findMidiMap(ins->iname().toUtf8().constData());
					Patch* p = ins->getDefaultPatch();
					if(p && map >= 0)
					{
						SamplerData* data;
						if(lsClient->createInstrumentChannel(m_track->name().toUtf8().constData(), p->engine.toUtf8().constData(), p->filename.toUtf8().constData(), p->index, map, &data))
						{
							if(data)
							{
								((MidiTrack*)m_track)->setSamplerData(data);
							}
							QString prefix("LinuxSampler:");
							QString postfix("-audio");
							QString devname(QString(prefix).append(m_track->name()));
							QString audioName(QString(prefix).append(m_track->name()).append(postfix));
							QString midi(QString("O-").append(m_track->name()));

							MidiDevice *ndev = MidiJackDevice::createJackMidiDevice(midi, 3);
							if(ndev)
							{
								if(debugMsg)
									qDebug("TrackManager::setTrackInstrument: Created MIDI input device: JACK_MIDI");
								int openFlags = 0;
								openFlags ^= 0x1;
								ndev->setOpenFlags(openFlags);
								midiSeq->msgSetMidiDevice(mp, ndev);

								Route dstRoute(devname, false, -1, Route::JACK_ROUTE);
								Route srcRoute(ndev, -1);

								audio->msgAddRoute(srcRoute, dstRoute);

								audio->msgUpdateSoloStates();
								song->update(SC_ROUTE);
							}
							
							Track* input = 0;
							Track *buss = 0;
							QList<qint64> *chain = track->audioChain();
							if(chain && !chain->isEmpty())
							{
								for(int c = 0; c < chain->size(); c++)
								{
									if(!input)
										input = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_INPUT);
									if(!buss)
										buss = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_BUSS);
									if(input && buss)
										break;
								}
							}

							QString inputName = QString("i").append(m_track->name());
							QString bussName = QString("B").append(m_track->name());
							
							bool useBuss = (input && !buss);
							if(!input)
							{
								input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false, false);
								input->setMasterFlag(false);
								input->setChainMaster(m_track->id());
								m_track->addManagedTrack(input->id());
								m_track->setMasterFlag(true);
								input->setMute(false);
								useBuss = true;//Default to using a buss
							}
							if(!buss && useBuss)
							{//Create a new one
								buss = song->addTrackByName(bussName, Track::AUDIO_BUSS, -1, false, true);
								buss->setMasterFlag(false);
								buss->setChainMaster(m_track->id());
								m_track->addManagedTrack(buss->id());

								Route srcRoute(input, 0, input->channels());
								Route dstRoute(buss->name(), true, -1);

								audio->msgAddRoute(srcRoute, dstRoute);

								Track* master = song->findTrackByIdAndType(song->masterId(), Track::AUDIO_OUTPUT);
								if(master)
								{
									Route srcRoute3(buss, 0, buss->channels());
									Route dstRoute3(master->name(), true, -1);
								
									audio->msgAddRoute(srcRoute3, dstRoute3);
								}
							}
								
							if(buss)
								configureVerb(buss, config.minSlider, 0.0);
							if(input)
							{//Do we have to remove that old route to the synth port, evaluate later
								configureVerb(input, ins->defaultVerb(), ins->defaultPan());
								//Route channel 1
								Route srcRoute(audioName, false, -1, Route::JACK_ROUTE);
								Route dstRoute(input, 0);
								srcRoute.channel = 0;
								audio->msgAddRoute(srcRoute, dstRoute);

								//Route channel 2
								Route srcRoute2(audioName, false, -1, Route::JACK_ROUTE);
								Route dstRoute2(input, 1);
								srcRoute2.channel = 1;
								audio->msgAddRoute(srcRoute2, dstRoute2);
							}
						}
						else
						{
							//TODO Show Error dialog
							return;
						}
					}
				}
				else
				{
					//Now set the instrument and let it load itself into LS
					mp->setInstrument(ins);
					song->update();
					int map = lsClient->findMidiMap(ins->iname().toUtf8().constData());
					Patch* p = ins->getDefaultPatch();
					if(p && map >= 0)
					{
						if(!lsClient->updateInstrumentChannel(track->samplerData(), p->engine.toUtf8().constData(), p->filename.toUtf8().constData(), p->index, map))
						{
							if(debugMsg)
								qDebug("TrackManager::setTrackInstrument: Failed to update Sampler Channel");
						}
					}
				}
				mp->setInstrument(ins);
				track->setWantsAutomation(false);
			}
			break;
			case SYNTH_INSTRUMENT:
			{
				MidiDevice *ndev = midiDevices.find(instrument, MidiDevice::SYNTH_MIDI);
				if(!ndev)
				{
					return;
				}
				if(isSynth)
				{
					Track* input = 0;/*{{{*/
					QList<qint64> *chain = track->audioChain();
					if(chain && !chain->isEmpty())
					{
						for(int c = 0; c < chain->size(); c++)
						{
							input = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_INPUT);
							if(input)
								break;
						}
					}
					if(input)
					{//Remove route
						int channels = input->channels();
						for(int i = 0; i < channels; i++)
						{
							if(debugMsg)
								qDebug("TrackManager::setTrackInstrument: Removing route for channel: %d", i);
							AudioInput* itrack = (AudioInput*)input;
							if(itrack)
							{
								RouteList* rl = itrack->inRoutes();
								for (iRoute irl = rl->begin(); irl != rl->end(); ++irl)
								{
									if (irl->channel != i)
										continue;
									QString name = irl->name();
									QByteArray ba = name.toLatin1();
									const char* portName = ba.constData();
									if(name.contains(QString("O-").append(m_track->name()).append(":output")))
									{
										Route srcRoute(portName, false, i, Route::JACK_ROUTE);
										Route dstRoute(itrack, i);
										if(debugMsg)
										{
											qDebug("TrackManager::setTrackInstrument: srcRoute: %s, %d, %d\n", portName, true, Route::JACK_ROUTE);
											qDebug("TrackManager::setTrackInstrument: dstRoute: %s, %d, %d\n", itrack->name().toUtf8().constData(), true, i);
										}
										audio->msgRemoveRoute(srcRoute, dstRoute);
										break;
									}
								}		
							}
						}
					}/*}}}*/
					SynthPluginDevice* synth = (SynthPluginDevice*)md;
					if (synth->duplicated())
					{
						midiDevices.remove(md);
						synth->close();
					}
				}
				else
				{//Maybe remove the channel in LS if it was an LS instrument
					if(oldins->isOOMInstrument() && track->samplerData())
					{//Delete sampler channel and null the track sampler data
						if(debugMsg)
							qDebug("TrackManager::setTrackInstrument Found old LS Instrument, Removing...");
						if(!lsClient)
						{
							lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
							lsClientStarted = lsClient->startClient();
						}
						else if(!lsClientStarted)
						{
							lsClientStarted = lsClient->startClient();
							if(config.lsClientResetOnStart && lsClientStarted)
							{
								lsClient->resetSampler();
							}
						}
						Route srcRoute(md, -1);
						RouteList *rl = md->outRoutes();
						QString outRoute;
						std::list<QString> sl = audioDevice->inputPorts(true, false);
						for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
						{
							bool found = false;
							Route rt(*ip, true, -1, Route::JACK_ROUTE);
							for (iRoute ir = rl->begin(); ir != rl->end(); ++ir)
							{
								if (*ir == rt)
								{
									outRoute = (*ir).name();
									found = true;
									break;
								}
							}
							if(found)
								break;
						}
						Route dstRoute(outRoute, true, -1, Route::JACK_ROUTE);
						audio->msgRemoveRoute(srcRoute, dstRoute);
						if (audioDevice)
						{
							if (md->outClientPort())
								audioDevice->unregisterPort(md->outClientPort());
						}
						if(debugMsg)
							qDebug("TrackManager::setTrackInstrument: Removing midi Device: %s", md->name().toUtf8().constData());
						midiDevices.remove(md);
						//Remove Audio Routes
						Track* input = 0;/*{{{*/
						QList<qint64> *chain = track->audioChain();
						if(chain && !chain->isEmpty())
						{
							for(int c = 0; c < chain->size(); c++)
							{
								input = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_INPUT);
								if(input)
									break;
							}
						}
						if(input)
						{//Remove route
							int channels = input->channels();
							for(int i = 0; i < channels; i++)
							{
								if(debugMsg)
									qDebug("TrackManager::setTrackInstrument: Removing route for channel: %d", i);
								QString src(QString("LinuxSampler:").append(m_track->name()).append("-audio"));
                				Route srcRoute(src, true, Route::JACK_ROUTE);
                				Route dstRoute(input->name(), true, i);
								if(debugMsg)
								{
									qDebug("TrackManager::setTrackInstrument: srcRoute: %s, %d, %d\n", src.toUtf8().constData(), true, Route::JACK_ROUTE);
									qDebug("TrackManager::setTrackInstrument: dstRoute: %s, %d, %d\n", input->name().toUtf8().constData(), true, i);
								}
								audio->msgRemoveRoute(srcRoute, dstRoute);
								/*if (audioDevice)
								{
									if (((AudioInput*)input)->jackPort(i))
										audioDevice->unregisterPort(((AudioInput*)input)->jackPort(i));
								}*/
							}
						}/*}}}*/
						//Remove the LinuxSampler channel
						if(lsClientStarted)
						{
							lsClient->removeInstrumentChannel(track->samplerData());
						}
					}
					track->setWantsAutomation(true);
				}
				if(ndev)
				{
					QString devName(QString("O-").append(m_track->name()));
                    SynthPluginDevice* oldSynth = (SynthPluginDevice*)ndev;
                    SynthPluginDevice* synth = oldSynth->clone(devName);
                    synth->open();

                    midiSeq->msgSetMidiDevice(mp, synth);

                    QString selectedInput  = synth->getAudioOutputPortName(0);
                    QString selectedInput2 = synth->getAudioOutputPortName(1);

					if(debugMsg)
						qDebug("TrackManager::setTrackInstrument: Port Names: left: %s, right: %s", selectedInput.toUtf8().constData(), selectedInput2.toUtf8().constData());
					Track* input = 0;
					Track* buss = 0;
					QList<qint64> *chain = track->audioChain();
					if(chain && !chain->isEmpty())
					{
						for(int c = 0; c < chain->size(); c++)
						{
							if(!input)
								input = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_INPUT);
							if(!buss)
								buss = song->findTrackByIdAndType(chain->at(c), Track::AUDIO_BUSS);
							if(input && buss)
								break;
						}
					}
					QString inputName = QString("i").append(m_track->name());
					QString bussName = QString("B").append(m_track->name());
					
					bool useBuss = (input && !buss);
					if(!input)
					{
						input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false, false);
						input->setMasterFlag(false);
						input->setChainMaster(m_track->id());
						m_track->addManagedTrack(input->id());
						input->setMute(false);
						m_track->setMasterFlag(true);
						useBuss = true;
					}
					if(!buss && useBuss)
					{//Create a new one
						buss = song->addTrackByName(bussName, Track::AUDIO_BUSS, -1, false, true);
						buss->setMasterFlag(false);
						buss->setChainMaster(m_track->id());
						m_track->addManagedTrack(buss->id());

						Route srcRoute(input, 0, input->channels());
						Route dstRoute(buss->name(), true, -1);

						audio->msgAddRoute(srcRoute, dstRoute);

						Track* master = song->findTrackByIdAndType(song->masterId(), Track::AUDIO_OUTPUT);
						if(master)
						{
							Route srcRoute3(buss, 0, buss->channels());
							Route dstRoute3(master->name(), true, -1);
						
							audio->msgAddRoute(srcRoute3, dstRoute3);
						}
					}
					if(buss)
						configureVerb(buss, config.minSlider, 0.0);
					if(input)
					{//Do we have to remove that old route to the synth port, evaluate later
						configureVerb(input, config.minSlider, 0.0);
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
						break;
					}
				}
				track->setSamplerData(0);
				oom->changeConfig(true); // save configuration file
				song->update(SC_MIDI_TRACK_PROP);
			}
			break;
			default:
			{//External manual cannections required
				MidiInstrument* ins = 0;
				for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
				{
					if ((*i)->iname() == instrument)
					{
						ins = *i;
						break;
					}
				}
				if(!ins)
				{//Reset GUI widgets that made this call and return
					return;
				}
				if(isSynth)
				{
					SynthPluginDevice* synth = (SynthPluginDevice*)md;
					if (synth->duplicated())
					{
						midiDevices.remove(md);
						synth->close();
					}
					mp->setInstrument(ins);
				}
				else
				{//Maybe remove the channel in LS if it was an LS instrument
					if(oldins->isOOMInstrument() && track->samplerData())
					{//Delete sampler channel and null the track sampler data
						if(!lsClient)
						{
							lsClient = new LSClient(config.lsClientHost, config.lsClientPort);
							lsClientStarted = lsClient->startClient();
						}
						else if(!lsClientStarted)
						{
							lsClientStarted = lsClient->startClient();
						}
						if(lsClientStarted)
						{
							lsClient->removeInstrumentChannel(track->samplerData());
						}
						midiDevices.remove(md);
					}
					mp->setInstrument(ins);
				}
				track->setSamplerData(0);
				track->setWantsAutomation(false);
				midiSeq->msgSetMidiDevice(mp, 0);
				song->update(SC_MIDI_TRACK_PROP);
			}
			break;
		}
		song->update(SC_MIDI_TRACK_PROP);
		song->update(SC_TRACK_INSTRUMENT);
		song->dirty = true;
	}
}/*}}}*/

void TrackManager::removeTrack(qint64 id)/*{{{*/
{
	Track* track = song->findTrackById(id);
	if(track)
	{
		QList<qint64> idList;
		QStringList names;
		if(track->masterFlag())
		{//Find children
			idList.append(track->id());
			names.append(track->name());
			if(track->hasChildren())
			{
				QList<qint64> *chain = track->audioChain();
				for(int i = 0; i < chain->size(); i++)
				{
					Track* child = song->findTrackById(chain->at(i));
					if(child && child->chainMaster() == id)
					{
						names.append(child->name());
						idList.append(child->id());
					}
				}
			}
		}
		else
		{//Find parent and siblings
			Track *ptrack = song->findTrackById(track->chainMaster());
			if(ptrack && ptrack->chainContains(id))
			{//We're good
				idList.append(ptrack->id());
				names.append(ptrack->name());
				if(ptrack->hasChildren())
				{
					QList<qint64> *chain = ptrack->audioChain();
					for(int i = 0; i < chain->size(); i++)
					{
						Track* child = song->findTrackById(chain->at(i));
						if(child)
						{
							names.append(child->name());
							idList.append(child->id());
						}
					}
				}
			}
			else
			{//Unparented track, maybe old file, just delete it
				idList.append(track->id());
				names.append(track->name());
			}
		}
		//Process list
		if(idList.size())
		{
			QString msg(tr("You are about to delete the following track(s) \n%1 \nAre you sure this is what you want?"));
			if(QMessageBox::question(oom, 
				tr("Delete Track(s)"),
				msg.arg(names.join("\n")),
				QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
			{
				audio->msgRemoveTrackGroup(idList, true);
			}
		}
	}
}/*}}}*/

void TrackManager::removeSelectedTracks()/*{{{*/
{
	QList<qint64> selected = song->selectedTracks();
	if(selected.isEmpty())
		return;
	QList<qint64> idList;
	QStringList names;
	for(int t = 0; t < selected.size(); t++)
	{
		qint64 id =  selected.at(t);
		Track* track = song->findTrackById(id);
		if(track)
		{
			if(track->masterFlag())
			{//Find children
				if(!idList.contains(track->id()))
					idList.append(track->id());
				if(!names.contains(track->name()))
					names.append(track->name());
				if(track->hasChildren())
				{
					QList<qint64> *chain = track->audioChain();
					for(int i = 0; i < chain->size(); i++)
					{
						Track* child = song->findTrackById(chain->at(i));
						if(child && child->chainMaster() == id)
						{
							if(!names.contains(child->name()))
								names.append(child->name());
							if(!idList.contains(child->id()))
								idList.append(child->id());
						}
					}
				}
			}
			else
			{//Find parent and siblings
				Track *ptrack = song->findTrackById(track->chainMaster());
				if(ptrack && ptrack->chainContains(id))
				{//We're good
					if(!idList.contains(ptrack->id()))
						idList.append(ptrack->id());
					if(!names.contains(ptrack->name()))
						names.append(ptrack->name());
					if(ptrack->hasChildren())
					{
						QList<qint64> *chain = ptrack->audioChain();
						for(int i = 0; i < chain->size(); i++)
						{
							Track* child = song->findTrackById(chain->at(i));
							if(child)
							{
								if(!names.contains(child->name()))
									names.append(child->name());
								if(!idList.contains(child->id()))
									idList.append(child->id());
							}
						}
					}
				}
				else
				{//Unparented track, maybe old file, just delete it
					if(!idList.contains(track->id()))
						idList.append(track->id());
					if(!names.contains(track->name()))
						names.append(track->name());
				}
			}
		}
	}
	//Process list
	if(idList.size())
	{
		QString msg(tr("You are about to delete the following track(s) \n%1 \nAre you sure this is what you want?"));
		if(QMessageBox::question(oom, 
			tr("Delete Track(s)"),
			msg.arg(names.join("\n")),
			QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
		{
			audio->msgRemoveTrackGroup(idList, true);
		}
	}
}/*}}}*/

bool TrackManager::removeTrack(VirtualTrack* vtrack)/*{{{*/
{
	bool rv = false;
	Q_UNUSED(vtrack);
	return rv;
}/*}}}*/

bool TrackManager::loadInstrument(VirtualTrack *vtrack)/*{{{*/
{
	bool rv = true;
	Track::TrackType type = (Track::TrackType)vtrack->type;
	if(type == Track::MIDI)
	{
		QString instrumentName = vtrack->instrumentName;
		QString trackName;
		if(m_track)
		{
			trackName = m_track->name();
		}
		else
			trackName = vtrack->name;
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
							if(debugMsg)
								qDebug("TrackManager::loadInstrument: Loading Instrument to LinuxSampler");
							if(lsClient->loadInstrument(*i))
							{
								rv = true;
								if(debugMsg)
									qDebug("TrackManager::loadInstrument: Instrument Map Loaded");
								if(vtrack->autoCreateInstrument)
								{
									int map = lsClient->findMidiMap((*i)->iname().toUtf8().constData());
									Patch* p = (*i)->getDefaultPatch();
									if(p && map >= 0)
									{
										SamplerData* data;
										if(lsClient->createInstrumentChannel(trackName.toUtf8().constData(), p->engine.toUtf8().constData(), p->filename.toUtf8().constData(), p->index, map, &data))
										{
											if(data)
											{
												((MidiTrack*)m_track)->setSamplerData(data);
											}
											if(debugMsg)
												qDebug("TrackManager::loadInstrument: Created Channel for track");
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
                rv = false;
                if (portIdx >= 0)
                {
                    for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
                    {
                        if ((*i)->isSynthPlugin() && (*i)->name() == instrumentName)
                        {
							QString devName(QString("O-").append(m_track->name()));
                            SynthPluginDevice* oldSynth = (SynthPluginDevice*)(*i);
                            SynthPluginDevice* synth = oldSynth->clone(devName);
                            synth->open();

                            midiSeq->msgSetMidiDevice(&midiPorts[portIdx], synth);

                            QString selectedInput  = synth->getAudioOutputPortName(0);
                            QString selectedInput2 = synth->getAudioOutputPortName(1);

							m_midiOutPort = portIdx;
							if(debugMsg)
								qDebug("TrackManager::loadInstrument: Port Names: left: %s, right: %s", selectedInput.toUtf8().constData(), selectedInput2.toUtf8().constData());
							m_synthConfigs.clear();
							m_synthConfigs.append(qMakePair(portIdx, devName));
                            m_synthConfigs.append(qMakePair(0, selectedInput));
                            m_synthConfigs.append(qMakePair(0, selectedInput2));
							rv = true;

                            break;
                        }
                    }
                }
			}
			break;
			case GM_INSTRUMENT:  //Regular idf no linuxsampler
			{
				rv = true;
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
	return rv;
}/*}}}*/

void TrackManager::configureVerb(Track* track, double auxval, double panval)/*{{{*/
{
	qint64 verb = song->oomVerbId();
	if(verb)
	{
		double aux;
		if (auxval <= config.minSlider)
		{
			aux = 0.0;
		}
		else
			aux = pow(10.0, auxval / 20.0);

		if(track)
		{
			//Setup reverb value
			audio->msgSetAux((AudioTrack*) track, verb, aux);
			song->update(SC_AUX);

			//Setup Pan value
			AutomationType at = ((AudioTrack*) track)->automationType();
			if (at == AUTO_WRITE || (audio->isPlaying() && at == AUTO_TOUCH))
				track->enablePanController(false);

			if(debugMsg)
				qDebug("TrackManager::configureVerb: pan: %f, verb: %f", aux, panval);
			audio->msgSetPan(((AudioTrack*) track), panval);
			((AudioTrack*) track)->recordAutomation(AC_PAN, panval);
			
			audio->msgUpdateSoloStates();
		}
	}
}/*}}}*/

//newBuss, useBuss, 
void TrackManager::createMonitorInputTracks(VirtualTrack* vtrack)/*{{{*/
{
	bool newBuss = vtrack->bussConfig.first;
	QString name(m_track->name());

	QString inputName = QString("i").append(name);
	QString bussName = QString("B").append(name);
	//QString audioName = QString("A-").append(name);
	//if(m_track)
	Track* input = m_track->inputTrack();
	//Track* input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false, false);
	Track* buss = 0;
	if(vtrack->useBuss)
	{
		if(newBuss)
		{
			buss = song->addTrackByName(bussName, Track::AUDIO_BUSS, -1, false, true);
			if(buss)
			{
				buss->setMasterFlag(false);
				buss->setChainMaster(m_track->id());
				m_track->addManagedTrack(buss->id());
			}
		}
		else
			buss = song->findTrack(vtrack->bussConfig.second);
	}
	if(input)
	{
		song->undoOp(UndoOp::AddTrack, -1, input);
		input->setMasterFlag(false);
		input->setChainMaster(m_track->id());
		m_track->addManagedTrack(input->id());
		input->setMute(false);
		QString selectedInput;
		if(vtrack->instrumentType == SYNTH_INSTRUMENT)
			selectedInput = m_synthConfigs.at(1).second;
		else
			selectedInput = vtrack->monitorConfig.second;
        QString selectedInput2;

		if(vtrack->instrumentType == SYNTH_INSTRUMENT)
		{
			if(m_synthConfigs.at(2).second.isEmpty())
        	    selectedInput2 = selectedInput;
			else
				selectedInput2 = m_synthConfigs.at(2).second;
		}
		else
		{
			if (vtrack->monitorConfig2.second.isEmpty())
        	    selectedInput2 = selectedInput;
			else
        	    selectedInput2 = vtrack->monitorConfig2.second;
		}
		if(debugMsg)
			qDebug("TrackManager::createMonitorInputTracks: Port Names: left: %s, right: %s", selectedInput.toUtf8().constData(), selectedInput2.toUtf8().constData());

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
			song->undoOp(UndoOp::AddTrack, -1, buss);
			Route srcRoute(input, 0, input->channels());
			Route dstRoute(buss->name(), true, -1);

			audio->msgAddRoute(srcRoute, dstRoute);
		}

		//Route audio to master
		Track* master = song->findTrackByIdAndType(song->masterId(), Track::AUDIO_OUTPUT);
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
		//Set the predefined pan and reverb level on buss or input
		qint64 verb = song->oomVerbId();
		if(verb)
		{
			double auxval = vtrack->instrumentVerb, panval = vtrack->instrumentPan;

			if(vtrack->useBuss && buss)
			{
				//Setup reverb value
				configureVerb(buss, auxval, panval);
				configureVerb(input, config.minSlider, 0.0);
			}
			else if(!vtrack->useBuss && input)
			{
				//Setup reverb value
				configureVerb(input, auxval, panval);
			}
		}

		audio->msgUpdateSoloStates();
		song->update(SC_ROUTE);
	}
}/*}}}*/

void TrackManager::removeMonitorInputTracks(VirtualTrack* vtrack)/*{{{*/
{
	Q_UNUSED(vtrack);
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

