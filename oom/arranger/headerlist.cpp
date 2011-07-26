//===============================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//===============================================================


#include <cmath>

#include <QtGui>

#include "popupmenu.h"
#include "globals.h"
#include "icons.h"
#include "headerlist.h"
#include "mididev.h"
#include "midiport.h"
#include "midiseq.h"
//#include "comment.h"
#include "track.h"
#include "song.h"
#include "node.h"
#include "audio.h"
//#include "instruments/minstrument.h"
#include "app.h"
#include "arranger.h"
#include "gconfig.h"
#include "event.h"
//#include "midiedit/drummap.h"
//#include "synth.h"
#include "config.h"
#include "menulist.h"
#include "midimonitor.h"
#include "pcanvas.h"
#include "trackheader.h"


HeaderList::HeaderList(QWidget* parent, const char* name)
: QFrame(parent) 
{
	setObjectName(name);
	setMouseTracking(true);
	setAcceptDrops(true);
	setFocusPolicy(Qt::NoFocus);
	
	wantCleanup = false;
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_layout = new QVBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setAlignment(Qt::AlignTop|Qt::AlignLeft);
	QSpacerItem* vSpacer = new QSpacerItem(20, 40, QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	m_layout->addItem(vSpacer);

	ypos = 0;
	setFocusPolicy(Qt::StrongFocus);

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
}

void HeaderList::songChanged(int flags)/*{{{*/
{
	if(wantCleanup && !m_dirtyheaders.isEmpty())
	{
		TrackHeader* item;
		while(!m_dirtyheaders.isEmpty() && (item = m_dirtyheaders.takeAt(0)) != 0)
		{
			if(item)
				delete item;
		}
		wantCleanup = false;
	}
	//if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_TRACK_INSERTED
	//		| SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_ROUTE | SC_CHANNELS | SC_MIDI_TRACK_PROP | SC_VIEW_CHANGED))
	if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_MIDI_TRACK_PROP | SC_SELECTION | SC_TRACK_MODIFIED))
	{
		emit updateHeader(flags);
	}
	if (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED /*| SC_TRACK_MODIFIED*/ | SC_VIEW_CHANGED))
	{
		updateTrackList();
	}
}/*}}}*/

void HeaderList::updateTrackList()/*{{{*/
{
	//printf("HeaderList::updateTrackList\n");
	TrackHeader* item;
	while(!m_headers.isEmpty() && (item = m_headers.takeAt(0)) != 0)
	{
		if(item)
		{
			item->hide();
			m_dirtyheaders.append(item);
		}
	}
	TrackList* l = song->visibletracks();
	m_headers.clear();
	int index = 0;
	for (iTrack i = l->begin(); i != l->end();++index, ++i)
	{
		Track* track = *i;
		//Track::TrackType type = track->type();
		//int trackHeight = track->height();
		TrackHeader* header = new TrackHeader(track, this);
		header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		header->setFixedHeight(track->height());
		connect(this, SIGNAL(updateHeader(int)), header, SLOT(songChanged(int)));
		connect(header, SIGNAL(selectionChanged(Track*)), SIGNAL(selectionChanged(Track*)));
		connect(header, SIGNAL(trackInserted(int)), SIGNAL(trackInserted(int)));
		m_headers.append(header);
		m_layout->insertWidget(index, header);
	}
	//Request a cleanup on the next song change, this should be frequent enough to 
	//keep things tidy, If it proves not to be we just switch to the heartBeat that is 
	//20ms guaranteed.
	wantCleanup = true;
	//printf("Leaving updateTrackList\n");
}/*}}}*/

void HeaderList::renameTrack(Track* t)/*{{{*/
{
	if (t && t->name() != "Master")
	{
	}
}/*}}}*/

Track* HeaderList::y2Track(int y) const/*{{{*/
{
	TrackList* l = song->visibletracks();
	int ty = 0;
	for (iTrack it = l->begin(); it != l->end(); ++it)
	{
		int h = (*it)->height();
		if (y >= ty && y < ty + h)
			return *it;
		ty += h;
	}
	return 0;
}/*}}}*/

