//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "CreateTrackDialog.h"
#include "track.h"
#include "song.h"
#include "app.h"
#include "mididev.h"
#include "midiport.h"
#include "audio.h"
#include "driver/jackaudio.h"


CreateTrackDialog::CreateTrackDialog(int type, int pos, QWidget* parent)
: QDialog(parent),
m_insertType(type),
m_insertPosition(pos)
{
	setupUi(this);
	//populateInputs();
	qWarning("CreateTrackDialog CTor end()");
}

//Add button slot
void CreateTrackDialog::addTrack()
{
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			//cmbInput->addItem(md->name(), i);
			//Add global menu toggle
			//int ch1bit = 1 << 0;
			//Route myRoute(i, ch1bit);
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
}

//Input combo slot
void CreateTrackDialog::inputSelected(int pos)
{
}

//Populate input combo based on type
void CreateTrackDialog::showEvent(QShowEvent*)
{
	qWarning("Inside CreateTrackDialog::showEvent trackType: %i, position: %i", m_insertType, m_insertPosition);
	while(cmbInput->count())
		cmbInput->removeItem(cmbInput->count()-1);
	Track::TrackType type = (Track::TrackType)m_insertType;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{
			qWarning("Midi track type");
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiPort* mp = &midiPorts[i];
				MidiDevice* md = mp->device();
				
				if (!md)
					continue;

				if ((md->rwFlags() & 2))
					cmbInput->addItem(md->name(), i);

				if((md->rwFlags() & 1))
					cmbOutput->addItem(md->name(), i);

				//Add global menu toggle
				//int ch1bit = 1 << 0;
				//Route myRoute(i, ch1bit);
			}

			if (!cmbInput->count())
			{//TODO: Maybe popup the midiport manager here to make them create an input
			}
		}
		break;
		case Track::WAVE:
		{
			qWarning("Audio Track Type");
			txtInChannel->setVisible(false);
			txtOutChannel->setVisible(false);

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
			importOutputs();
			if (!cmbInput->count())
			{//TODO: Not sure what we could do here except notify the user
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			for(iTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
			{
				if((*t)->isMidiTrack())
					continue;
				AudioTrack* track = (AudioTrack*) (*t);
				Route r(track, -1);
				cmbInput->addItem(r.name());
			}

			importInputs();
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
			importOutputs();
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
				else if(track->type() == Track::AUDIO_OUTPUT)
				{
					cmbOutput->addItem(Route(track, -1).name());
				}
			}
		}
		break;
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			cmbInput->setVisible(false);
			cmbOutput->setVisible(false);
			label_2->setVisible(false);
			label_3->setVisible(false);
			txtInChannel->setVisible(false);
			txtOutChannel->setVisible(false);
		break;
	}
}

void CreateTrackDialog::importOutputs()
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->outputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbInput->addItem(*i);
		}
	}
}

void CreateTrackDialog::importInputs()
{
	if (checkAudioDevice()) 
	{
		std::list<QString> sl = audioDevice->inputPorts();
		for (std::list<QString>::iterator i = sl.begin(); i != sl.end(); ++i) {
			cmbOutput->addItem(*i);
		}
	}
}
