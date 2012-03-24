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
#include "HeaderList.h"
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
#include "Composer.h"
#include "gconfig.h"
#include "event.h"
//#include "midiedit/drummap.h"
#include "config.h"
#include "menulist.h"
#include "midimonitor.h"
#include "ComposerCanvas.h"
#include "trackheader.h"
#include "CreateTrackDialog.h"


HeaderList::HeaderList(QWidget* parent, const char* name)
: QFrame(parent) 
{
	setObjectName(name);
	setMouseTracking(true);
	setAcceptDrops(true);
	setFocusPolicy(Qt::NoFocus);
	
	wantCleanup = false;
	m_lockupdate = false;
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_layout = new QVBoxLayout(this);
	m_layout->setSpacing(0);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setAlignment(Qt::AlignTop|Qt::AlignLeft);
	QSpacerItem* vSpacer = new QSpacerItem(20, 440, QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
	m_layout->addItem(vSpacer);

	ypos = 0;
	setFocusPolicy(Qt::StrongFocus);

	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	connect(song, SIGNAL(composerViewChanged()), SLOT(composerViewChanged()));
}

void HeaderList::composerViewChanged()
{
	updateTrackList(true);
}

void HeaderList::songChanged(int flags)/*{{{*/
{
	if(m_lockupdate)
		return;
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
	if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_SELECTION | SC_TRACK_MODIFIED | SC_CHANNELS))
	{
		/*if(!song->invalid && !m_headers.isEmpty())
		{
			foreach(TrackHeader* header, m_headers)
			{
				header->songChanged(flags);
			}
		}*/
		emit updateHeader(flags);
	}
	/*if (!song->invalid && (flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_VIEW_CHANGED )))
	{
		updateTrackList(true);
		return;
	}*/
	//if (flags & (SC_MUTE | SC_SOLO | SC_RECFLAG | SC_TRACK_INSERTED
	//		| SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_ROUTE | SC_CHANNELS | SC_MIDI_TRACK_PROP | SC_VIEW_CHANGED))
}/*}}}*/

void HeaderList::updateTrackList(bool viewupdate)/*{{{*/
{
	if(m_lockupdate)
		return;
	m_lockupdate = true;
	if(debugMsg)
		printf("HeaderList::updateTrackList\n");
	TrackList* l = song->visibletracks();
	//Attempt to prevent widget destruction recreation cycle and just set 
	//the track on the header instead. I will also try to never recreate 
	//the list at all unless the size changes and when that happens just add the 
	//new tracks and update the old headers with the new track of thier own.
	int lsize = l->size();
	if(viewupdate && !m_headers.isEmpty() && lsize == m_headers.size())
	{
		//printf("Using optimized update\n");
		iTrack i = l->begin();
		foreach(TrackHeader* header, m_headers)
		{
			//if((*i) != header->track())
			//{
				//Set the new track on the header
				header->setTrack((*i));
				//header->setSelected(false);
			/*}
			else
			{
				//Just let the header update itself as the track has not changed
				header->songChanged(-1);
			}*/
			++i;
		}
		emit updateHeader(-1);
	}
	else if(viewupdate && !m_headers.isEmpty() && lsize < m_headers.size())
	{
		//printf("Using optimized update\n");
		int remcount = m_headers.size() - lsize;
		for(int i = 0; i < remcount; ++i)
		{
			TrackHeader *h = m_headers.takeLast();
			h->stopProcessing();
			h->hide();
			m_dirtyheaders.append(h);
		}
		int hcount = 0;
		for(iTrack i = l->begin(); i != l->end(); ++hcount, ++i)
		{
			m_headers.at(hcount)->setTrack((*i));
		}
		wantCleanup = true;
		update();
		emit updateHeader(-1);
	}
	else if(viewupdate && !m_headers.isEmpty() && lsize > m_headers.size())
	{
		//printf("Using optimized update\n");
		int hcount = m_headers.size();
		int addcount = 0;
		for(iTrack i = l->begin(); i != l->end(); ++addcount, ++i)
		{
			Track* track = (Track*)(*i);
			if(addcount < hcount)
			{
				m_headers.at(addcount)->setTrack(track);
			}
			else 
			{
				if(track)
				{
					TrackHeader* header = new TrackHeader(track, this);
					header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
					//header->setFixedHeight(track->height());
					connect(this, SIGNAL(updateHeader(int)), header, SLOT(songChanged(int)));
					connect(header, SIGNAL(selectionChanged(Track*)), SIGNAL(selectionChanged(Track*)));
					connect(header, SIGNAL(trackInserted(int)), SIGNAL(trackInserted(int)));
					connect(header, SIGNAL(trackHeightChanged()), SIGNAL(trackHeightChanged()));
					m_layout->insertWidget(m_headers.size(), header);
					m_headers.append(header);
				}
			}
		}
		emit updateHeader(-1);
	}
	else
	{
		TrackHeader* item;
		while(!m_headers.isEmpty() && (item = m_headers.takeAt(0)) != 0)
		{
			if(item)
			{
				item->stopProcessing();
				item->hide();
				m_dirtyheaders.append(item);
			}
		}
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
			connect(header, SIGNAL(trackHeightChanged()), SIGNAL(trackHeightChanged()));
			m_headers.append(header);
			m_layout->insertWidget(index, header);
		}
		//Request a cleanup on the next song change, this should be frequent enough to 
		//keep things tidy, If it proves not to be we just switch to the heartBeat that is 
		//20ms guaranteed.
		wantCleanup = true;
	}
	m_lockupdate = false;
	//printf("Leaving updateTrackList\n");
}/*}}}*/