void HeaderList::tracklistChanged()/*{{{*/
{
	updateTrackList();
}/*}}}*/

bool HeaderList::isEditing()/*{{{*/
{
	if (m_headers.isEmpty())
	{
		return false;
	}
	else
	{
		foreach(TrackHeader* h, m_headers)
		{
			if(h->isEditing())
				return true;
		}
	}
	return false;
}/*}}}*/

TrackList HeaderList::getRecEnabledTracks()/*{{{*/
{
	//printf("getRecEnabledTracks\n");
	TrackList recEnabled;
	//This changes to song->visibletracks()
	TrackList* tracks = song->visibletracks();
	for (iTrack t = tracks->begin(); t != tracks->end(); ++t)
	{
		if ((*t)->recordFlag())
		{
			//printf("rec enabled track\n");
			recEnabled.push_back(*t);
		}
	}
	return recEnabled;
}/*}}}*/

void HeaderList::updateSelection(Track* t, bool shift)/*{{{*/
{
	printf("HeaderList::updateSelection Before track check\n");
	if(t)
	{
		printf("HeaderList::updateSelection\n");
		if (!shift)
		{
			song->deselectTracks();
			if(song->hasSelectedParts)
				song->deselectAllParts();
			t->setSelected(true);

			// rec enable track if expected
			TrackList recd = getRecEnabledTracks();
			if (recd.size() == 1 && config.moveArmedCheckBox)
			{ // one rec enabled track, move rec enabled with selection
				song->setRecordFlag((Track*) recd.front(), false);
				song->setRecordFlag(t, true);
			}
		}
		else
		{
			song->deselectAllParts();
			t->setSelected(!t->selected());
		}
		emit selectionChanged(t->selected() ? t : 0);
		song->update(SC_SELECTION);
	}
}/*}}}*/

void HeaderList::selectTrack(Track* tr)/*{{{*/
{
	song->deselectTracks();
	tr->setSelected(true);


	// rec enable track if expected
	TrackList recd = getRecEnabledTracks();
	if (recd.size() == 1 && config.moveArmedCheckBox)
	{ // one rec enabled track, move rec enabled with selection
		song->setRecordFlag((Track*) recd.front(), false);
		song->setRecordFlag(tr, true);
	}

	song->update(SC_SELECTION);
	emit selectionChanged(tr);
}/*}}}*/

void HeaderList::moveSelection(int n)/*{{{*/
{
	//This changes to song->visibletracks()
	TrackList* tracks = song->visibletracks();

	// check for single selection
	TrackList selectedTracks = song->getSelectedTracks();
	int nselect = selectedTracks.size();

	// if there isn't a selection, select the first in the list
	// if there is any, else return, not tracks at all
	if (nselect == 0)
	{
		if (song->visibletracks()->size())
		{
			Track* track = (*(song->visibletracks()->begin()));
			track->setSelected(true);
			if(song->hasSelectedParts)
				song->deselectAllParts();
			emit selectionChanged(track);
		}
		return;
	}

	if (nselect != 1)
	{
		song->deselectTracks();
		if (n == 1)
		{
			Track* bottomMostSelected = *(--selectedTracks.end());
			bottomMostSelected->setSelected(true);
			if(song->hasSelectedParts)
				song->deselectAllParts();
			emit selectionChanged(bottomMostSelected);
		}
		else if (n == -1)
		{
			Track* topMostSelected = *(selectedTracks.begin());
			topMostSelected->setSelected(true);
			if(song->hasSelectedParts)
				song->deselectAllParts();
			emit selectionChanged(topMostSelected);
		}
		else
		{
			return;
		}
	}

	Track* selTrack = 0;
	for (iTrack t = tracks->begin(); t != tracks->end(); ++t)
	{
		iTrack s = t;
		if ((*t)->selected())
		{
			if (n > 0)
			{
				while (n--)
				{
					++t;
					if (t == tracks->end())
					{
						--t;
						break;
					}
				}
			}
			else
			{
				while (n++ != 0)
				{
					if (t == tracks->begin())
						break;
					--t;
				}
			}
			(*s)->setSelected(false);
			(*t)->setSelected(true);
			selTrack = *t;

			// rec enable track if expected
			TrackList recd = getRecEnabledTracks();
			if (recd.size() == 1 && config.moveArmedCheckBox)
			{ // one rec enabled track, move rec enabled with selection
				song->setRecordFlag((Track*) recd.front(), false);
				song->setRecordFlag((*t), true);
			}

			break;
		}
	}
	if(song->hasSelectedParts)
		song->deselectAllParts();
	emit selectionChanged(selTrack);
}/*}}}*/

