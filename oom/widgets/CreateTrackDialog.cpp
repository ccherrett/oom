//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "CreateTrackDialog.h"
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


CreateTrackDialog::CreateTrackDialog(int type, int pos, QWidget* parent)
: QDialog(parent),
m_insertType(type),
m_insertPosition(pos)
{
	setupUi(this);
	m_createMidiInputDevice = false;
	m_createMidiOutputDevice = false;
	
	m_midiSameIO = false;
	m_createTrackOnly = false;
	m_showJackAliases = -1;
	
	m_midiInPort = -1;
	m_midiOutPort = -1;

	m_lsClient = 0;
	m_clientStarted = false;

    m_height = 290;
	m_width = 450;

	m_allChannelBit = (1 << MIDI_CHANNELS) - 1;

	cmbType->addItem(*addMidiIcon, tr("Midi"), Track::MIDI);
	cmbType->addItem(*addAudioIcon, tr("Audio"), Track::WAVE);
    cmbType->addItem(*addSynthIcon, tr("Synth"), Track::AUDIO_SOFTSYNTH);
	cmbType->addItem(*addOutputIcon, tr("Output"), Track::AUDIO_OUTPUT);
	cmbType->addItem(*addInputIcon, tr("Input"), Track::AUDIO_INPUT);
	cmbType->addItem(*addBussIcon, tr("Buss"), Track::AUDIO_BUSS);
	cmbType->addItem(*addAuxIcon, tr("Aux Send"), Track::AUDIO_AUX);

	cmbInChannel->addItem(tr("All"), m_allChannelBit);
	for(int i = 0; i < 16; ++i)
	{
		cmbInChannel->addItem(QString(tr("Chan ")).append(QString::number(i+1)), 1 << i);
	}
	cmbInChannel->setCurrentIndex(1);
	
	//cmbOutChannel->addItem(tr("All"), m_allChannelBit);
	for(int i = 0; i < 16; ++i)
	{
		cmbOutChannel->addItem(QString(tr("Chan ")).append(QString::number(i+1)), i);
	}
	cmbOutChannel->setCurrentIndex(0);
	
	int row = cmbType->findData(m_insertType);
	cmbType->setCurrentIndex(row);

	connect(chkInput, SIGNAL(toggled(bool)), this, SLOT(updateInputSelected(bool)));
	connect(chkOutput, SIGNAL(toggled(bool)), this, SLOT(updateOutputSelected(bool)));
	connect(chkBuss, SIGNAL(toggled(bool)), this, SLOT(updateBussSelected(bool)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), this, SLOT(trackTypeChanged(int)));
	connect(cmbInstrument, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInstrument(int)));
	connect(btnAdd, SIGNAL(clicked()), this, SLOT(addTrack()));
}

