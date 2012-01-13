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
#include "icons.h"
#include "midimonitor.h"


CreateTrackDialog::CreateTrackDialog(int type, int pos, QWidget* parent)
: QDialog(parent),
m_insertType(type),
m_insertPosition(pos)
{
	setupUi(this);
	m_createMidiInputDevice = false;
	m_createMidiOutputDevice = false;
	m_createTrackOnly = false;
	m_showJackAliases = -1;
	m_midiInPort = -1;
	m_midiOutPort = -1;
	txtInChannel->setValue(1);
	txtOutChannel->setValue(1);
	
	btnNewInput->setIcon(*plusIconSet3);
	btnNewOutput->setIcon(*plusIconSet3);

	cmbType->addItem(*addMidiIcon, tr("Midi"), Track::MIDI);
	cmbType->addItem(*addAudioIcon, tr("Audio"), Track::WAVE);
	cmbType->addItem(*addOutputIcon, tr("Output"), Track::AUDIO_OUTPUT);
	cmbType->addItem(*addInputIcon, tr("Input"), Track::AUDIO_INPUT);
	cmbType->addItem(*addBussIcon, tr("Buss"), Track::AUDIO_BUSS);
	cmbType->addItem(*addAuxIcon, tr("Aux Send"), Track::AUDIO_AUX);
	int row = cmbType->findData(m_insertType);
	cmbType->setCurrentIndex(row);

	connect(btnNewInput, SIGNAL(toggled(bool)), this, SLOT(updateInputSelected(bool)));
	connect(btnNewOutput, SIGNAL(toggled(bool)), this, SLOT(updateOutputSelected(bool)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), this, SLOT(trackTypeChanged(int)));
	connect(btnAdd, SIGNAL(clicked()), this, SLOT(addTrack()));
}

//Add button slot
void CreateTrackDialog::addTrack()
{
	if(txtName->text().isEmpty())
		return;
	
	int inputIndex = cmbInput->currentIndex();
	int outputIndex = cmbOutput->currentIndex();
	int instrumentIndex = cmbInstrument->currentIndex();

	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			int toggleAllBit = (1 << MIDI_CHANNELS) - 1;

			Track* track =  song->addTrackByName(txtName->text(), Track::MIDI, m_insertPosition, false);
			if(track)
			{
				MidiTrack* mtrack = (MidiTrack*)track;
				//Process Input connections
				if(inputIndex >= 0)
				{
					MidiPort* inport = 0;
					MidiDevice* indev = 0;
					if(m_createMidiInputDevice)
					{
						m_midiInPort = getFreeMidiPort();
						qDebug("m_createMidiInputDevice is set: %i", m_midiInPort);
						inport = &midiPorts[m_midiInPort];
						int devtype = cmbInput->itemData(inputIndex).toInt();
						QString devname = cmbInput->itemText(inputIndex);
						if(devtype == MidiDevice::ALSA_MIDI)
						{
							indev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
							if(indev)
							{
								qDebug("Found MIDI input device: ALSA_MIDI");
								midiSeq->msgSetMidiDevice(inport, indev);
							}
						}
						else if(devtype == MidiDevice::JACK_MIDI)
						{
							indev = MidiJackDevice::createJackMidiDevice(devname, 3);
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
						int chanbit = 1 << 0;
						int inChan = txtInChannel->value();
						if(!inChan)
						{
							chanbit = toggleAllBit;
						}
						else
						{
							chanbit = 1 << (inChan - 1);
						}
						Route srcRoute(m_midiInPort, chanbit);
						Route dstRoute(track, chanbit);

						audio->msgAddRoute(srcRoute, dstRoute);

						audio->msgUpdateSoloStates();
						song->update(SC_ROUTE);
					}
				}
				
				//Process Output connections
				if(outputIndex >= 0)
				{
					MidiPort* outport= 0;
					MidiDevice* outdev = 0;
					if(m_createMidiOutputDevice)
					{
						m_midiOutPort = getFreeMidiPort();
						qDebug("m_createMidiOutputDevice is set: %i", m_midiOutPort);
						outport = &midiPorts[m_midiOutPort];
						int devtype = cmbOutput->itemData(outputIndex).toInt();
						QString devname = cmbOutput->itemText(outputIndex);
						if(devtype == MidiDevice::ALSA_MIDI)
						{
							outdev = midiDevices.find(devname, MidiDevice::ALSA_MIDI);
							if(outdev)
							{
								qDebug("Found MIDI output device: ALSA_MIDI");
								midiSeq->msgSetMidiDevice(outport, outdev);
							}
						}
						else if(devtype == MidiDevice::JACK_MIDI)
						{
							outdev = MidiJackDevice::createJackMidiDevice(devname, 3);
							if(outdev)
							{
								int openFlags = 0;
								openFlags ^= 0x1;
								qDebug("Created MIDI output device: JACK_MIDI");
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
						int outChan = txtOutChannel->value();
						--outChan;
						mtrack->setOutChanAndUpdate(outChan);
						audio->msgIdle(false);
						QString instrumentName = cmbInstrument->itemText(instrumentIndex);
						for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
						{
							if ((*i)->iname() == instrumentName)
							{
								outport->setInstrument(*i);
								break;
							}
						}
						song->update(SC_MIDI_TRACK_PROP);
					}
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
		}
		break;
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
		{
			//Just add the track type and rename it
		}
		break;
	}
	done(1);
}

//Input raw slot
void CreateTrackDialog::updateInputSelected(bool raw)
{
	if(raw)
	{
		populateNewInputList();
		m_createMidiInputDevice = true;
	}
	else
	{
		populateInputList();
		m_createMidiInputDevice = false;
	}
}

//Output raw slot
void CreateTrackDialog::updateOutputSelected(bool raw)
{
	if(raw)
	{
		populateNewOutputList();
		m_createMidiOutputDevice = true;
	}
	else
	{
		populateOutputList();
		m_createMidiOutputDevice = false;
	}
}

//Track type combo slot
void CreateTrackDialog::trackTypeChanged(int type)
{
	m_insertType = cmbType->itemData(type).toInt();
	showAllElements();
	populateInputList();
	populateOutputList();
	populateInstrumentList();
}

void CreateTrackDialog::setTitleText()/*{{{*/
{
	/*QString trackLabel(tr("Create new %1 track"));
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			lblType->setText(trackLabel.arg(tr("MIDI")));
		}
		break;
		case Track::WAVE:
		{
			lblType->setText(trackLabel.arg(tr("Audio")));
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			lblType->setText(trackLabel.arg(tr("Output")));
		}
		break;
		case Track::AUDIO_INPUT:
		{
			lblType->setText(trackLabel.arg(tr("Input")));
		}
		break;
		case Track::AUDIO_BUSS:
		{
			lblType->setText(trackLabel.arg(tr("Buss")));
		}
		break;
		case Track::AUDIO_AUX:
		{
			lblType->setText(trackLabel.arg(tr("Aux")));
		}
		break;
		case Track::AUDIO_SOFTSYNTH:
		{
			lblType->setText(trackLabel.arg(tr("Synth")));
		}
		break;
	}*/
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
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if ((md->openFlags() & 2))
					cmbInput->addItem(md->name(), i);

				//Add global menu toggle
				//int ch1bit = 1 << 0;
				//Route myRoute(i, ch1bit);
			}

			if (!cmbInput->count())
			{
				populateNewInputList();
			}
		}
		break;
		case Track::WAVE:
		{
			hideMidiElements();
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			hideMidiElements();
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
				chkInput->setChecked(true);
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			hideMidiElements();
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
				chkInput->setChecked(true);
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			hideMidiElements();
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
				chkInput->setChecked(true);
			}
		}
		break;
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			hideMidiElements();
			setMaximumHeight(100);
			resize(width(), 100);
			cmbInput->setVisible(false);
			lblInput->setVisible(false);
			txtInChannel->setVisible(false);
			chkInput->setVisible(false);
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
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if((md->openFlags() & 1))
					cmbOutput->addItem(md->name(), i);

				//Add global menu toggle
				//int ch1bit = 1 << 0;
				//Route myRoute(i, ch1bit);
			}

			if (!cmbOutput->count())
			{
				populateNewOutputList();
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
				chkOutput->setChecked(true);
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			importInputs();
			if (!cmbOutput->count())
			{
				chkOutput->setChecked(true);
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
				chkOutput->setChecked(true);
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
				chkOutput->setChecked(true);
			}
		}
		break;
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			cmbOutput->setVisible(false);
			lblOutput->setVisible(false);
			txtOutChannel->setVisible(false);
			chkOutput->setVisible(false);
		break;
	}
}/*}}}*/