void HeaderList::selectTrackAbove()/*{{{*/
{
	moveSelection(-1);
}/*}}}*/

void HeaderList::selectTrackBelow()/*{{{*/
{
	moveSelection(1);
}/*}}}*/

void HeaderList::moveSelectedTrack(int dir)/*{{{*/
{
	TrackList tl = song->getSelectedTracks();
	if(tl.size() == 1)
	{
		Track* src = (Track*)tl.front();
		if(src)
		{
			int i = song->visibletracks()->index(src);
			ciTrack it = song->visibletracks()->index2iterator(i);
			Track* t = 0;
			if(dir == 1)
			{
				if(it != song->visibletracks()->begin())//already at top
				{
					ciTrack d = --it;
					t = *(d);
				}
				if (t)
				{
					int dTrack = song->visibletracks()->index(t);
					audio->msgMoveTrack(i, dTrack);
					//The selection event should be harmless enough to call here to update 
					oom->arranger->verticalScrollSetYpos(oom->arranger->getCanvas()->track2Y(src));
				}
			}
			else
			{
				if(it != song->visibletracks()->end())//already at bottom
				{
					ciTrack d = ++it;
					t = *(d);
				}
				if (t)
				{
					int dTrack = song->visibletracks()->index(t);
					audio->msgMoveTrack(i, dTrack);
					//The selection event should be harmless enough to call here to update 
					oom->arranger->verticalScrollSetYpos(oom->arranger->getCanvas()->track2Y(t));
				}
			}
		}
	}
}/*}}}*/

void HeaderList::dragEnterEvent(QDragEnterEvent *event)/*{{{*/
{
	if (event->mimeData()->hasFormat("oomidi/x-trackinfo"))
	{
		if (children().contains(event->source()))
		{
		    event->setDropAction(Qt::MoveAction);
		    event->accept();
		}
		else
		{
		    event->ignore();
		}
	}
	else
	{
	    event->ignore();
	}
}/*}}}*/

void HeaderList::dragMoveEvent(QDragMoveEvent *event)/*{{{*/
{
	if (event->mimeData()->hasFormat("oomidi/x-trackinfo"))
	{
		if (children().contains(event->source()))
		{
		    event->setDropAction(Qt::MoveAction);
		    event->accept();
		}
		else
		{
		    event->ignore();
		}
	}
	else
	{
	    event->ignore();
	}
}/*}}}*/

void HeaderList::dropEvent(QDropEvent *event)/*{{{*/
{
	if (event->mimeData()->hasFormat("oomidi/x-trackinfo"))
	{
		const QMimeData *mime = event->mimeData();
		QByteArray itemData = mime->data("oomidi/x-trackinfo");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);
		
		QString trackName;
		int index;
		QPoint offset;
		dataStream >> trackName >> index >> offset;
		Track* srcTrack = song->findTrack(trackName);
		Track* t = y2Track(event->pos().y() + ypos);
		if (srcTrack && t)
		{
			int sTrack = song->visibletracks()->index(srcTrack);
			int dTrack = song->visibletracks()->index(t);
			//We are about to repopulate the whole list so lets disconnect all the signals here
			//This will prevent race condition crashing from event firing when widget is dying
			if(!m_headers.isEmpty() && index < m_headers.size())
			{
				foreach(TrackHeader* head, m_headers)
				{
					head->stopProcessing();
					disconnect(this, SIGNAL(updateHeader(int)), head, SLOT(songChanged(int)));
				}
			}
			audio->msgMoveTrack(sTrack, dTrack);
			updateTrackList();
		}
		
		if (event->source() != this)
		{
		    event->setDropAction(Qt::MoveAction);
		    event->accept();
		}
		else
		{
		    event->acceptProposedAction();
		}
	}
	else
	{
		event->ignore();
	}
}/*}}}*/