//Add button slot
void CreateTrackDialog::addTrack()/*{{{*/
{
	if(txtName->text().isEmpty())
		return;
	
	int inputIndex = cmbInput->currentIndex();
	int outputIndex = cmbOutput->currentIndex();
	int instrumentIndex = cmbInstrument->currentIndex();
	int monitorIndex = cmbMonitor->currentIndex();
	int inChanIndex = cmbInChannel->currentIndex();
	int outChanIndex = cmbOutChannel->currentIndex();

	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{

			Track* track =  song->addTrackByName(txtName->text(), Track::MIDI, m_insertPosition, false);
			if(track)
			{
				MidiTrack* mtrack = (MidiTrack*)track;
				//Process Input connections
				if(inputIndex >= 0 && chkInput->isChecked())
				{
					QString devname = cmbInput->itemText(inputIndex);
					if(m_currentMidiInputList.isEmpty())
					{
						m_createMidiInputDevice = true;
					}
					else
					{
						m_createMidiInputDevice = !(m_currentMidiInputList.contains(inputIndex) && m_currentMidiInputList.value(inputIndex) == devname);
					}
					MidiPort* inport = 0;
					MidiDevice* indev = 0;
					QString inputDevName(QString("I-").append(track->name()));
					if(m_createMidiInputDevice)
					{
						m_midiInPort = getFreeMidiPort();
						qDebug("m_createMidiInputDevice is set: %i", m_midiInPort);
						inport = &midiPorts[m_midiInPort];
						int devtype = cmbInput->itemData(inputIndex).toInt();
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
						m_midiInPort = cmbInput->itemData(inputIndex).toInt();
						qDebug("Using existing MIDI port: %i", m_midiInPort);
						inport = &midiPorts[m_midiInPort];
						if(inport)
							indev = inport->device();
					}
					if(inport && indev)
					{
						qDebug("MIDI Input port and MIDI devices found, Adding final input routing");
						int chanbit = cmbInChannel->itemData(inChanIndex).toInt();
						Route srcRoute(m_midiInPort, chanbit);
						Route dstRoute(track, chanbit);

						audio->msgAddRoute(srcRoute, dstRoute);

						audio->msgUpdateSoloStates();
						song->update(SC_ROUTE);
					}
				}
				
				//Process Output connections
				if(outputIndex >= 0 && chkOutput->isChecked())
				{
					MidiPort* outport= 0;
					MidiDevice* outdev = 0;
					QString devname = cmbOutput->itemText(outputIndex);
					if(m_currentMidiOutputList.isEmpty())
					{
						m_createMidiOutputDevice = true;
					}
					else
					{
						m_createMidiOutputDevice = !(m_currentMidiOutputList.contains(outputIndex) && m_currentMidiOutputList.value(outputIndex) == devname);
					}
					QString outputDevName(QString("O-").append(track->name()));
					if(m_createMidiOutputDevice)
					{
						m_midiOutPort = getFreeMidiPort();
						qDebug("m_createMidiOutputDevice is set: %i", m_midiOutPort);
						outport = &midiPorts[m_midiOutPort];
						int devtype = cmbOutput->itemData(outputIndex).toInt();
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
						m_midiOutPort = cmbOutput->itemData(outputIndex).toInt();
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
						int outChan = cmbOutChannel->itemData(outChanIndex).toInt();
						mtrack->setOutChanAndUpdate(outChan);
						audio->msgIdle(false);
						if(m_createMidiOutputDevice)
						{
							QString instrumentName = cmbInstrument->itemText(instrumentIndex);
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

				if(monitorIndex >= 0 && midiBox->isChecked())
				{
					createMonitorInputTracks(track->name());
				}

				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
			}
		}
		break;
		case Track::WAVE:
		{
			Track* track =  song->addTrackByName(txtName->text(), Track::WAVE, m_insertPosition, !chkOutput->isChecked());
			if(track)
			{
				if(inputIndex >= 0 && chkInput->isChecked())
				{
					QString inputName = QString("i").append(track->name());
					QString selectedInput = cmbInput->itemText(inputIndex);
					bool addNewRoute = cmbInput->itemData(inputIndex).toBool();
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
				if(outputIndex >= 0 && chkOutput->isChecked())
				{
					//Route to the Output or Buss
					QString selectedOutput = cmbOutput->itemText(outputIndex);
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
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			Track* track = song->addTrackByName(txtName->text(), Track::AUDIO_OUTPUT, -1, false);
			if(track)
			{
				if(inputIndex >= 0 && chkInput->isChecked())
				{
					QString selectedInput = cmbInput->itemText(inputIndex);
					Route dstRoute(track, 0, track->channels());
					Route srcRoute(selectedInput, true, -1);

					audio->msgAddRoute(srcRoute, dstRoute);

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}

				if(outputIndex >= 0 && chkOutput->isChecked())
				{
					QString jackPlayback("system:playback");
					QString selectedOutput = cmbOutput->itemText(outputIndex);
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
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			Track* track = song->addTrackByName(txtName->text(), Track::AUDIO_INPUT, -1, false);
			if(track)
			{
				track->setMute(false);
				if(inputIndex >= 0 && chkInput->isChecked())
				{
					QString selectedInput = cmbInput->itemText(inputIndex);

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
				if(outputIndex >= 0 && chkOutput->isChecked())
				{
					QString selectedOutput = cmbOutput->itemText(outputIndex);

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
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			Track* track = song->addTrackByName(txtName->text(), Track::AUDIO_BUSS, -1, false);
			if(track)
			{
				if(inputIndex >= 0 && chkInput->isChecked())
				{
					QString selectedInput = cmbInput->itemText(inputIndex);
					Route srcRoute(selectedInput, true, -1);
					Route dstRoute(track, 0, track->channels());

					audio->msgAddRoute(srcRoute, dstRoute);

					audio->msgUpdateSoloStates();
					song->update(SC_ROUTE);
				}
				if(outputIndex >= 0 && chkOutput->isChecked())
				{
					QString selectedOutput = cmbOutput->itemText(outputIndex);
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
			}
		}
		break;
		case Track::AUDIO_AUX:
		{
			Track* track = song->addTrackByName(txtName->text(), Track::AUDIO_AUX, -1, false);
			if(track)
			{
				midiMonitor->msgAddMonitoredTrack(track);
				song->deselectTracks();
				track->setSelected(true);
				emit trackAdded(track->name());
			}
		}
		break;
		case Track::AUDIO_SOFTSYNTH:
		{
			//Just add the track type and rename it
            Track* track = song->addTrackByName(txtName->text(), Track::AUDIO_SOFTSYNTH, -1, false);
            if(track)
            {
                midiMonitor->msgAddMonitoredTrack(track);
                song->deselectTracks();
                track->setSelected(true);
                emit trackAdded(track->name());
            }
		}
		break;
	}
	if(m_lsClient && m_clientStarted)
	{
		m_lsClient->stopClient();
	}
	done(0);
}/*}}}*/

void CreateTrackDialog::updateInstrument(int index)
{
	QString instrumentName = cmbInstrument->itemText(index);
	for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
	{
		if ((*i)->iname() == instrumentName && (*i)->isOOMInstrument())
		{
			if(!m_lsClient)
			{
				m_lsClient = new LSClient(config.lsClientHost.toUtf8().constData(), config.lsClientPort);
				m_clientStarted = m_lsClient->startClient();
			}
			else if(!m_clientStarted)
			{
				m_clientStarted = m_lsClient->startClient();
			}
			if(m_clientStarted)
			{
				qDebug("Loadin`g Instrument to LinuxSampler");
				if(m_lsClient->loadInstrument(*i))
				{
					qDebug("Instrument Loaded");
					QString prefix("LinuxSampler:");
					QString postfix("-audio");
					QString audio(QString(prefix).append(instrumentName).append(postfix));
					QString midi(QString(prefix).append(instrumentName));
					//reload input/output list and select the coresponding ports respectively
					updateVisibleElements();
					//populateInputList();
					populateOutputList();
					populateMonitorList();
					cmbOutput->setCurrentIndex(cmbOutput->findText(midi));
					cmbMonitor->setCurrentIndex(cmbMonitor->findText(audio));
				}
			}
			break;
		}
	}
}

//Input raw slot
void CreateTrackDialog::updateInputSelected(bool raw)/*{{{*/
{
	cmbInput->setEnabled(raw);
	cmbInChannel->setEnabled(raw);
}/*}}}*/

//Output raw slot
void CreateTrackDialog::updateOutputSelected(bool raw)/*{{{*/
{
	cmbOutput->setEnabled(raw);
	cmbOutChannel->setEnabled(raw);
}/*}}}*/

void CreateTrackDialog::updateBussSelected(bool raw)/*{{{*/
{
	cmbBuss->setEnabled(raw);
}/*}}}*/

//Track type combo slot
void CreateTrackDialog::trackTypeChanged(int type)
{
	m_insertType = cmbType->itemData(type).toInt();
	updateVisibleElements();
	populateInputList();
	populateOutputList();
	populateInstrumentList();
	populateMonitorList();
	populateBussList();
}

void CreateTrackDialog::createMonitorInputTracks(QString name)/*{{{*/
{
	int monitorIndex = cmbMonitor->currentIndex();
	int bussIndex = cmbBuss->currentIndex();
	bool newBuss = cmbBuss->itemData(bussIndex).toBool();

	QString inputName = QString("i").append(name);
	QString bussName = QString("B").append(name);
	//QString audioName = QString("A-").append(name);
	Track* input = song->addTrackByName(inputName, Track::AUDIO_INPUT, -1, false);
	Track* buss = 0;
	if(chkBuss->isChecked())
	{
		if(newBuss)
			buss = song->addTrackByName(bussName, Track::AUDIO_BUSS, -1, true);
		else
			buss = song->findTrack(cmbBuss->itemText(bussIndex));
	}
	//Track* audiot = song->addTrackByName(audioName, Track::WAVE, -1, false);
	if(input/* && audiot*/)
	{
		input->setMute(false);
		QString selectedInput = cmbMonitor->itemText(monitorIndex);
		
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
		
		if(chkBuss->isChecked() && buss)
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
			if(chkBuss->isChecked() && buss)
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

//Populate input combo based on type
void CreateTrackDialog::populateInputList()/*{{{*/
{
	while(cmbInput->count())
		cmbInput->removeItem(cmbInput->count()-1);
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			m_currentMidiInputList.clear();
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if ((md->openFlags() & 2))
				{
					QString mdname(md->name());
					if(md->deviceType() == MidiDevice::ALSA_MIDI)
					{
						mdname = QString("(OOMidi) ").append(mdname);
					}
					cmbInput->addItem(mdname, i);
					m_currentMidiInputList.insert(cmbInput->count()-1, mdname);
				}
			}
			populateNewInputList();

			if (!cmbInput->count())
			{
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::WAVE:
		{
			for(iTrack it = song->inputs()->begin(); it != song->inputs()->end(); ++it)
			{
				cmbInput->addItem((*it)->name(), 0);
			}
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack() || (*t)->type() == Track::AUDIO_OUTPUT)
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				Route r(track, -1);
				cmbInput->addItem(r.name());
			}

			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				if(track->type() == Track::AUDIO_INPUT || track->type() == Track::WAVE || track->type() == Track::AUDIO_SOFTSYNTH)
				{
					cmbInput->addItem(Route(track, -1).name());
				}
			}
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(false);
				chkInput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
		break;
	}
}/*}}}*/

void CreateTrackDialog::populateOutputList()/*{{{*/
{
	while(cmbOutput->count())
		cmbOutput->removeItem(cmbOutput->count()-1);
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			m_currentMidiOutputList.clear();
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if((md->openFlags() & 1))
				{
					QString mdname(md->name());
					if(md->deviceType() == MidiDevice::ALSA_MIDI)
					{
						mdname = QString("(OOMidi) ").append(mdname);
					}
					cmbOutput->addItem(mdname, i);
					m_currentMidiOutputList.insert(cmbOutput->count()-1, mdname);
				}
			}

			populateNewOutputList();
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::WAVE:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				if(track->type() == Track::AUDIO_OUTPUT || track->type() == Track::AUDIO_BUSS)
				{
					cmbOutput->addItem(Route(track, -1).name());
				}
			}
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			importInputs();
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				switch(track->type())
				{
					case Track::AUDIO_OUTPUT:
					case Track::AUDIO_BUSS:
					case Track::WAVE:
						cmbOutput->addItem(Route(track, -1).name());
					break;
					default:
					break;
				}
			}
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			for(iTrack t = song->outputs()->begin(); t != song->outputs()->end(); ++t)
			{
				AudioTrack* track = (AudioTrack*) (*t);
				cmbOutput->addItem(Route(track, -1).name());
			}
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(false);
				chkOutput->setEnabled(false);
			}
		}
		break;
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
		break;
	}
}/*}}}*/

void CreateTrackDialog::importOutputs()/*{{{*/
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbInput->addItem(*i, 1);
		}
	}
}/*}}}*/

void CreateTrackDialog::importInputs()/*{{{*/
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->inputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbOutput->addItem(*i, 1);
		}
	}
}/*}}}*/

