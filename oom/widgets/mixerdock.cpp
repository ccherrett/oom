//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <list>
#include <cmath>

#include <QApplication>
#include <QDockWidget>
#include <QtGui>
/*#include <QMenuBar>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QShowEvent>
#include <QMenu>
#include <QActionGroup>
#include <QAction>*/

#include "app.h"
#include "icons.h"
#include "amixer.h"
#include "song.h"
#include "shortcuts.h"

#include "astrip.h"
#include "mstrip.h"
#include "apconfig.h"

#include "gconfig.h"
#include "xml.h"
#include "traverso_shared/TConfig.h"
#include "mixerdock.h"

MixerDock::MixerDock(const QString& title, QWidget* parent)
: QDockWidget(title, parent)
{
	routingDialog = 0;
	oldAuxsSize = 0;

	view = new QScrollArea(this);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setLayout(new QVBoxLayout());

	central = new QWidget(view);
	central->setObjectName("MixerCenter");
	//lbox = new QHBoxLayout();
	layout = new QHBoxLayout();
	central->setLayout(layout);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	//lbox->addLayout(layout);
	layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
	view->setWidget(central);
	view->setWidgetResizable(true);
	setWidget(view);

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(oom, SIGNAL(configChanged()), SLOT(configChanged()));
	song->update(); // calls update mixer
}

MixerDock::~MixerDock()
{
}

void MixerDock::addStrip(Track* t, int idx)/*{{{*/
{
	StripList::iterator si = stripList.begin();
	for (int i = 0; i < idx; ++i)
	{
		if (si != stripList.end())
			++si;
	}
	if (si != stripList.end() && (*si)->getTrack() == t)
		return;

	std::list<Strip*>::iterator nsi = si;
	++nsi;
	if (si != stripList.end()
			&& nsi != stripList.end()
			&& (*nsi)->getTrack() == t)
	{
		layout->removeWidget(*si);
		delete *si;
		stripList.erase(si);
	}
	else
	{
		Strip* strip;
		if (t->isMidiTrack())
		{
			strip = new MidiStrip(central, (MidiTrack*) t);
		}
		else
		{
			strip = new AudioStrip(central, (AudioTrack*) t);
		}
		switch (t->type())
		{/*{{{*/
			case Track::AUDIO_OUTPUT:
				strip->setObjectName("MixerAudioOutStrip");
				break;
			case Track::AUDIO_BUSS:
				strip->setObjectName("MixerAudioBussStrip");
				break;
			case Track::AUDIO_AUX:
				strip->setObjectName("MixerAuxStrip");
				break;
			case Track::WAVE:
				strip->setObjectName("MixerWaveStrip");
				break;
			case Track::AUDIO_INPUT:
				strip->setObjectName("MixerAudioInStrip");
				break;
			case Track::AUDIO_SOFTSYNTH:
				strip->setObjectName("MixerSynthStrip");
				break;
			case Track::MIDI:
			{
				strip->setObjectName("MidiTrackStrip");
				break;
			}
			case Track::DRUM:
			{
				strip->setObjectName("MidiDrumTrackStrip");
				break;
			}
				break;
		}/*}}}*/
		layout->insertWidget(idx, strip);
		stripList.insert(si, strip);
		strip->show();
	}
}/*}}}*/

void MixerDock::clear()/*{{{*/
{
	StripList::iterator si = stripList.begin();
	for (; si != stripList.end(); ++si)
	{
		layout->removeWidget(*si);
		delete *si;
	}
	stripList.clear();
	oldAuxsSize = -1;
}/*}}}*/

void MixerDock::updateMixer(UpdateAction action)
{

	int auxsSize = song->auxs()->size();
	if ((action == UPDATE_ALL) || (auxsSize != oldAuxsSize))
	{
		clear();
		oldAuxsSize = auxsSize;
	}
	else if (action == STRIP_REMOVED)
	{
		StripList::iterator si = stripList.begin();
		for (; si != stripList.end();)
		{
			Track* track = (*si)->getTrack();
			TrackList* tl = song->tracks();
			iTrack it;
			for (it = tl->begin(); it != tl->end(); ++it)
			{
				if (*it == track)
					break;
			}
			StripList::iterator ssi = si;
			++si;
			if (it != tl->end())
				continue;
			layout->removeWidget(*ssi);
			delete *ssi;
			stripList.erase(ssi);
		}
		return;
	}
	else if (action == UPDATE_MIDI)
	{
		int i = 0;
		int idx = -1;
		StripList::iterator si = stripList.begin();
		for (; si != stripList.end(); ++i)
		{
			Track* track = (*si)->getTrack();
			if (!track->isMidiTrack())
			{
				++si;
				continue;
			}

			if (idx == -1)
				idx = i;

			StripList::iterator ssi = si;
			++si;
			layout->removeWidget(*ssi);
			delete *ssi;
			stripList.erase(ssi);
		}

		if (idx == -1)
			idx = 0;

		//---------------------------------------------------
		//  generate Midi channel/port Strips
		//---------------------------------------------------

		TrackList* mtl = song->visibletracks();
		for (iTrack i = mtl->begin(); i != mtl->end(); ++i)
		{
			Track* mt = *i;
			if ((mt->type() == Track::MIDI) || (mt->type() == Track::DRUM))
				addStrip(*i, idx++);
		}

		return;
	}

	int idx = 0;

	TrackList* itl = song->visibletracks();
	for (iTrack i = itl->begin(); i != itl->end(); ++i)
		addStrip(*i, idx++);

}

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void MixerDock::configChanged()
{
	songChanged(SC_CONFIG);
}

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MixerDock::songChanged(int flags)
{
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;

	UpdateAction action = NO_UPDATE;
	if (flags == -1)
		action = UPDATE_ALL;
	else if(flags & SC_VIEW_CHANGED)
	{
		//printf("SC_VIEW_CHANGED fired\n");
		action = UPDATE_ALL;
	}
	else if (flags & SC_TRACK_REMOVED)
		action = STRIP_REMOVED;
	else if (flags & SC_TRACK_INSERTED)
		action = STRIP_INSERTED;
	else if (flags & SC_MIDI_TRACK_PROP)
		action = UPDATE_MIDI;
	if (action != NO_UPDATE)
	{
		//printf("running updateMixer()\n");
		updateMixer(action);
	}
	if (action != UPDATE_ALL)
	{
		//printf("Running songChanged() on all strips\n");
		StripList::iterator si = stripList.begin();
		for (; si != stripList.end(); ++si)
		{
			(*si)->songChanged(flags);
		}
	}
}