void HeaderList::keyPressEvent(QKeyEvent* e)/*{{{*/
{
	if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
	{
		if(isEditing())
		{
			printf("HeaderList::keyPressEvent editing\n");
			return;
		}
	}
	emit keyPressExt(e); //redirect keypress events to main app
}/*}}}*/

void HeaderList::mousePressEvent(QMouseEvent* ev) //{{{
{
	int button = ev->button();

	Track* t = 0;
	if (button == Qt::RightButton)
	{
		QMenu* p = new QMenu;
		QAction* midi = p->addAction(*addtrack_addmiditrackIcon, tr("Add Midi Track"));
		midi->setData(Track::MIDI);
		QAction* wave = p->addAction(*addtrack_wavetrackIcon, tr("Add Audio Track"));
		wave->setData(Track::WAVE);
		QAction* aoutput = p->addAction(*addtrack_audiooutputIcon, tr("Add Output"));
		aoutput->setData(Track::AUDIO_OUTPUT);
		QAction* agroup = p->addAction(*addtrack_audiogroupIcon, tr("Add Buss"));
		agroup->setData(Track::AUDIO_BUSS);
		QAction* ainput = p->addAction(*addtrack_audioinputIcon, tr("Add Input"));
		ainput->setData(Track::AUDIO_INPUT);
		QAction* aaux = p->addAction(*addtrack_auxsendIcon, tr("Add Aux Send"));
		aaux->setData(Track::AUDIO_AUX);

		// Show the menu
		QAction* act = p->exec(ev->globalPos(), 0);

		// Valid click?
		if (act)
		{
			int n = act->data().toInt();
			// Valid item?
			if ((n >= 0) && ((Track::TrackType)n != Track::AUDIO_SOFTSYNTH))
			{
				// Synth sub-menu id?
				if (n >= MENU_ADD_SYNTH_ID_BASE)
				{
					/*n -= MENU_ADD_SYNTH_ID_BASE;
					//if(n < synthis.size())
					//  t = song->createSynthI(synthis[n]->baseName());
					//if((n - MENU_ADD_SYNTH_ID_BASE) < (int)synthis.size())
					if (n < (int) synthis.size())
					{
						//t = song->createSynthI(synp->text(n));
						//t = song->createSynthI(synthis[n]->name());
						t = song->createSynthI(synthis[n]->baseName(), synthis[n]->name());

						if (t)
						{
							// Add instance last in midi device list.
							for (int i = 0; i < MIDI_PORTS; ++i)
							{
								MidiPort* port = &midiPorts[i];
								MidiDevice* dev = port->device();
								if (dev == 0)
								{
									midiSeq->msgSetMidiDevice(port, (SynthI*) t);
									oom->changeConfig(true); // save configuration file
									song->update();
									break;
								}
							}
						}
					}*/
				}
				else
				{
					t = song->addTrack((Track::TrackType)n);
				}

				if (t)
				{
					midiMonitor->msgAddMonitoredTrack(t);
					song->deselectTracks();
					t->setSelected(true);

					emit selectionChanged(t);
					emit trackInserted(n);
					song->updateTrackViews1();
				}
			}
		}
		if(p)
			delete p;
	}

}/*}}}*/

void HeaderList::wheelEvent(QWheelEvent* ev)/*{{{*/
{
	emit redirectWheelEvent(ev);
	QFrame::wheelEvent(ev);
	return;
}/*}}}*/

/*void HeaderList::resizeEvent(QResizeEvent*)
{
}*/