void CreateTrackDialog::populateMonitorList()/*{{{*/
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
}/*}}}*/

void CreateTrackDialog::populateBussList()/*{{{*/
{
	while(cmbBuss->count())
		cmbBuss->removeItem(cmbBuss->count()-1);
	cmbBuss->addItem(tr("Add New Buss"), 1);
	for(iTrack it = song->groups()->begin(); it != song->groups()->end(); ++it)
	{
		cmbBuss->addItem((*it)->name(), 0);
	}
}/*}}}*/

void CreateTrackDialog::populateNewInputList()/*{{{*/
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
}/*}}}*/

void CreateTrackDialog::populateNewOutputList()/*{{{*/
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
}/*}}}*/

void CreateTrackDialog::populateInstrumentList()/*{{{*/
{
    cmbInstrument->clear();

    if (m_insertType == Track::MIDI)
    {
        for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
        {
            //SynthI* si = dynamic_cast<SynthI*> (*i);
            //if (!si)
                cmbInstrument->addItem((*i)->iname());
        }
        //Default to the GM instrument
        int gm = cmbInstrument->findText("GM");
        if(gm >= 0)
            cmbInstrument->setCurrentIndex(gm);
    }
    else if (m_insertType == Track::AUDIO_SOFTSYNTH)
    {
        for (iPlugin i = plugins.begin(); i != plugins.end(); ++i)
        {
            if (i->hints() & PLUGIN_IS_SYNTH)
                cmbInstrument->addItem(i->name());
        }
    }
}/*}}}*/

