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

#include "app.h"
#include "icons.h"
#include "config.h"
#include "globals.h"
#include "song.h"
#include "shortcuts.h"

#include "astrip.h"
#include "mstrip.h"
#include "apconfig.h"
#include "track.h"

#include "gconfig.h"
#include "xml.h"
#include "traverso_shared/TConfig.h"
#include "mixerdock.h"

MixerDock::MixerDock(QWidget* parent)
: QFrame(parent)
{
	routingDialog = 0;
	oldAuxsSize = 0;
	masterStrip = 0;
	m_mode = DOCKED;
	m_tracklist = song->visibletracks();

	loading = true;
	layoutUi();
	loading = false;
}

MixerDock::MixerDock(MixerMode mode, QWidget* parent)
: QFrame(parent)
{
	routingDialog = 0;
	oldAuxsSize = 0;
	masterStrip = 0;
	if(mode == DOCKED)
		m_tracklist = song->visibletracks();
	else
		m_tracklist = new TrackList();
	
	m_mode = mode;
	loading = true;
	layoutUi();
	loading = false;
}

MixerDock::~MixerDock()
{
}

void MixerDock::layoutUi()/*{{{*/
{
	setObjectName("MixerDock");
	setMinimumHeight(300);
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	m_mixerBox = new QHBoxLayout(this);
	m_mixerBox->setContentsMargins(0, 0, 0, 0);
	m_mixerBox->setSpacing(0);
	m_mixerBox->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

	//setFeatures(QDockWidget::DockWidgetVerticalTitleBar|QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetClosable);
	QFrame* titleWidget = new QFrame(this);
	titleWidget->setObjectName("mixerDockTitle");
	titleWidget->setFrameShape(QFrame::StyledPanel);
	titleWidget->setFrameShadow(QFrame::Raised);
	//titleWidget->setMaximumWidth(250);
	m_adminBox = new QVBoxLayout(titleWidget);
	m_adminBox->setContentsMargins(2, 2, 2, 0);
	m_adminBox->setSpacing(0);
	m_adminBox->setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	
	//m_adminBox->addItem(new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum));

	m_btnAux = new QPushButton(titleWidget);
	m_btnAux->setToolTip(tr("Show/hide Effects Rack"));
	if(m_mode == DOCKED)
		m_btnAux->setShortcut(shortcuts[SHRT_TOGGLE_RACK].key);
	m_btnAux->setMaximumSize(QSize(22,18));
	m_btnAux->setObjectName("m_btnAux");
	m_btnAux->setCheckable(true);
	m_btnAux->setChecked(true);
	m_btnAux->setIcon(*expandIcon);
	m_btnAux->setIconSize(record_on_Icon->size());
	m_adminBox->addWidget(m_btnAux);

	m_mixerBox->addWidget(titleWidget);

	view = new QScrollArea(this);
	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	central = new QFrame(view);
	central->setObjectName("MixerCenter");
	central->setFrameShape(QFrame::StyledPanel);
	central->setFrameShadow(QFrame::Raised);
	layout = new QHBoxLayout();
	central->setLayout(layout);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
	view->setWidget(central);
	view->setWidgetResizable(true);
	m_mixerBox->addWidget(view);
	if(m_mode == DOCKED || m_mode == MASTER)
	{
		m_masterBox = new QHBoxLayout();
		m_masterBox->setContentsMargins(4, 0, 0, 0);
		m_masterBox->setSpacing(0);
		Track* master = song->findTrack("Master");
		if(master)
		{
			masterStrip = new AudioStrip(this, (AudioTrack*)master);
			masterStrip->setObjectName("MixerAudioOutStrip");
			m_masterBox->addWidget(masterStrip);
		}
		m_mixerBox->addLayout(m_masterBox);
	}

	//Push all the widgets in the m_adminBox to the top
	m_adminBox->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

	connect(m_btnAux, SIGNAL(toggled(bool)), SLOT(toggleAuxRack(bool)));
	//connect(m_btnDock, SIGNAL(clicked(bool)), SLOT(toggleDetach()));
	if(m_mode == DOCKED)
		connect(oom, SIGNAL(configChanged()), SLOT(configChanged()));
	//	connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(updateConnections(bool)));
	songChanged(-1);
}/*}}}*/

TrackList* MixerDock::tracklist()
{
	return m_tracklist;
}

void MixerDock::setTracklist(TrackList* list)
{
	m_tracklist = list;
	songChanged(-1);
}

void MixerDock::updateConnections(bool visible)
{
	if(visible)
	{
		if(!song->invalid)
			songChanged(-1);
		connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	}
	else
	{
		if(song && !song->invalid)
			disconnect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	}
}

void MixerDock::toggleAuxRack(bool toggle)/*{{{*/
{
	StripList::iterator si = stripList.begin();
	for (; si != stripList.end(); ++si)
	{
		Strip* audioStrip = (*si);
		if (audioStrip)
		{
			audioStrip->toggleAuxPanel(toggle);
		}

	}
	if((m_mode == DOCKED || m_mode == MASTER )&& masterStrip)
	{
		masterStrip->toggleAuxPanel(toggle);
	}
	//Just in case this was called from outside the button
	m_btnAux->blockSignals(true);
	m_btnAux->setChecked(toggle);
	m_btnAux->blockSignals(false);
}/*}}}*/

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

void MixerDock::updateMixer(UpdateAction action)/*{{{*/
{
	loading = true;
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

		TrackList* mtl = m_tracklist;
		for (iTrack i = mtl->begin(); i != mtl->end(); ++i)
		{
			Track* mt = *i;
			if ((mt->type() == Track::MIDI) || (mt->type() == Track::DRUM))
				addStrip(*i, idx++);
		}

		return;
	}

	int idx = 0;

	TrackList* itl = m_tracklist;
	for (iTrack i = itl->begin(); i != itl->end(); ++i)
	{
		if((*i)->name() != "Master")
			addStrip(*i, idx++);
	}
	Track* master = song->findTrack("Master");
	if(master)
	{
		if((m_mode == DOCKED || m_mode == MASTER) && !masterStrip)
		{
			masterStrip = new AudioStrip(this, (AudioTrack*)master);
			masterStrip->setObjectName("MixerAudioOutStrip");
			m_masterBox->addWidget(masterStrip);
		}
		else if(m_mode == DOCKED || m_mode == MASTER)
		{
			masterStrip->setTrack(master);
		}
	}
	loading= false;
}/*}}}*/

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

void MixerDock::songChanged(int flags)/*{{{*/
{
	//printf("MixerDock::songChanged(%d)\n",flags);
	// Is it simply a midi controller value adjustment? Forget it.
	if (flags == SC_MIDI_CONTROLLER)
		return;

	UpdateAction action = NO_UPDATE;
	if (flags == -1)
	{
		//printf("MixerDock::songChanged -1 fired\n");
		action = UPDATE_ALL;
	}
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
	if (action != UPDATE_ALL && !loading)
	{
		//printf("Running songChanged() on all strips\n");
		StripList::iterator si = stripList.begin();
		for (; si != stripList.end(); ++si)
		{
			(*si)->songChanged(flags);
		}
		if((m_mode == DOCKED || m_mode == MASTER )&& masterStrip)
		{
			masterStrip->songChanged(flags);
		}
	}
}/*}}}*/