void HeaderList::clear()/*{{{*/
{
	TrackHeader* item;
	while(!m_headers.isEmpty() && (item = m_headers.takeAt(0)) != 0)
	{
		if(item)
		{
			item->stopProcessing();
			item->hide();
			m_dirtyheaders.append(item);
		}
	}
	m_headers.clear();
	//Request a cleanup on the next song change, this should be frequent enough to 
	//keep things tidy, If it proves not to be we just switch to the heartBeat that is 
	//20ms guaranteed.
	wantCleanup = true;
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
	updateTrackList(true);
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
	if(debugMsg)
		printf("HeaderList::updateSelection Before track check\n");
	if(t)
	{
		if(debugMsg)
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
			song->update(SC_SELECTION);
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
			song->update(SC_SELECTION);
		}
		else if (n == -1)
		{
			Track* topMostSelected = *(selectedTracks.begin());
			topMostSelected->setSelected(true);
			if(song->hasSelectedParts)
				song->deselectAllParts();
			emit selectionChanged(topMostSelected);
			song->update(SC_SELECTION);
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
	song->update(SC_SELECTION);
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
			if(src->id() == song->masterId())
				return;
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
					if(t->id() == song->masterId())
						return;
					int dTrack = song->visibletracks()->index(t);
					audio->msgMoveTrack(i, dTrack);
					//The selection event should be harmless enough to call here to update 
					oom->composer->verticalScrollSetYpos(oom->composer->getCanvas()->track2Y(src));
				}
				else
					return;
			}
			else
			{
				if(it != song->visibletracks()->end())//already at bottom
				{
					ciTrack d = ++it;
					if(d == song->visibletracks()->end())//already at bottom
						return;
					t = *(d);
				}
				if (t)
				{
					if(t->id() == song->masterId())
						return;
					int dTrack = song->visibletracks()->index(t);
					audio->msgMoveTrack(i, dTrack);
					//The selection event should be harmless enough to call here to update 
					oom->composer->verticalScrollSetYpos(oom->composer->getCanvas()->track2Y(t));
				}
				else
					return;
			}
			updateTrackList(true);
		}
		//song->update(SC_TRACK_MODIFIED);
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
	else if(event->mimeData()->hasUrls())
	{
		if(!children().contains(event->source()))
			event->accept();
		else
	    	event->ignore();
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
	else if(event->mimeData()->hasUrls())
	{
		if(!children().contains(event->source()))
			event->accept();
		else
	    	event->ignore();
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
		if (srcTrack && t && t->name() != "Master")
		{
			int sTrack = song->visibletracks()->index(srcTrack);
			int dTrack = song->visibletracks()->index(t);
			//We are about to repopulate the whole list so lets disconnect all the signals here
			//This will prevent race condition crashing from event firing when widget is dying
			/*if(!m_headers.isEmpty() && index < m_headers.size())
			{
				foreach(TrackHeader* head, m_headers)
				{
					head->stopProcessing();
					//disconnect(this, SIGNAL(updateHeader(int)), head, SLOT(songChanged(int)));
				}
			}*/
			audio->msgMoveTrack(sTrack, dTrack);
			//song->update(SC_TRACK_INSERTED);
			updateTrackList(true);
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
	else if(event->mimeData()->hasUrls())
	{
		if(!children().contains(event->source()))
		{
			// Multiple urls not supported here. Grab the first one.
			QString text = event->mimeData()->urls()[0].path();
		    event->acceptProposedAction();

			if (text.endsWith(".wav", Qt::CaseInsensitive) ||
					text.endsWith(".ogg", Qt::CaseInsensitive) ||
					text.endsWith(".mpt", Qt::CaseInsensitive))
			{
				Track* track = y2Track(event->pos().y() + ypos);
				if (!track)
				{//Create the track
					Track::TrackType t = Track::MIDI;
					if(text.endsWith(".wav", Qt::CaseInsensitive) || text.endsWith(".ogg", Qt::CaseInsensitive))
					{
						t = Track::WAVE;
						QFileInfo f(text);
						track = song->addTrackByName(f.baseName(), Track::WAVE, -1, true, true);
						song->updateTrackViews();
					}
					else
					{
						VirtualTrack* vt;
						CreateTrackDialog *ctdialog = new CreateTrackDialog(&vt, t, -1, this);
						ctdialog->lockType(true);
						if(ctdialog->exec() && vt)
						{
							qint64 nid = trackManager->addTrack(vt, -1);
							track = song->findTrackById(nid);
						}
					}
				}
				
				if (track)
				{
					if (track->type() == Track::WAVE &&
							(text.endsWith(".wav", Qt::CaseInsensitive) ||
							(text.endsWith(".ogg", Qt::CaseInsensitive))))
					{
						oom->importWaveToTrack(text, song->cpos(), track);
					}
					else if ((track->isMidiTrack() || track->type() == Track::WAVE) && text.endsWith(".mpt", Qt::CaseInsensitive))
					{//Who saves a wave part as anything but a wave file?
						oom->importPartToTrack(text, song->cpos(), track);
					}
				}
			}
			else if(text.endsWith(".mid", Qt::CaseInsensitive) ||
				text.endsWith(".kar", Qt::CaseInsensitive))
			{
				oom->importMidi(text);
			}
		}
		else
	    	event->ignore();
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
			//printf("HeaderList::keyPressEvent editing\n");
			return;
		}
	}
	//printf("HeaderList::keyPressEvent emitting signal keyPressExt(%p)\n", e);
	emit keyPressExt(e); //redirect keypress events to main app
}/*}}}*/

void HeaderList::mousePressEvent(QMouseEvent* ev) //{{{
{
	int button = ev->button();

	//Track* t = 0;
	if (button == Qt::RightButton)
	{
		QMenu* p = new QMenu;
		QAction* midi = p->addAction(*addMidiIcon, tr("Add Midi Track"));
		midi->setData(Track::MIDI);
		QAction* wave = p->addAction(*addAudioIcon, tr("Add Audio Track"));
		wave->setData(Track::WAVE);
		QAction* aoutput = p->addAction(*addOutputIcon, tr("Add Output"));
		aoutput->setData(Track::AUDIO_OUTPUT);
		QAction* agroup = p->addAction(*addBussIcon, tr("Add Buss"));
		agroup->setData(Track::AUDIO_BUSS);
		QAction* ainput = p->addAction(*addInputIcon, tr("Add Input"));
		ainput->setData(Track::AUDIO_INPUT);
		QAction* aaux = p->addAction(*addAuxIcon, tr("Add Aux Send"));
		aaux->setData(Track::AUDIO_AUX);

		// Show the menu
		QAction* act = p->exec(ev->globalPos(), 0);

		// Valid click?
		if (act)
		{
			int n = act->data().toInt();
			// Valid item?
            if (n >= 0 && ((Track::TrackType)n != Track::AUDIO_SOFTSYNTH))
			{
				CreateTrackDialog *ctdialog = new CreateTrackDialog(n, -1, this);
				connect(ctdialog, SIGNAL(trackAdded(qint64)), this, SLOT(newTrackAdded(qint64)));
				ctdialog->exec();
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

void HeaderList::newTrackAdded(qint64 id)
{
	Track* t = song->findTrackById(id);
	if(t)
	{
		emit selectionChanged(t);
		emit trackInserted(t->type());
		song->updateTrackViews();
	}
}