int CreateTrackDialog::getFreeMidiPort()/*{{{*/
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

void CreateTrackDialog::updateVisibleElements()/*{{{*/
{
	chkInput->setEnabled(true);
	chkOutput->setEnabled(true);
	chkInput->setChecked(true);
	chkOutput->setChecked(true);
	chkBuss->setChecked(true);

	Track::TrackType type = (Track::TrackType)m_insertType;
	switch (type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			cmbInChannel->setVisible(true);
			cmbOutChannel->setVisible(true);
			lblInstrument->setVisible(true);
			cmbInstrument->setVisible(true);
			cmbMonitor->setVisible(true);
			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			midiBox->setVisible(true);

            m_height = 290;
			m_width = width();
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBox->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			m_height = 160;
			m_width = width();
		}
		break;
		case Track::AUDIO_INPUT:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBox->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			m_height = 160;
			m_width = width();
		}
		break;
		case Track::WAVE:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBox->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			m_height = 160;
			m_width = width();
		}
		break;
		case Track::AUDIO_BUSS:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			midiBox->setVisible(false);

			cmbInput->setVisible(true);
			chkInput->setVisible(true);
			cmbOutput->setVisible(true);
			chkOutput->setVisible(true);
			
			m_height = 160;
			m_width = width();
		}
		break;
		case Track::AUDIO_AUX:
		{
			cmbInChannel->setVisible(false);
			cmbOutChannel->setVisible(false);
			lblInstrument->setVisible(false);
			cmbInstrument->setVisible(false);
			cmbMonitor->setVisible(false);
			
			midiBox->setVisible(false);
			
			cmbInput->setVisible(false);
			chkInput->setVisible(false);

			cmbOutput->setVisible(false);
			chkOutput->setVisible(false);
			
			m_height = 100;
			m_width = width();
		}
		case Track::AUDIO_SOFTSYNTH:
		{
            cmbInChannel->setVisible(false);
            cmbOutChannel->setVisible(false);
            lblInstrument->setVisible(true);
            cmbInstrument->setVisible(true);
            cmbMonitor->setVisible(false);

            midiBox->setVisible(false);

            cmbInput->setVisible(false);
            chkInput->setVisible(false);

            cmbOutput->setVisible(false);
            chkOutput->setVisible(false);

            m_height = 130;
            m_width = width();
		}
		break;
		default:
		break;
	}
	setFixedHeight(m_height);
	updateGeometry();
}/*}}}*/

void CreateTrackDialog::showEvent(QShowEvent*)
{
	qDebug("Inside CreateTrackDialog::showEvent trackType: %i, position: %i", m_insertType, m_insertPosition);
	updateVisibleElements();
	populateInputList();
	populateOutputList();
	populateInstrumentList();
	populateMonitorList();
	populateBussList();
}

/*QSize CreateTrackDialog::sizeHint()
{
	return QSize(m_width, m_height);
	//return QSize(450, 100);
}*/