void CreateTrackDialog::importOutputs()/*{{{*/
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbInput->addItem(*i);
		}
	}
}/*}}}*/

void CreateTrackDialog::importInputs()/*{{{*/
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->inputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbOutput->addItem(*i);
		}
	}
}/*}}}*/

void CreateTrackDialog::populateNewInputList()/*{{{*/
{
	while(cmbInput->count())
		cmbInput->removeItem(cmbInput->count()-1);
	m_createMidiInputDevice = true;
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
	while(cmbOutput->count())
		cmbOutput->removeItem(cmbOutput->count()-1);
	m_createMidiOutputDevice = true;
	if(audioDevice->deviceType() != AudioDevice::JACK_AUDIO)
		return;
	std::list<QString> sl = audioDevice->inputPorts(true, m_showJackAliases);
	for (std::list<QString>::iterator ip = sl.begin(); ip != sl.end(); ++ip)
	{
		cmbOutput->addItem(*ip, MidiDevice::JACK_MIDI);
	}
}/*}}}*/

void CreateTrackDialog::populateInstrumentList()
{
	for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)
	{
		SynthI* si = dynamic_cast<SynthI*> (*i);
		if (!si)
			cmbInstrument->addItem((*i)->iname());
	}
	//Default to the GM instrument
	int gm = cmbInstrument->findText("GM");
	if(gm >= 0)
		cmbInstrument->setCurrentIndex(gm);
}

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

void CreateTrackDialog::hideMidiElements()/*{{{*/
{
	txtInChannel->setVisible(false);
	btnNewInput->setVisible(false);
	txtOutChannel->setVisible(false);
	btnNewOutput->setVisible(false);
	lblInstrument->setVisible(false);
	cmbInstrument->setVisible(false);
	chkMonitor->setVisible(false);
	setMaximumHeight(150);
	resize(width(), 150);
}/*}}}*/

void CreateTrackDialog::showAllElements()/*{{{*/
{
	txtInChannel->setVisible(true);
	btnNewInput->setVisible(true);
	txtOutChannel->setVisible(true);
	btnNewOutput->setVisible(true);
	lblInstrument->setVisible(true);
	cmbInstrument->setVisible(true);
	chkMonitor->setVisible(true);
	cmbInput->setVisible(true);
	lblInput->setVisible(true);
	cmbOutput->setVisible(true);
	lblOutput->setVisible(true);
	setMaximumHeight(200);
	resize(width(), 200);
}/*}}}*/

void CreateTrackDialog::showEvent(QShowEvent*)
{
	qDebug("Inside CreateTrackDialog::showEvent trackType: %i, position: %i", m_insertType, m_insertPosition);
	//setTitleText();
	populateInputList();
	populateOutputList();
	populateInstrumentList();
}

