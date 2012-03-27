//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: song.cpp,v 1.59.2.52 2009/12/15 03:39:58 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2012 The Open Octave Project <info@openoctave.org>
//=========================================================

#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <QAction>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QSignalMapper>
#include <QTextStream>
#include <QUndoStack>

#include "app.h"
#include "driver/jackmidi.h"
#include "driver/alsamidi.h"
#include "song.h"
#include "track.h"
#include "undo.h"
#include "key.h"
#include "globals.h"
#include "event.h"
#include "drummap.h"
#include "marker/marker.h"
#include "audio.h"
#include "mididev.h"
#include "midiport.h"
#include "AudioMixer.h"
#include "midiseq.h"
#include "audiodev.h"
#include "gconfig.h"
#include "sync.h"
#include "midictrl.h"
#include "menutitleitem.h"
#include "midi.h"
///#include "sig.h"
#include "al/sig.h"
#include <sys/wait.h>
#include "trackview.h"
#include "mpevent.h"
#include "midimonitor.h"
#include "plugin.h"
#include "traverso_shared/OOMCommand.h"
#include "traverso_shared/TConfig.h"
#include "CreateTrackDialog.h"
//#include <omp.h>

extern void clearMidiTransforms();
extern void clearMidiInputTransforms();
Song* song;

/*
//---------------------------------------------------------
//   RoutingMenuItem
//---------------------------------------------------------

class RoutingMenuItem : public QCustomMenuItem 
{
	  Route route;
	  //virtual QSize sizeHint() { return QSize(80, h); }
	  virtual void paint(QPainter* p, const QColorGroup&, bool, bool, int x, int y, int w, int h)
	  {
		p->fillRect(x, y, w, h, QBrush(lightGray));
		p->drawText(x, y, w, h, AlignCenter, route.name());
	  }

   public:
	  RoutingMenuItem(const Route& r) : route(r) { }
};
 */

//---------------------------------------------------------
//   Song
//---------------------------------------------------------

Song::Song(QUndoStack* stack, const char* name)
: QObject(0)
{
	setObjectName(name);
	_composerRaster = 0; // Set to measure, the same as Composer intial value. Composer snap combo will set this.
	noteFifoSize = 0;
	noteFifoWindex = 0;
	noteFifoRindex = 0;
	undoList = new UndoList;
	redoList = new UndoList;
	m_undoStack = stack; //new QUndoStack(this);
	_markerList = new MarkerList;
	_globalPitchShift = 0;
	jackErrorBox = 0;
	viewselected = false;
	hasSelectedParts = false;
	invalid = false;
	_replay = false;
	_replayPos = 0;
	//Master track ID
	m_masterId = 0;
	m_oomVerbId = 0;
	//Create the AutoView
	TrackView* wv = new TrackView();
	wv->setViewName(tr("Working View"));
	wv->setSelected(false);
	m_autoTrackViewIndex.append(wv->id());
	_autotviews[wv->id()] = wv;
	m_workingViewId = wv->id();
	//TODO:Set working view ID for later use
	TrackView* iv = new TrackView();
	iv->setViewName(tr("Inputs  View"));
	iv->setSelected(false);
	_autotviews[iv->id()] = iv;
	m_autoTrackViewIndex.append(iv->id());
	m_inputViewId = iv->id();
	//TODO:Set view ID for later use
	TrackView* ov = new TrackView();
	ov->setViewName(tr("Outputs View"));
	ov->setSelected(false);
	_autotviews[ov->id()] = ov;
	m_autoTrackViewIndex.append(ov->id());
	m_outputViewId = ov->id();
	//TODO:Set view ID for later use
	TrackView* gv = new TrackView();
	gv->setViewName(tr("Buss View"));
	gv->setSelected(false);
	_autotviews[gv->id()] = gv;
	m_autoTrackViewIndex.append(gv->id());
	m_bussViewId = gv->id();
	//TODO:Set view ID for later use
	TrackView* av = new TrackView();
	av->setViewName(tr("Aux View"));
	av->setSelected(false);
	_autotviews[av->id()] = av;
	m_autoTrackViewIndex.append(av->id());
	m_auxViewId = av->id();
	//TODO:Set view ID for later use
	TrackView* cv = new TrackView();
	cv->setViewName(tr("Comment View"));
	cv->setSelected(false);
	_autotviews[cv->id()] = cv;
	m_autoTrackViewIndex.append(cv->id());
	m_commentViewId = cv->id();
	//TODO:Set view ID for later use
	QHash<int, QString> hash;

	QStringList map;
	map << "C-2" << "C#-2" << "D-2" << "D#-2" << "E-2" << "F-2" << "F#-2" << "G-2" << "G#-2" << "A-2" << "A#-2" << "B-2" << "C-1" ;
	map << "C#-1" << "D-1" << "D#-1" << "E-1" << "F-1" << "F#-1" << "G-1" << "G#-1" << "A-1" << "A#-1" << "B-1" << "C0" << "C#0" << "D0" << "D#0" << "E0" << "F0" << "F#0";
	map << "G0" << "G#0" << "A0" << "A#0" << "B0" << "C1" << "C#1" << "D1" << "D#1" << "E1" << "F1" << "F#1" << "G1" << "G#1" << "A1" << "A#1";
	map << "B1" << "C2" << "C#2" << "D2" << "D#2" << "E2" << "F2" << "F#2" << "G2" << "G#2" << "A2" << "A#2" << "B2" << "C3" << "C#3" << "D3";
	map << "D#3" << "E3" << "F3" << "F#3" << "G3" << "G#3" << "A3" << "A#3" << "B3" << "C4" << "C#4" << "D4" << "D#4" << "E4" << "F4" << "F#4";
	map << "G4" << "G#4" << "A4" << "A#4" << "B4" << "C5" << "C#5" << "D5" << "D#5" << "E5" << "F5" << "F#5" << "G5" << "G#5" << "A5" << "A#5";
	map << "B5" << "C6" << "C#6" << "D6" << "D#6" << "E6" << "F6" << "F#6" << "G6" << "G#6" << "A6" << "A#6" << "B6" << "C7" << "C#7" << "D7";
	map << "D#7" << "E7" << "F7" << "F#7" << "G7" << "G#7" << "A7" << "A#7" << "B7" << "C8" << "C#8" << "D8" << "D#8" << "E8" << "F8" << "F#8" << "G8";
	for(int i = 0; i < 128; ++i)
	{
		m_midiKeys[i] = map.at(i);
	}

	clear(false);
}

//---------------------------------------------------------
//   Song
//---------------------------------------------------------

Song::~Song()
{
	delete undoList;
	delete redoList;
	delete _markerList;
}

//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

void Song::putEvent(int pv)
{
	if (noteFifoSize < REC_NOTE_FIFO_SIZE)
	{
		recNoteFifo[noteFifoWindex] = pv;
		noteFifoWindex = (noteFifoWindex + 1) % REC_NOTE_FIFO_SIZE;
		++noteFifoSize;
	}
}

//---------------------------------------------------------
//   setTempo
//    public slot
//---------------------------------------------------------

void Song::setTempo(int newTempo)
{
	audio->msgSetTempo(pos[0].tick(), newTempo, true);
}

//---------------------------------------------------------
//   setSig
//    called from transport window
//---------------------------------------------------------

void Song::setSig(int z, int n)
{
	if (_masterFlag)
	{
		audio->msgAddSig(pos[0].tick(), z, n);
	}
}

void Song::setSig(const AL::TimeSignature& sig)
{
	if (_masterFlag)
	{
		audio->msgAddSig(pos[0].tick(), sig.z, sig.n);
	}
}

//---------------------------------------------------------
//    addNewTrack
//    Called from GUI context
//    Besides normal track types, n includes synth menu ids from populateAddTrack()
//---------------------------------------------------------

Track* Song::addNewTrack(QAction* action)
{
	int n = action->data().toInt();
	// Ignore negative numbers since this slot could be called by a menu or list etc. passing -1.
	if (n < 0)
		return 0;

	CreateTrackDialog *ctdialog = new CreateTrackDialog(n, -1, oom);
	connect(ctdialog, SIGNAL(trackAdded(qint64)), this, SLOT(newTrackAdded(qint64)));
	ctdialog->exec();

	return 0;
}


void Song::newTrackAdded(qint64 id)
{
	Track* t = findTrackById(id);
	if(t)
	{
		updateTrackViews();
		update(SC_SELECTION);
	}
}

//---------------------------------------------------------
//    addTrack
//    called from GUI context
//---------------------------------------------------------

Track* Song::addTrack(int t, bool doUndo)/*{{{*/
{
	Track::TrackType type = (Track::TrackType) t;
	Track* track = 0;
	switch (type)
	{
		case Track::MIDI:
			track = new MidiTrack();
			track->setType(Track::MIDI);
			
			if(config.partColorNames[lastTrackPartColorIndex].contains("menu:", Qt::CaseSensitive))
				lastTrackPartColorIndex ++;
			
			track->setDefaultPartColor(lastTrackPartColorIndex);
			lastTrackPartColorIndex ++;
			
			if(lastTrackPartColorIndex == NUM_PARTCOLORS)
				lastTrackPartColorIndex = 1;

			break;
		case Track::DRUM:
			track = new MidiTrack();
			track->setType(Track::DRUM);
			((MidiTrack*) track)->setOutChannel(9);
			break;
		case Track::WAVE:
			track = new WaveTrack();
			
			if(config.partColorNames[lastTrackPartColorIndex].contains("menu:", Qt::CaseSensitive))
				lastTrackPartColorIndex ++;
			
			track->setDefaultPartColor(lastTrackPartColorIndex);
			lastTrackPartColorIndex ++;
			
			if(lastTrackPartColorIndex == NUM_PARTCOLORS)
				lastTrackPartColorIndex = 1;
			
			((AudioTrack*) track)->addAuxSend();
			break;
		case Track::AUDIO_OUTPUT:
			track = new AudioOutput();
			break;
		case Track::AUDIO_BUSS:
			track = new AudioBuss();
			((AudioTrack*) track)->addAuxSend();
			break;
		case Track::AUDIO_AUX:
			track = new AudioAux();
			break;
		case Track::AUDIO_INPUT:
			track = new AudioInput();
			((AudioTrack*) track)->addAuxSend();
			break;
		default:
			printf("Song::addTrack() illegal type %d\n", type);
			abort();
	}
	track->setDefaultName();
	track->setHeight(DEFAULT_TRACKHEIGHT);
	insertTrack1(track, -1);
	msgInsertTrack(track, -1, doUndo);
	midiMonitor->msgAddMonitoredTrack(track);

	// Add default track <-> midiport routes.
	if (track->isMidiTrack())
	{
		//Create the Audio input side of the track
		Track* input = addTrackByName(QString("i").append(track->name()), Track::AUDIO_INPUT, -1, false, false);
		if(input)
		{
			input->setMasterFlag(false);
			input->setChainMaster(track->id());
			track->addManagedTrack(input->id());
		}
		MidiTrack* mt = (MidiTrack*) track;
		int c, cbi, ch;
		bool defOutFound = false; /// TODO: Remove this when multiple out routes supported.
		for (int i = 0; i < MIDI_PORTS; ++i)
		{
			MidiPort* mp = &midiPorts[i];

			c = mp->defaultInChannels();
			if (c)
			{
				audio->msgAddRoute(Route(i, c), Route(track, c));
				updateFlags |= SC_ROUTE;
			}

			if (!defOutFound)
			{
				c = mp->defaultOutChannels();
				if (c)
				{
					for (ch = 0; ch < MIDI_CHANNELS; ++ch)
					{
						cbi = 1 << ch;
						if (c & cbi)
						{
							defOutFound = true;
							mt->setOutPort(i);
							mt->setOutChannel(ch);
							updateFlags |= SC_ROUTE;
							break;
						}
					}
				}
			}
		}
	}
	else if(track->type() == Track::WAVE)
	{
		//Create the Audio input side of the track
		Track* input = addTrackByName(QString("i").append(track->name()), Track::AUDIO_INPUT, -1, false, false);
		if(input)
		{
			input->setMasterFlag(false);
			input->setChainMaster(track->id());
			track->addManagedTrack(input->id());
			
			//Route the input to the wave track
			Route srcRoute(input->name(), true, -1);
			Route dstRoute(track, 0, track->channels());
			audio->msgAddRoute(srcRoute, dstRoute);
			updateFlags |= SC_ROUTE;
		}
	}

	//
	//  add default route to master
	//
	AudioOutput* ao = (AudioOutput*)findTrackByIdAndType(m_masterId, Track::AUDIO_OUTPUT);
	if(ao)
	{
		switch (type)
		{
			case Track::WAVE:
			case Track::AUDIO_AUX:
				audio->msgAddRoute(Route((AudioTrack*) track, -1), Route(ao, -1));
				updateFlags |= SC_ROUTE;
				break;
			default:
				break;
		}
	}
	audio->msgUpdateSoloStates();
	//updateTrackViews();
	return track;
}/*}}}*/

Track* Song::addTrackByName(QString name, int t, int pos, bool doUndo, bool connectMaster)/*{{{*/
{
	Track::TrackType type = (Track::TrackType) t;
	Track* track = 0;
	switch (type)
	{
		case Track::MIDI:
			track = new MidiTrack();
			track->setType(Track::MIDI);
			
			if(config.partColorNames[lastTrackPartColorIndex].contains("menu:", Qt::CaseSensitive))
				lastTrackPartColorIndex ++;
			
			track->setDefaultPartColor(lastTrackPartColorIndex);
			lastTrackPartColorIndex ++;
			
			if(lastTrackPartColorIndex == NUM_PARTCOLORS)
				lastTrackPartColorIndex = 1;

			break;
		case Track::DRUM:
			track = new MidiTrack();
			track->setType(Track::DRUM);
			((MidiTrack*) track)->setOutChannel(9);
			break;
		case Track::WAVE:
			track = new WaveTrack();
			
			if(config.partColorNames[lastTrackPartColorIndex].contains("menu:", Qt::CaseSensitive))
				lastTrackPartColorIndex ++;
			
			track->setDefaultPartColor(lastTrackPartColorIndex);
			lastTrackPartColorIndex ++;
			
			if(lastTrackPartColorIndex == NUM_PARTCOLORS)
				lastTrackPartColorIndex = 1;
			
			((AudioTrack*) track)->addAuxSend();
			break;
		case Track::AUDIO_OUTPUT:
			track = new AudioOutput();
			break;
		case Track::AUDIO_BUSS:
			track = new AudioBuss();
			((AudioTrack*) track)->addAuxSend();
			break;
		case Track::AUDIO_AUX:
			track = new AudioAux();
			break;
		case Track::AUDIO_INPUT:
			track = new AudioInput();
			((AudioTrack*) track)->addAuxSend();
			break;
		case Track::AUDIO_SOFTSYNTH:
			break;
		default:
			printf("Song::addTrack() illegal type %d\n", type);
			abort();
	}
	track->setName(track->getValidName(name));
	track->setHeight(DEFAULT_TRACKHEIGHT);
	insertTrack1(track, pos);
	msgInsertTrack(track, pos, doUndo);
	midiMonitor->msgAddMonitoredTrack(track);

	// Add default track <-> midiport routes.
	if (track->isMidiTrack())
	{
		//Create the Audio input side of the track
		Track* input = addTrackByName(QString("i").append(track->name()), Track::AUDIO_INPUT, -1, false, false);
		if(input)
		{
			input->setMasterFlag(false);
			input->setChainMaster(track->id());
			track->addManagedTrack(input->id());
		}
		MidiTrack* mt = (MidiTrack*) track;
		int c, cbi, ch;
		bool defOutFound = false; /// TODO: Remove this when multiple out routes supported.
		for (int i = 0; i < MIDI_PORTS; ++i)
		{
			MidiPort* mp = &midiPorts[i];

			c = mp->defaultInChannels();
			if (c)
			{
				audio->msgAddRoute(Route(i, c), Route(track, c));
				updateFlags |= SC_ROUTE;
			}

			if (!defOutFound) ///
			{
				c = mp->defaultOutChannels();
				if (c)
				{

					/// TODO: Switch when multiple out routes supported.
#if 0
					audio->msgAddRoute(Route(track, c), Route(i, c));
					updateFlags |= SC_ROUTE;
#else 
					for (ch = 0; ch < MIDI_CHANNELS; ++ch)
					{
						cbi = 1 << ch;
						if (c & cbi)
						{
							defOutFound = true;
							mt->setOutPort(i);
							mt->setOutChannel(ch);
							updateFlags |= SC_ROUTE;
							break;
						}
					}
#endif
				}
			}
		}
	}
	else if(track->type() == Track::WAVE)
	{
		//Create the Audio input side of the track
		Track* input = addTrackByName(QString("i").append(track->name()), Track::AUDIO_INPUT, -1, false, false);
		if(input)
		{
			input->setMasterFlag(false);
			input->setChainMaster(track->id());
			track->addManagedTrack(input->id());
			
			//Route the input to the wave track
			Route srcRoute(input->name(), true, -1);
			Route dstRoute(track, 0, track->channels());
			audio->msgAddRoute(srcRoute, dstRoute);
			updateFlags |= SC_ROUTE;
		}
	}

	//
	//  add default route to master
	//
	if(connectMaster)
	{
		AudioOutput* ao = (AudioOutput*)findTrackByIdAndType(m_masterId, Track::AUDIO_OUTPUT);
		if(ao)
		{
			switch (type)
			{
				case Track::WAVE:
				case Track::AUDIO_AUX:
					audio->msgAddRoute(Route((AudioTrack*) track, -1), Route(ao, -1));
					updateFlags |= SC_ROUTE;
					break;
				default:
					break;
			}
		}
	}
	audio->msgUpdateSoloStates();
	//updateTrackViews();
	return track;
}/*}}}*/

//---------------------------------------------------------
//   cmdRemoveTrack
//---------------------------------------------------------

void Song::cmdRemoveTrack(Track* track)
{
	int idx = _tracks.index(track);
	undoOp(UndoOp::DeleteTrack, idx, track);
	removeTrackRealtime(track);
	updateFlags |= SC_TRACK_REMOVED;
}

//---------------------------------------------------------
//   removeMarkedTracks
//---------------------------------------------------------

void Song::removeMarkedTracks()
{
	bool loop;
	do
	{
		loop = false;
		for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
		{
			if ((*t)->selected())
			{
				removeTrackRealtime(*t);
				loop = true;
				break;
			}
		}
	} while (loop);
}

//---------------------------------------------------------
//   deselectTracks
//---------------------------------------------------------

void Song::deselectTracks()
{
	for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
		(*t)->setSelected(false);
}

void Song::deselectAllParts()
{
	for(iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
		(*t)->deselectParts();
	hasSelectedParts = false;
	update(SC_SELECTION);
}

void Song::disarmAllTracks()
{
	//printf("Song::disarmAllTracks()\n");
	if(viewselected)
	{
		unsigned int noop = -1;
		for(iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
		{
			if(_viewtracks.index((*t)) == noop)
			{
				(*t)->setRecordFlag1(false);
				(*t)->setRecordFlag2(false);
				(*t)->setSelected(false);
			}
		}
	}
}

//---------------------------------------------------------
//   changeTrack
//    oldTrack - copy of the original track befor modification
//    newTrack - modified original track
//---------------------------------------------------------

void Song::changeTrack(Track* oldTrack, Track* newTrack)
{
	oldTrack->setSelected(false); //??
	int idx = _tracks.index(newTrack);

	undoOp(UndoOp::ModifyTrack, idx, oldTrack, newTrack);
	updateFlags |= SC_TRACK_MODIFIED;
}

//---------------------------------------------------------
//  addEvent
//    return true if event was added
//---------------------------------------------------------

bool Song::addEvent(Event& event, Part* part)
{
	// Return false if the event is already found.
	if (part->events()->find(event) != part->events()->end())
	{
		// This can be normal for some (redundant) operations.
		if (debugMsg)
			printf("Song::addEvent event already found in part:%s size:%zd\n", part->name().toLatin1().constData(), part->events()->size());
		return false;
	}

	part->events()->add(event);
	return true;
}

//---------------------------------------------------------
//   changeEvent
//---------------------------------------------------------

void Song::changeEvent(Event& oldEvent, Event& newEvent, Part* part)
{
	iEvent i = part->events()->find(oldEvent);

	if (i == part->events()->end())
	{
		// This can be normal for some (redundant) operations.
		if (debugMsg)
			printf("Song::changeEvent event not found in part:%s size:%zd\n", part->name().toLatin1().constData(), part->events()->size());
	}
	else
		part->events()->erase(i);

	part->events()->add(newEvent);
}

//---------------------------------------------------------
//   deleteEvent
//---------------------------------------------------------

void Song::deleteEvent(Event& event, Part* part)
{
	iEvent ev = part->events()->find(event);
	if (ev == part->events()->end())
	{
		// This can be normal for some (redundant) operations.
		if (debugMsg)
			printf("Song::deleteEvent event not found in part:%s size:%zd\n", part->name().toLatin1().constData(), part->events()->size());
		return;
	}
	part->events()->erase(ev);
}

//---------------------------------------------------------
//   remapPortDrumCtrlEvents
//   Called when drum map anote, channel, or port is changed.
//---------------------------------------------------------

void Song::remapPortDrumCtrlEvents(int mapidx, int newnote, int newchan, int newport)
{
	if (mapidx == -1)
		return;

	for (ciMidiTrack it = _midis.begin(); it != _midis.end(); ++it)
	{
		MidiTrack* mt = *it;
		if (mt->type() != Track::DRUM)
			continue;

		MidiPort* trackmp = &midiPorts[mt->outPort()];
		const PartList* pl = mt->cparts();
		for (ciPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			MidiPart* part = (MidiPart*) (ip->second);
			const EventList* el = part->cevents();
			unsigned len = part->lenTick();
			for (ciEvent ie = el->begin(); ie != el->end(); ++ie)
			{
				const Event& ev = ie->second;
				// Added by T356. Do not handle events which are past the end of the part.
				if (ev.tick() >= len)
					break;

				if (ev.type() != Controller)
					continue;

				int cntrl = ev.dataA();

				// Is it a drum controller event, according to the track port's instrument?
				MidiController* mc = trackmp->drumController(cntrl);
				if (!mc)
					continue;

				int note = cntrl & 0x7f;
				// Does the index match?
				if (note == mapidx)
				{
					int tick = ev.tick() + part->tick();
					int ch = drumMap[note].channel;
					int port = drumMap[note].port;
					MidiPort* mp = &midiPorts[port];
					cntrl = (cntrl & ~0xff) | drumMap[note].anote;

					// Remove the port controller value.
					mp->deleteController(ch, tick, cntrl, part);

					if (newnote != -1 && newnote != drumMap[note].anote)
						cntrl = (cntrl & ~0xff) | newnote;
					if (newchan != -1 && newchan != ch)
						ch = newchan;
					if (newport != -1 && newport != port)
						port = newport;

					mp = &midiPorts[port];

					// Add the port controller value.
					mp->setControllerVal(ch, tick, cntrl, ev.dataB(), part);
				}
			}
		}
	}
}

//---------------------------------------------------------
//   changeAllPortDrumCtlEvents
//   add true: add events. false: remove events
//   drumonly true: Do drum controller events ONLY. false (default): Do ALL controller events.
//---------------------------------------------------------

void Song::changeAllPortDrumCtrlEvents(bool add, bool drumonly)
{
	int ch, trackch, cntrl, tick;
	MidiPort* mp, *trackmp;
	for (ciMidiTrack it = _midis.begin(); it != _midis.end(); ++it)
	{
		MidiTrack* mt = *it;
		if (mt->type() != Track::DRUM)
			continue;

		trackmp = &midiPorts[mt->outPort()];
		trackch = mt->outChannel();
		const PartList* pl = mt->cparts();
		for (ciPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			MidiPart* part = (MidiPart*) (ip->second);
			const EventList* el = part->cevents();
			unsigned len = part->lenTick();
			for (ciEvent ie = el->begin(); ie != el->end(); ++ie)
			{
				const Event& ev = ie->second;
				// Added by T356. Do not handle events which are past the end of the part.
				if (ev.tick() >= len)
					break;

				if (ev.type() != Controller)
					continue;

				cntrl = ev.dataA();
				mp = trackmp;
				ch = trackch;

				// Is it a drum controller event, according to the track port's instrument?
				if (trackmp->drumController(cntrl))
				{
					int note = cntrl & 0x7f;
					ch = drumMap[note].channel;
					mp = &midiPorts[drumMap[note].port];
					cntrl = (cntrl & ~0xff) | drumMap[note].anote;
				}
				else
				{
					if (drumonly)
						continue;
				}

				tick = ev.tick() + part->tick();

				if (add)
					// Add the port controller value.
					mp->setControllerVal(ch, tick, cntrl, ev.dataB(), part);
				else
					// Remove the port controller value.
					mp->deleteController(ch, tick, cntrl, part);
			}
		}
	}
}

//---------------------------------------------------------
//   cmdAddRecordedEvents
//    add recorded Events into part
//---------------------------------------------------------

void Song::cmdAddRecordedEvents(MidiTrack* mt, EventList* events, unsigned startTick)
{
	if (events->empty())
	{
		if (debugMsg)
			printf("no events recorded\n");
		return;
	}
	iEvent s;
	iEvent e;
	unsigned endTick;

	// Changed by Tim. p3.3.8

	//if (punchin())
	if ((audio->loopCount() > 0 && startTick > lPos().tick()) || (punchin() && startTick < lPos().tick()))
	{
		startTick = lpos();
		s = events->lower_bound(startTick);
	}
	else
	{
		s = events->begin();
		//            startTick = s->first;
	}

	// Changed by Tim. p3.3.8

	//if (punchout())
	//{
	//      endTick = rpos();
	//      e = events->lower_bound(endTick);
	//}
	//else
	//{
	// search for last noteOff:
	endTick = 0;
	for (iEvent i = events->begin(); i != events->end(); ++i)
	{
		Event ev = i->second;
		unsigned l = ev.endTick();
		if (l > endTick)
			endTick = l;
	}
	//      e = events->end();
	//}
	if ((audio->loopCount() > 0) || (punchout() && endTick > rPos().tick()))
	{
		endTick = rpos();
		e = events->lower_bound(endTick);
	}
	else
		e = events->end();

	if (startTick > endTick)
	{
		if (debugMsg)
			printf("no events in record area\n");
		return;
	}

	//---------------------------------------------------
	//    if startTick points into a part,
	//          record to that part
	//    else
	//          create new part
	//---------------------------------------------------

	PartList* pl = mt->parts();
	MidiPart* part = 0;
	iPart ip;
	for (ip = pl->begin(); ip != pl->end(); ++ip)
	{
		part = (MidiPart*) (ip->second);
		unsigned partStart = part->tick();
		unsigned partEnd = part->endTick();
		if (startTick >= partStart && startTick < partEnd)
			break;
	}
	if (ip == pl->end())
	{
		if (debugMsg)
			printf("create new part for recorded events\n");
		// create new part
		part = new MidiPart(mt);

		// Changed by Tim. p3.3.8

		// Honour the Composer snap settings. (Set to bar by default).
		//startTick = roundDownBar(startTick);
		//endTick   = roundUpBar(endTick);
		// Round the start down using the Composer part snap raster value.
		startTick = AL::sigmap.raster1(startTick, composerRaster());
		// Round the end up using the Composer part snap raster value.
		endTick = AL::sigmap.raster2(endTick, composerRaster());

		part->setTick(startTick);
		part->setLenTick(endTick - startTick);
		part->setName(mt->name());
		// copy events
		for (iEvent i = s; i != e; ++i)
		{
			Event old = i->second;
			Event event = old.clone();
			event.setTick(old.tick() - startTick);
			// addEvent also adds port controller values. So does msgAddPart, below. Let msgAddPart handle them.
			//addEvent(event, part);
			if (part->events()->find(event) == part->events()->end())
				part->events()->add(event);
		}
		audio->msgAddPart(part);
		updateFlags |= SC_PART_INSERTED;
		return;
	}

	updateFlags |= SC_EVENT_INSERTED;

	unsigned partTick = part->tick();
	if (endTick > part->endTick())
	{
		// Determine new part length...
		endTick = 0;
		for (iEvent i = s; i != e; ++i)
		{
			Event event = i->second;
			unsigned tick = event.tick() - partTick + event.lenTick();
			if (endTick < tick)
				endTick = tick;
		}
		// Added by Tim. p3.3.8

		// Round the end up (again) using the Composer part snap raster value.
		endTick = AL::sigmap.raster2(endTick, composerRaster());

		// Remove all of the part's port controller values. Indicate do not do clone parts.
		removePortCtrlEvents(part, false);
		// Clone the part. This doesn't increment aref count, and doesn't chain clones.
		// It also gives the new part a new serial number, but it is
		//  overwritten with the old one by Song::changePart(), below.
		Part* newPart = part->clone();
		// Set the new part's length.
		newPart->setLenTick(endTick);
		// Change the part.
		changePart(part, newPart);
		// Manually adjust reference counts.
		part->events()->incARef(-1);
		newPart->events()->incARef(1);
		// Replace the part in the clone chain with the new part.
		replaceClone(part, newPart);
		// Now add all of the new part's port controller values. Indicate do not do clone parts.
		addPortCtrlEvents(newPart, false);
		// Create an undo op. Indicate do port controller values but not clone parts.
		undoOp(UndoOp::ModifyPart, part, newPart, true, false);
		updateFlags |= SC_PART_MODIFIED;

		if (_recMode == REC_REPLACE)
		{
			iEvent si = newPart->events()->lower_bound(startTick - newPart->tick());
			iEvent ei = newPart->events()->lower_bound(newPart->endTick() - newPart->tick());
			for (iEvent i = si; i != ei; ++i)
			{
				Event event = i->second;
				// Create an undo op. Indicate do port controller values and clone parts.
				undoOp(UndoOp::DeleteEvent, event, newPart, true, true);
				// Remove the event from the new part's port controller values, and do all clone parts.
				removePortCtrlEvents(event, newPart, true);
			}
			newPart->events()->erase(si, ei);
		}

		for (iEvent i = s; i != e; ++i)
		{
			Event event = i->second;
			event.setTick(event.tick() - partTick);
			Event e;
			// Create an undo op. Indicate do port controller values and clone parts.
			undoOp(UndoOp::AddEvent, e, event, newPart, true, true);

			if (newPart->events()->find(event) == newPart->events()->end())
				newPart->events()->add(event);

			// Add the event to the new part's port controller values, and do all clone parts.
			addPortCtrlEvents(event, newPart, true);
		}
	}
	else
	{
		if (_recMode == REC_REPLACE)
		{
			iEvent si = part->events()->lower_bound(startTick - part->tick());
			iEvent ei = part->events()->lower_bound(endTick - part->tick());

			for (iEvent i = si; i != ei; ++i)
			{
				Event event = i->second;
				// Create an undo op. Indicate that controller values and clone parts were handled.
				undoOp(UndoOp::DeleteEvent, event, part, true, true);
				// Remove the event from the part's port controller values, and do all clone parts.
				removePortCtrlEvents(event, part, true);
			}
			part->events()->erase(si, ei);
		}
		for (iEvent i = s; i != e; ++i)
		{
			Event event = i->second;
			int tick = event.tick() - partTick;
			event.setTick(tick);

			// Create an undo op. Indicate that controller values and clone parts were handled.
			undoOp(UndoOp::AddEvent, event, part, true, true);

			if (part->events()->find(event) == part->events()->end())
				part->events()->add(event);

			// Add the event to the part's port controller values, and do all clone parts.
			addPortCtrlEvents(event, part, true);
		}
	}
}

//---------------------------------------------------------
//   findTrack
//---------------------------------------------------------

MidiTrack* Song::findTrack(const Part* part) const
{
	for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t)
	{
		MidiTrack* track = dynamic_cast<MidiTrack*> (*t);
		if (track == 0)
			continue;
		PartList* pl = track->parts();
		for (iPart p = pl->begin(); p != pl->end(); ++p)
		{
			if (part == p->second)
				return track;
		}
	}
	return 0;
}

//---------------------------------------------------------
//   findTrack
//    find track by name
//---------------------------------------------------------

Track* Song::findTrack(const QString& name) const
{
	for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
	{
		if ((*i)->name() == name)
			return *i;
	}
	return 0;
}

//---------------------------------------------------------
//   updateAuxIndex
//   Update all respective tracks to use trackId for AUX
//   vs the old index order
//---------------------------------------------------------

void Song::updateAuxIndex()
{
	qDebug("Song::updateAuxIndex: Auxes found in song with old index, auto updating");
	for(ciTrack it = _tracks.begin(); it != _tracks.end(); ++it)
	{
		Track* track = *it;
		Track::TrackType type = (Track::TrackType) track->type();
		switch (type)
		{
			case Track::WAVE:
			case Track::AUDIO_BUSS:
			case Track::AUDIO_INPUT:
			{	
				//QList<int> repList;
				QList<qint64> inList;
				QHash<qint64, AuxInfo> *auxList = ((AudioTrack*) track)->auxSends();
				QHash<qint64, AuxInfo>::const_iterator iter = auxList->constBegin();
				while(iter != auxList->constEnd())
				{
					if(iter.key() < 1000)
					{
						inList.append(iter.key());
					}
					++iter;
				}
				if(!inList.isEmpty())
				{
					foreach(qint64 id, inList)
					{
						AuxInfo info = auxList->take(id);
						int inId = (int)id;
						AudioAux* at = (AudioAux*)_auxs[inId];
						//AudioAux* a = (AudioAux*) ((*al)[k]);
						if(at)
						{
							auxList->insert(at->id(), info);
						}
					}
				}
				((AudioTrack*) track)->addAuxSend();
			}
			break;
			default:
			break;
		}
	}
	gUpdateAuxes = false;
	update();
	dirty = true;
	//TODO: show dialog here to ask the user to save project
	int n = QMessageBox::warning(oom, tr("OOMidi: Aux Auto Update"),
			tr("This Project was found to be in an old format\n"
			"It has been automaticly updated\n"
			"Save Current Project?"),
			tr("&Save"), tr("&Don't Save"), 0, 1);
	if (n == 0)
	{
		oom->save();
	}
}

//---------------------------------------------------------
//   findTrackById
//    find track by id
//---------------------------------------------------------

Track* Song::findTrackById(qint64 id) const
{
	for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
	{
		if ((*i)->id() == id)
			return *i;
	}
	return 0;
}

//---------------------------------------------------------
//   findTrackByIdAndType
//    find track by id and type
//---------------------------------------------------------

Track* Song::findTrackByIdAndType(qint64 id, int ttype) const
{
	Track::TrackType type = (Track::TrackType)ttype;
	switch(type)
	{
		case Track::MIDI:
		case Track::DRUM:
		{/*{{{*/
			for (ciTrack i = _midis.begin(); i != _midis.end(); ++i)
			{
				if ((*i)->id() == id)
					return *i;
			}
		}/*}}}*/
		break;
		case Track::WAVE:
		{
			for (ciTrack i = _waves.begin(); i != _waves.end(); ++i)
			{
				if ((*i)->id() == id)
					return *i;
			}
		}
		break;
		case Track::AUDIO_OUTPUT:
		{
			for (ciTrack i = _outputs.begin(); i != _outputs.end(); ++i)
			{
				if ((*i)->id() == id)
					return *i;
			}
		}
		break;
		case Track::AUDIO_INPUT:
		{
			for (ciTrack i = _inputs.begin(); i != _inputs.end(); ++i)
			{
				if ((*i)->id() == id)
					return *i;
			}
		}
		break;
		case Track::AUDIO_BUSS:
		{
			for (ciTrack i = _groups.begin(); i != _groups.end(); ++i)
			{
				if ((*i)->id() == id)
					return *i;
			}
		}
		break;
		case Track::AUDIO_AUX:
		{
			for (ciTrack i = _auxs.begin(); i != _auxs.end(); ++i)
			{
				if ((*i)->id() == id)
					return *i;
			}
		}
		break;
		case Track::AUDIO_SOFTSYNTH:
        break;
	}
	return 0;
}

//---------------------------------------------------------
// setReplay
// set transport audition flag
//---------------------------------------------------------

void Song::setReplay(bool t)
{
	_replay = t;
	if(t)
	{
		_replayPos = song->cpos();
		emit replayChanged(_replay, _replayPos);
	}
}

void Song::updateReplayPos()
{
	_replayPos = song->cpos();
	emit replayChanged(_replay, _replayPos);
}

//---------------------------------------------------------
//   setLoop
//    set transport loop flag
//---------------------------------------------------------

void Song::setLoop(bool f)
{
	if (loopFlag != f)
	{
		loopFlag = f;
		loopAction->setChecked(loopFlag);
		emit loopChanged(loopFlag);
	}
}

//---------------------------------------------------------
//   clearTrackRec
//---------------------------------------------------------

void Song::clearTrackRec()
{
	//printf("Song::clearTrackRec()\n");
	for (iTrack it = tracks()->begin(); it != tracks()->end(); ++it)
		setRecordFlag(*it, false);
}

//---------------------------------------------------------
//   setRecord
//---------------------------------------------------------

void Song::setRecord(bool f, bool autoRecEnable)
{
	if(debugMsg)
		printf("Song::setRecord recordflag =%d f(record state)=%d autoRecEnable=%d\n", recordFlag, f, autoRecEnable);
	if (f && oomProject == oomProjectInitPath)
	{ // check that there is a project stored before commencing
		// no project, we need to create one.
		if (!oom->saveAs())
			return; // could not store project, won't enable record
	}
	if (recordFlag != f)
	{
		if (f && autoRecEnable)
		{
			bool alreadyRecEnabled = false;
			Track *selectedTrack = 0;
			// loop through list and check if any track is rec enabled
			// if not then rec enable the selected track
			WaveTrackList* wtl = waves();
			for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i)
			{
				if ((*i)->recordFlag())
				{
					alreadyRecEnabled = true;
					break;
				}
				if ((*i)->selected())
					selectedTrack = (*i);
			}
			if (!alreadyRecEnabled)
			{
				MidiTrackList* mtl = midis();
				for (iMidiTrack it = mtl->begin(); it != mtl->end(); ++it)
				{
					if ((*it)->recordFlag())
					{
						alreadyRecEnabled = true;
						break;
					}
					if ((*it)->selected())
						selectedTrack = (*it);
				}
			}
			if (!alreadyRecEnabled && selectedTrack)
			{
				setRecordFlag(selectedTrack, true);
			}
			else if (alreadyRecEnabled)
			{
				// do nothing
			}
			else
			{
				// if there are no tracks, do not enable record
				if (!waves()->size() && !midis()->size())
				{
					printf("No track to select, won't enable record\n");
					f = false;
				}
			}
			// prepare recording of wave files for all record enabled wave tracks
			for (iWaveTrack i = wtl->begin(); i != wtl->end(); ++i)
			{
				if ((*i)->recordFlag() || (selectedTrack == (*i) && autoRecEnable)) // prepare if record flag or if it is set to recenable
				{	// setRecordFlag may take too long time to complete
					// so we try this case specifically
					(*i)->prepareRecording();
				}
			}

#if 0
			// check for midi devices suitable for recording
			bool portFound = false;
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				MidiDevice* dev = midiPorts[i].device();
				if (dev && (dev->rwFlags() & 0x2))
					portFound = true;
			}
			if (!portFound)
			{
				QMessageBox::critical(qApp->mainWidget(), "OOMidi: Record",
						"There are no midi devices configured for recording");
				f = false;
			}
#endif
		}
		else
		{
			bounceTrack = 0;
		}
		if (audio->isPlaying() && f)
			f = false;
		recordFlag = f;
		recordAction->setChecked(recordFlag);
		emit recordChanged(recordFlag);
	}
}

//---------------------------------------------------------
//   setPunchin
//    set punchin flag
//---------------------------------------------------------

void Song::setPunchin(bool f)
{
	if (punchinFlag != f)
	{
		punchinFlag = f;
		punchinAction->setChecked(punchinFlag);
		emit punchinChanged(punchinFlag);
	}
}

//---------------------------------------------------------
//   setPunchout
//    set punchout flag
//---------------------------------------------------------

void Song::setPunchout(bool f)
{
	if (punchoutFlag != f)
	{
		punchoutFlag = f;
		punchoutAction->setChecked(punchoutFlag);
		emit punchoutChanged(punchoutFlag);
	}
}

//---------------------------------------------------------
//   setClick
//---------------------------------------------------------

void Song::setClick(bool val)
{
	if(val)/*{{{*/
	{
		AudioOutput* master = (AudioOutput*)findTrackByIdAndType(m_masterId, Track::AUDIO_OUTPUT);
		bool hasoutput = (master && master->sendMetronome());
		if(!hasoutput)
		{
			for (iAudioOutput iao = _outputs.begin(); iao != _outputs.end(); ++iao)
			{
				AudioOutput* t = (AudioOutput*)*iao;
				if(t && t->sendMetronome())
				{
					hasoutput = true;
					break;
				}
			}
		}
		if(!hasoutput || !audioClickFlag)
		{
			oom->configMetronome();
		}
	}/*}}}*/
	if (_click != val)
	{
		_click = val;
		emit clickChanged(_click);
	}
}

//---------------------------------------------------------
//   setQuantize
//---------------------------------------------------------

void Song::setQuantize(bool val)
{
	if (_quantize != val)
	{
		_quantize = val;
		emit quantizeChanged(_quantize);
	}
}

//---------------------------------------------------------
//   setMasterFlag
//---------------------------------------------------------

void Song::setMasterFlag(bool val)
{
	_masterFlag = val;
	if (tempomap.setMasterFlag(cpos(), val))
	{
		if(!invalid)
			emit songChanged(SC_MASTER);
	}
	masterEnableAction->blockSignals(true);
	masterEnableAction->setChecked(song->masterFlag());
	masterEnableAction->blockSignals(false);
}

//---------------------------------------------------------
//   setPlay
//    set transport play flag
//---------------------------------------------------------

void Song::setPlay(bool f)
{
	if (extSyncFlag.value())
	{
		if (debugMsg)
			printf("not allowed while using external sync");
		return;
	}
	// only allow the user to set the button "on"
	if (!f)
		playAction->setChecked(true);
	else
	{
		audio->msgPlay(true);
	}
	emit playChanged(f); // signal transport window
}

void Song::setStop(bool f)
{
	if (extSyncFlag.value())
	{
		if (debugMsg)
			printf("not allowed while using external sync");
		return;
	}
	// only allow the user to set the button "on"
	if (!f)
		stopAction->setChecked(true);
	else
	{
		audio->msgPlay(false);
		if(_replay)
		{
			Pos p(_replayPos, true);
			setPos(0, p, true, true, true);
		}
		//emit playbackStateChanged(false);
	}
}

void Song::setStopPlay(bool f)
{
	playAction->blockSignals(true);
	stopAction->blockSignals(true);

	emit playChanged(f); // signal transport window

	playAction->setChecked(f);
	stopAction->setChecked(!f);

	stopAction->blockSignals(false);
	playAction->blockSignals(false);
}

//---------------------------------------------------------
//   swapTracks
//---------------------------------------------------------

void Song::swapTracks(int i1, int i2)
{
	undoOp(UndoOp::SwapTrack, i1, i2);
	//printf("Song::swapTracks(int %d, int %d)\n", i1, i2);
	Track* track = _viewtracks[i1];
	_viewtracks[i1] = _viewtracks[i2];
	_viewtracks[i2] = track;

	if(!viewselected)
	{
		Track *t = _artracks[i1];
		_artracks[i1] = _artracks[i2];
		_artracks[i2] = t;
	}
	else
	{
		Track *t = _tracks[i1];
		_tracks[i1] = _tracks[i2];
		_tracks[i2] = t;
	}
}

//---------------------------------------------------------
//   setPos
//   song->setPos(Song::CPOS, pos, true, true, true);
//---------------------------------------------------------

void Song::setPos(int idx, const Pos& val, bool sig,
		bool isSeek, bool adjustScrollbar)
{
	//      printf("setPos %d sig=%d,seek=%d,scroll=%d  ",
	//         idx, sig, isSeek, adjustScrollbar);
	//      val.dump(0);
	//      printf("\n");

	// p3.3.23
	//printf("Song::setPos before audio->msgSeek idx:%d isSeek:%d frame:%d\n", idx, isSeek, val.frame());
	if (pos[idx] == val)
		return;
	if (idx == CPOS)
	{
		_vcpos = val;
		if (isSeek && !extSyncFlag.value())
		{
			audio->msgSeek(val);
			// p3.3.23
			//printf("Song::setPos after audio->msgSeek idx:%d isSeek:%d frame:%d\n", idx, isSeek, val.frame());
			return;
		}
	}
	pos[idx] = val;
	bool swap = pos[LPOS] > pos[RPOS];
	if (swap)
	{ // swap lpos/rpos if lpos > rpos
		Pos tmp = pos[LPOS];
		pos[LPOS] = pos[RPOS];
		pos[RPOS] = tmp;
	}
	if (sig)
	{
		if (swap)
		{
			emit posChanged(LPOS, pos[LPOS].tick(), adjustScrollbar);
			emit posChanged(RPOS, pos[RPOS].tick(), adjustScrollbar);
			if (idx != LPOS && idx != RPOS)
				emit posChanged(idx, pos[idx].tick(), adjustScrollbar);
		}
		else
			emit posChanged(idx, pos[idx].tick(), adjustScrollbar);
	}

	if (idx == CPOS)
	{
		iMarker i1 = _markerList->begin();
		iMarker i2 = i1;
		bool currentChanged = false;
		for (; i1 != _markerList->end(); ++i1)
		{
			++i2;
			if (val.tick() >= i1->first && (i2 == _markerList->end() || val.tick() < i2->first))
			{
				if (i1->second.current())
					return;
				i1->second.setCurrent(true);
				if (currentChanged)
				{
					emit markerChanged(MARKER_CUR);
					return;
				}
				++i1;
				for (; i1 != _markerList->end(); ++i1)
				{
					if (i1->second.current())
						i1->second.setCurrent(false);
				}
				emit markerChanged(MARKER_CUR);
				return;
			}
			else
			{
				if (i1->second.current())
				{
					currentChanged = true;
					i1->second.setCurrent(false);
				}
			}
		}
		if (currentChanged)
			emit markerChanged(MARKER_CUR);
	}
}

//---------------------------------------------------------
//   forward
//---------------------------------------------------------

void Song::forward()
{
	unsigned newPos = pos[0].tick() + config.division;
	audio->msgSeek(Pos(newPos, true));
}

//---------------------------------------------------------
//   rewind
//---------------------------------------------------------

void Song::rewind()
{
	unsigned newPos;
	if (unsigned(config.division) > pos[0].tick())
		newPos = 0;
	else
		newPos = pos[0].tick() - config.division;
	audio->msgSeek(Pos(newPos, true));
}

//---------------------------------------------------------
//   rewindStart
//---------------------------------------------------------

void Song::rewindStart()
{
	// Added by T356
	//audio->msgIdle(true);

	audio->msgSeek(Pos(0, true));

	// Added by T356
	//audio->msgIdle(false);
}

//---------------------------------------------------------
//   update
//---------------------------------------------------------

void Song::update(int flags)
{
	static int level = 0; // DEBUG
	if (level)
	{
		printf("Song::update %08x, level %d\n", flags, level);
		return;
	}
	++level;
	if(flags & (SC_TRACK_REMOVED | SC_TRACK_INSERTED/* | SC_TRACK_MODIFIED*/))
	{
		//printf("Song::update firing updateTrackViews\n");
		updateTrackViews();
	}
	/*if(flags & SC_VIEW_CHANGED)
	{
		emit composerViewChanged();
	}*/
	if(!invalid)
		emit songChanged(flags);
	--level;
}

//---------------------------------------------------------
//   updatePos
//---------------------------------------------------------

void Song::updatePos()
{
	emit posChanged(0, pos[0].tick(), false);
	emit posChanged(1, pos[1].tick(), false);
	emit posChanged(2, pos[2].tick(), false);
}

//---------------------------------------------------------
//   setChannelMute
//    mute all midi tracks associated with channel
//---------------------------------------------------------

void Song::setChannelMute(int channel, bool val)
{
	for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
	{
		MidiTrack* track = dynamic_cast<MidiTrack*> (*i);
		if (track == 0)
			continue;
		if (track->outChannel() == channel)
			track->setMute(val);
	}
	if(!invalid)
		emit songChanged(SC_MUTE);
}

//---------------------------------------------------------
//   len
//---------------------------------------------------------

void Song::initLen()
{
	_len = AL::sigmap.bar2tick(264, 0, 0); // default song len
	for (iTrack t = _tracks.begin(); t != _tracks.end(); ++t)
	{
		if (!(*t)->isMidiTrack())
			continue;
		MidiTrack* track = dynamic_cast<MidiTrack*> (*t);
		PartList* parts = track->parts();
		for (iPart p = parts->begin(); p != parts->end(); ++p)
		{
			unsigned last = p->second->tick() + p->second->lenTick();
			if (last > _len)
				_len = last;
		}
	}
	_len = roundUpBar(_len);
}

//---------------------------------------------------------
//   tempoChanged
//---------------------------------------------------------

void Song::tempoChanged()
{
	if(!invalid)
		emit songChanged(SC_TEMPO);
}

//---------------------------------------------------------
//   roundUpBar
//---------------------------------------------------------

int Song::roundUpBar(int t) const
{
	int bar, beat;
	unsigned tick;
	AL::sigmap.tickValues(t, &bar, &beat, &tick);
	if (beat || tick)
		return AL::sigmap.bar2tick(bar + 1, 0, 0);
	return t;
}

//---------------------------------------------------------
//   roundUpBeat
//---------------------------------------------------------

int Song::roundUpBeat(int t) const
{
	int bar, beat;
	unsigned tick;
	AL::sigmap.tickValues(t, &bar, &beat, &tick);
	if (tick)
		return AL::sigmap.bar2tick(bar, beat + 1, 0);
	return t;
}

//---------------------------------------------------------
//   roundDownBar
//---------------------------------------------------------

int Song::roundDownBar(int t) const
{
	int bar, beat;
	unsigned tick;
	AL::sigmap.tickValues(t, &bar, &beat, &tick);
	return AL::sigmap.bar2tick(bar, 0, 0);
}

//---------------------------------------------------------
//   dumpMaster
//---------------------------------------------------------

void Song::dumpMaster()
{
	tempomap.dump();
	AL::sigmap.dump();
}

//---------------------------------------------------------
//   getSelectedParts
//---------------------------------------------------------

PartList* Song::getSelectedMidiParts() const
{
	PartList* parts = new PartList();

	//------------------------------------------------------
	//    wenn ein Part selektiert ist, diesen editieren
	//    wenn ein Track selektiert ist, den Ersten
	//       Part des Tracks editieren, die restlichen sind
	//       'ghostparts'
	//    wenn mehrere Parts selektiert sind, dann Ersten
	//       editieren, die restlichen sind 'ghostparts'
	//
	// Rough translation:
	/*
		  If a part is selected, edit that.
		  If a track is selected, edit the first
		   part of the track, the rest are
		   'ghost parts'
		  When multiple parts are selected, then edit the first,
			the rest are 'ghost parts'
	 */


	// collect marked parts
	for (ciMidiTrack t = _midis.begin(); t != _midis.end(); ++t)
	{
		MidiTrack* track = *t;
		PartList* pl = track->parts();
		for (iPart p = pl->begin(); p != pl->end(); ++p)
		{
			if (p->second->selected())
			{
				parts->add(p->second);
			}
		}
	}
	// if no part is selected, then search for selected track
	// and collect all parts of this track

	if (parts->empty())
	{
		for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t)
		{
			if ((*t)->selected())
			{
				MidiTrack* track = dynamic_cast<MidiTrack*> (*t);
				if (track == 0)
					continue;
				PartList* pl = track->parts();
				for (iPart p = pl->begin(); p != pl->end(); ++p)
					parts->add(p->second);
				break;
			}
		}
	}
	return parts;
}

PartList* Song::getSelectedWaveParts() const
{
	PartList* parts = new PartList();

	//------------------------------------------------------
	//    wenn ein Part selektiert ist, diesen editieren
	//    wenn ein Track selektiert ist, den Ersten
	//       Part des Tracks editieren, die restlichen sind
	//       'ghostparts'
	//    wenn mehrere Parts selektiert sind, dann Ersten
	//       editieren, die restlichen sind 'ghostparts'
	//

	// markierte Parts sammeln
	for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t)
	{
		WaveTrack* track = dynamic_cast<WaveTrack*> (*t);
		if (track == 0)
			continue;
		PartList* pl = track->parts();
		for (iPart p = pl->begin(); p != pl->end(); ++p)
		{
			if (p->second->selected())
			{
				parts->add(p->second);
			}
		}
	}
	// wenn keine Parts selektiert, dann markierten Track suchen
	// und alle Parts dieses Tracks zusammensuchen

	if (parts->empty())
	{
		for (ciTrack t = _tracks.begin(); t != _tracks.end(); ++t)
		{
			if ((*t)->selected())
			{
				WaveTrack* track = dynamic_cast<WaveTrack*> (*t);
				if (track == 0)
					continue;
				PartList* pl = track->parts();
				for (iPart p = pl->begin(); p != pl->end(); ++p)
					parts->add(p->second);
				break;
			}
		}
	}
	return parts;
}

void Song::setMType(MType t)
{
	//   printf("set MType %d\n", t);
	_mtype = t;
	song->update(SC_SONG_TYPE); // p4.0.7 Tim.
}

//---------------------------------------------------------
//   beat
//---------------------------------------------------------

void Song::beat()
{
	// Keep the sync detectors running...
	for (int port = 0; port < MIDI_PORTS; ++port)
	{
		// Must keep them running even if there's no device...
		//if(midiPorts[port].device())
		midiPorts[port].syncInfo().setTime();
	}


	int tick = audio->tickPos();
	if (audio->isPlaying())
		setPos(0, tick, true, false, true);

	// p3.3.40 Update synth native guis at the heartbeat rate.
    //for (ciSynthI is = _synthIs.begin(); is != _synthIs.end(); ++is)
    //	(*is)->guiHeartBeat();
	
	//Update native guis
	for(ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
	{
        AudioTrack* t = 0;
		if((*i)->isMidiTrack())
        {
            if ((*i)->wantsAutomation())
                t = ((MidiTrack*)(*i))->getAutomationTrack();
            else
                continue;
        }
        else
            t = (AudioTrack*)*i;

		if (t)
			t->efxPipe()->updateGuis();
	}

	while (noteFifoSize)
	{
		int pv = recNoteFifo[noteFifoRindex];
		noteFifoRindex = (noteFifoRindex + 1) % REC_NOTE_FIFO_SIZE;
		int pitch = (pv >> 8) & 0xff;
		int velo = pv & 0xff;

		//---------------------------------------------------
		// filter midi remote control events
		//---------------------------------------------------

		if (rcEnable && velo != 0)
		{
			if (pitch == rcStopNote)
				setStop(true);
			else if (pitch == rcRecordNote)
				setRecord(true);
			else if (pitch == rcGotoLeftMarkNote)
				setPos(0, pos[LPOS].tick(), true, true, true);
			else if (pitch == rcPlayNote)
				setPlay(true);
		}
		emit song->midiNote(pitch, velo);
		--noteFifoSize;
	}
	if(!invalid)
	{
		if(lsClient)
		{
			lsClientStarted = lsClient->isClientStarted();
			if(!lsClientStarted)
			{
				if(lsClient->startClient())
					lsClientStarted = true;
				else
				{
					lsClientStarted = false;
					lsClient = 0;
				}
			}
		}
		else
		{
			lsClientStarted = false;
		}
	}
}

//---------------------------------------------------------
//   setLen
//---------------------------------------------------------

void Song::setLen(unsigned l)
{
	_len = l;
	update();
}

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

Marker* Song::addMarker(const QString& s, int t, bool lck)
{
	Marker* marker = _markerList->add(s, t, lck);
	emit markerChanged(MARKER_ADD);
	return marker;
}

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

Marker* Song::getMarkerAt(int t)
{
	iMarker markerI;
	for (markerI = _markerList->begin(); markerI != _markerList->end(); ++markerI)
	{
		//                        if (i1->second.current())
		if (unsigned(t) == markerI->second.tick())//prevent of copmiler warning: comparison signed/unsigned
			return &markerI->second;
	}
	//Marker* marker = _markerList->add(s, t, lck);
	return NULL;
}

//---------------------------------------------------------
//   removeMarker
//---------------------------------------------------------

void Song::removeMarker(Marker* marker)
{
	_markerList->remove(marker);
	emit markerChanged(MARKER_REMOVE);
}

Marker* Song::setMarkerName(Marker* m, const QString& s)
{
	m->setName(s);
	emit markerChanged(MARKER_NAME);
	return m;
}

Marker* Song::setMarkerTick(Marker* m, int t)
{
	Marker mm(*m);
	_markerList->remove(m);
	mm.setTick(t);
	m = _markerList->add(mm);
	emit markerChanged(MARKER_TICK);
	return m;
}

Marker* Song::setMarkerLock(Marker* m, bool f)
{
	m->setType(f ? Pos::FRAMES : Pos::TICKS);
	emit markerChanged(MARKER_LOCK);
	return m;
}

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Song::setRecordFlag(Track* track, bool val, bool monitor)
{
	if(!monitor)
	{
		//Call the monitor here if it was not called from the monitor
		//midimonitor->msgSendMidiOutputEvent(track, CTRL_RECORD, val ? 127 : 0);
	}
	if (track->type() == Track::WAVE)
	{
		WaveTrack* audioTrack = (WaveTrack*) track;
		if (!audioTrack->setRecordFlag1(val, monitor))
		{
			printf("AudioTrack returns false on set record flag");
			return;
		}
		audio->msgSetRecord(audioTrack, val);
	}
	else
	{
		track->setRecordFlag1(val, monitor);
		track->setRecordFlag2(val, monitor);
	}
	//      updateFlags |= SC_RECFLAG;
	update(SC_RECFLAG);

}

//---------------------------------------------------------
//   rescanAlsaPorts
//---------------------------------------------------------

void Song::rescanAlsaPorts()
{
	emit midiPortsChanged();
}

//---------------------------------------------------------
//   endMsgCmd
//---------------------------------------------------------

void Song::endMsgCmd()
{
	if (updateFlags)
	{
		redoList->clear(); // TODO: delete elements in list
		undoAction->setEnabled(true);
		redoAction->setEnabled(false);
		if(updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED/* | SC_TRACK_MODIFIED*/))
		{
			//NOTE: This call was causing our mixer to update repeatedly, removed for now
			//keep an eye out for trackviews not updating after a undo/redo operation
			//printf("Song::endMsgCmd() calling updateTrackViews()\n");
			//updateTrackViews();
		}
		if(!invalid)
			emit songChanged(updateFlags);
	}
}

//---------------------------------------------------------
//   undo
//---------------------------------------------------------

void Song::undo()
{
	updateFlags = 0;
	if (doUndo1())
		return;
	audio->msgUndo();
	doUndo3();
	redoAction->setEnabled(true);
	undoAction->setEnabled(!undoList->empty());

	if (updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED))
		audio->msgUpdateSoloStates();

	if(updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED | SC_TRACK_MODIFIED))
		updateTrackViews();

	if(!invalid)
		emit songChanged(updateFlags);
}

//---------------------------------------------------------
//   redo
//---------------------------------------------------------

void Song::redo()
{
	updateFlags = 0;
	if (doRedo1())
		return;
	audio->msgRedo();
	doRedo3();
	undoAction->setEnabled(true);
	redoAction->setEnabled(!redoList->empty());

	if (updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED))
		audio->msgUpdateSoloStates();

	if(updateFlags && (SC_TRACK_REMOVED | SC_TRACK_INSERTED | SC_TRACK_MODIFIED))
		updateTrackViews();
	if(!invalid)
		emit songChanged(updateFlags);
}

//---------------------------------------------------------
//   processMsg
//    executed in realtime thread context
//---------------------------------------------------------

void Song::processMsg(AudioMsg* msg)
{
	switch (msg->id)
	{
		case SEQM_UPDATE_SOLO_STATES:
			updateSoloStates();
			break;
		case SEQM_UNDO:
			doUndo2();
			break;
		case SEQM_REDO:
			doRedo2();
			break;
		case SEQM_MOVE_TRACK:
			if (msg->a > msg->b)
			{
				for (int i = msg->a; i > msg->b; --i)
				{
					swapTracks(i, i - 1);
				}
			}
			else
			{
				for (int i = msg->a; i < msg->b; ++i)
				{
					swapTracks(i, i + 1);
				}
			}
			updateFlags = SC_TRACK_MODIFIED;
			break;
		case SEQM_ADD_EVENT:
			updateFlags = SC_EVENT_INSERTED;
			if (addEvent(msg->ev1, (MidiPart*) msg->p2))
			{
				Event ev;
				//undoOp(UndoOp::AddEvent, ev, msg->ev1, (Part*)msg->p2);
				undoOp(UndoOp::AddEvent, ev, msg->ev1, (Part*) msg->p2, msg->a, msg->b);
			}
			else
				updateFlags = 0;
			if (msg->a)
				addPortCtrlEvents(msg->ev1, (Part*) msg->p2, msg->b);
			break;
		case SEQM_ADD_EVENT_CHECK:
		{
			Track* track = msg->track;
			Event event = msg->ev1;
			unsigned tick = event.tick();
			PartList* pl = track->parts();
			if(pl && !pl->empty())
			{
				Part* part = pl->findAtTick(tick);
				if(part)
				{
					//recordEvent((MidiPart*)part, event);
					//unsigned tick = event.tick();
					int diff = event.endTick() - part->lenTick();
					if (diff > 0)
					{// too short part? extend it
						part->setLenTick(part->lenTick() + diff);
						updateFlags |= SC_PART_MODIFIED;
					}
					updateFlags |= SC_EVENT_INSERTED;
				
					tick -= part->tick();
					event.setTick(tick);
				
					Event ev;
					if (event.type() == Controller)
					{
						EventRange range = part->events()->equal_range(tick);
						for (iEvent i = range.first; i != range.second; ++i)
						{
							ev = i->second;
							// At the moment, Song::recordEvent() is only called by the 'Rec' buttons in the
							//  midi track info panel. So only controller types are fed to it. If other event types
							//  are to be passed, we will have to expand on this to check if equal. Instead, maybe add an isEqual() to Event class.
							if (ev.type() == Controller && ev.dataA() == event.dataA())
							{
								// Don't bother if already set.
								if (ev.dataB() == event.dataB())
									return;
								removePortCtrlEvents(ev, (MidiPart*) part, true);
								changeEvent(ev, event, (MidiPart*) part);
								addPortCtrlEvents(event, part, true);
								undoOp(UndoOp::ModifyEvent, event, ev, part, true, true);
								return;
							}
						}
					}
				
					if (addEvent(event, (MidiPart*) part))
					{
						Event ev;
						undoOp(UndoOp::AddEvent, ev, event, part, true, true);
					}
					addPortCtrlEvents(event, part, true);
				}
			}
		}
		break;
		case SEQM_REMOVE_EVENT:
		{
			Event event = msg->ev1;
			MidiPart* part = (MidiPart*) msg->p2;
			if (msg->a)
				removePortCtrlEvents(event, part, msg->b);
			Event e;
			undoOp(UndoOp::DeleteEvent, e, event, (Part*) part, msg->a, msg->b);
			deleteEvent(event, part);
			updateFlags = SC_EVENT_REMOVED;
		}
			break;
		case SEQM_CHANGE_EVENT:
			if (msg->a)
				removePortCtrlEvents(msg->ev1, (MidiPart*) msg->p3, msg->b);
			changeEvent(msg->ev1, msg->ev2, (MidiPart*) msg->p3);
			if (msg->a)
				addPortCtrlEvents(msg->ev2, (Part*) msg->p3, msg->b);
			undoOp(UndoOp::ModifyEvent, msg->ev2, msg->ev1, (Part*) msg->p3, msg->a, msg->b);
			updateFlags = SC_EVENT_MODIFIED;
			break;

		case SEQM_ADD_TEMPO:
			//printf("processMsg (SEQM_ADD_TEMPO) UndoOp::AddTempo. adding tempo at: %d with tempo=%d\n", msg->a, msg->b);
			undoOp(UndoOp::AddTempo, msg->a, msg->b);
			tempomap.addTempo(msg->a, msg->b);
			updateFlags = SC_TEMPO;
			break;

		case SEQM_SET_TEMPO:
			//printf("processMsg (SEQM_SET_TEMPO) UndoOp::AddTempo. adding tempo at: %d with tempo=%d\n", msg->a, msg->b);
			undoOp(UndoOp::AddTempo, msg->a, msg->b);
			tempomap.setTempo(msg->a, msg->b);
			updateFlags = SC_TEMPO;
			break;

		case SEQM_SET_GLOBAL_TEMPO:
			tempomap.setGlobalTempo(msg->a);
			break;

		case SEQM_REMOVE_TEMPO:
			//printf("processMsg (SEQM_REMOVE_TEMPO) UndoOp::DeleteTempo. adding tempo at: %d with tempo=%d\n", msg->a, msg->b);
			undoOp(UndoOp::DeleteTempo, msg->a, msg->b);
			tempomap.delTempo(msg->a);
			updateFlags = SC_TEMPO;
			break;

		case SEQM_REMOVE_TEMPO_RANGE:
		{
			//printf("processMsg (SEQM_REMOVE_TEMPO) UndoOp::DeleteTempo. adding tempo at: %d with tempo=%d\n", msg->a, msg->b);
			QList<void*> list = msg->objectList;
			if(!list.isEmpty())
			{
				TEvent* se = (TEvent*)list.front();
				TEvent* ee = (TEvent*)list.back();
				for(int i = 0; i < list.size(); i++)
				{
					TEvent* ev = (TEvent*)list.at(i);
					undoOp(UndoOp::DeleteTempo, ev->tick, ev->tempo);
				}
				tempomap.delTempoRange(se->tick, ee->tick);
				updateFlags = SC_TEMPO;
			}
			break;
		}
		case SEQM_ADD_SIG:
			undoOp(UndoOp::AddSig, msg->a, msg->b, msg->c);
			AL::sigmap.add(msg->a, AL::TimeSignature(msg->b, msg->c));
			updateFlags = SC_SIG;
			break;

		case SEQM_REMOVE_SIG:
			undoOp(UndoOp::DeleteSig, msg->a, msg->b, msg->c);
			AL::sigmap.del(msg->a);
			updateFlags = SC_SIG;
			break;

		default:
			printf("unknown seq message %d\n", msg->id);
			break;
	}
}

//---------------------------------------------------------
//   cmdAddPart
//---------------------------------------------------------

void Song::cmdAddPart(Part* part)
{
	addPart(part);
	undoOp(UndoOp::AddPart, part);
	updateFlags = SC_PART_INSERTED;
}

//---------------------------------------------------------
//   cmdRemovePart
//---------------------------------------------------------

void Song::cmdRemovePart(Part* part)
{
	removePart(part);
	undoOp(UndoOp::DeletePart, part);
	part->events()->incARef(-1);
	unchainClone(part);
	updateFlags = SC_PART_REMOVED;
}

//---------------------------------------------------------
//   cmdChangePart
//---------------------------------------------------------

void Song::cmdChangePart(Part* oldPart, Part* newPart, bool doCtrls, bool doClones)
{
	//printf("Song::cmdChangePart before changePart oldPart:%p events:%p refs:%d Arefs:%d sn:%d newPart:%p events:%p refs:%d Arefs:%d sn:%d\n", oldPart, oldPart->events(), oldPart->events()->refCount(), oldPart->events()->arefCount(), oldPart->sn(), newPart, newPart->events(), newPart->events()->refCount(), newPart->events()->arefCount(), newPart->sn());

	if (doCtrls)
		removePortCtrlEvents(oldPart, doClones);

	changePart(oldPart, newPart);

	undoOp(UndoOp::ModifyPart, oldPart, newPart, doCtrls, doClones);

	// Changed by T356. Do not decrement ref count if the new part is a clone of the old part, since the event list
	//  will still be active.
	if (oldPart->cevents() != newPart->cevents())
		oldPart->events()->incARef(-1);

	//printf("Song::cmdChangePart before repl/unchClone oldPart:%p events:%p refs:%d Arefs:%d sn:%d newPart:%p events:%p refs:%d Arefs:%d sn:%d\n", oldPart, oldPart->events(), oldPart->events()->refCount(), oldPart->events()->arefCount(), oldPart->sn(), newPart, newPart->events(), newPart->events()->refCount(), newPart->events()->arefCount(), newPart->sn());

	replaceClone(oldPart, newPart);

	if (doCtrls)
		addPortCtrlEvents(newPart, doClones);

	//printf("Song::cmdChangePart after repl/unchClone oldPart:%p events:%p refs:%d Arefs:%d sn:%d newPart:%p events:%p refs:%d Arefs:%d sn:%d\n", oldPart, oldPart->events(), oldPart->events()->refCount(), oldPart->events()->arefCount(), oldPart->sn(), newPart, newPart->events(), newPart->events()->refCount(), newPart->events()->arefCount(), newPart->sn());

	updateFlags = SC_PART_MODIFIED;
	//update(updateFlags);
}

//---------------------------------------------------------
//   panic
//---------------------------------------------------------

void Song::panic()
{
	audio->msgPanic();
}

//---------------------------------------------------------
//   clear
//    signal - emit signals for changes if true
//    called from constructor as clear(false) and
//    from OOMidi::clearSong() as clear(false)
//---------------------------------------------------------

void Song::clear(bool signal)
{
	if (debugMsg)
		printf("Song::clear\n");

	bounceTrack = 0;
	m_masterId = 0;
	m_oomVerbId = 0;

	m_tracks.clear();
	m_trackIndex.clear();
	m_composerTracks.clear();
	m_trackViewIndex.clear();
	m_composerTrackIndex.clear();
	_tviews.clear();
	_tracks.clear();
	_artracks.clear();
	_viewtracks.clear();
	_midis.clearDelete();
	_waves.clearDelete();
	_inputs.clearDelete(); // audio input ports
	_outputs.clearDelete(); // audio output ports
	_groups.clearDelete(); // mixer groups
	_auxs.clearDelete(); // aux sends

	//Clear all midi port devices.
	for (int i = 0; i < MIDI_PORTS; ++i)
	{
		//Since midi ports are not deleted, clear all midi port in/out routes. They point to non-existant tracks now.
		midiPorts[i].inRoutes()->clear();
		midiPorts[i].outRoutes()->clear();

		//Clear out the patch sequences between song load
		//This causes a crash right now only when the PR is open while changing songs
		//Its because all the components of PR like trackinfo are not reacting properly
		//I think the solution is to close the PR before changing songs, what's the point
		//of having it there between completely different songs.
		midiPorts[i].patchSequences()->clear();

		//Reset this.
		midiPorts[i].setFoundInSongFile(false);

		// This will also close the device.
		midiPorts[i].setMidiDevice(0);
	}

	// Make sure to delete Jack midi devices, and remove all ALSA midi device routes...
	QList<MidiDevice*> deleteList;
	for (iMidiDevice imd = midiDevices.begin(); imd != midiDevices.end(); ++imd)
	{
		MidiDevice* md = (MidiDevice*)*imd;
		if(md)
		{
			int type = md->deviceType();
			switch(type)
			{
				case MidiDevice::SYNTH_MIDI:
				{//Properly handle synths
					deleteList.append(md);
				}
				break;
				case MidiDevice::JACK_MIDI:
				{
					deleteList.append(md);
				}
				break;
				case MidiDevice::ALSA_MIDI:
				{
					// With alsa devices, we must not delete them (they're always in the list). But we must
					//  clear all routes. They point to non-existant midi tracks, which were all deleted above.
					(*imd)->inRoutes()->clear();
					(*imd)->outRoutes()->clear();
				}
				break;
			}
		}
	}

	//Now safely delete them from the device list
	foreach(MidiDevice* md, deleteList)
	{
		int type = md->deviceType();
		switch(type)
		{
			case MidiDevice::SYNTH_MIDI:
			{//Properly handle synths
				SynthPluginDevice* synth = (SynthPluginDevice*)md;
				if (synth && synth->duplicated())
				{
				    midiDevices.remove(md);
				    synth->close();
				    //delete synth;
				}
			}
			break;
			case MidiDevice::JACK_MIDI:
			{
				// Remove the device from the list.
				midiDevices.remove(md);
				// Since Jack midi devices are created dynamically, we must delete them.
				// The destructor unregisters the device from Jack, which also disconnects all device-to-jack routes.
				// This will also delete all midi-track-to-device routes, they point to non-existant midi tracks
				//  which were all deleted above
				delete md;
			}
			break;
			default://ALSA already handled
			break;
		}
	}

	tempomap.clear();
	AL::sigmap.clear();
	undoList->clearDelete();
	redoList->clear();
	m_undoStack->clear();
	_markerList->clear();
	pos[0].setTick(0);
	pos[1].setTick(0);
	pos[2].setTick(0);
	_vcpos.setTick(0);

	Track::clearSoloRefCounts();
	clearMidiTransforms();
	clearMidiInputTransforms();

	// Clear all midi port controller values.
	for (int i = 0; i < MIDI_PORTS; ++i)
		// Don't remove the controllers, just the values.
		midiPorts[i].controller()->clearDelete(false);

	_masterFlag = true;
	loopFlag = false;
	loopFlag = false;
	punchinFlag = false;
	punchoutFlag = false;
	recordFlag = false;
	soloFlag = false;
	// seq
	_mtype = MT_UNKNOWN;
	_recMode = REC_OVERDUP;
	_cycleMode = CYCLE_NORMAL;
	_click = false;
	_quantize = false;
	_len = 405504; // song len in ticks
	_follow = JUMP;
	// _tempo      = 500000;      // default tempo 120
	dirty = false;
	initDrumMap();
	if (signal)
	{
		emit loopChanged(false);
		recordChanged(false);
	}
}

//---------------------------------------------------------
//   cleanupForQuit
//   called from OOMidi::closeEvent
//---------------------------------------------------------

void Song::cleanupForQuit()
{
	bounceTrack = 0;
	invalid = true;

	if (debugMsg)
		printf("OOMidi: Song::cleanupForQuit...\n");

	m_tracks.clear();
	m_trackIndex.clear();
	m_composerTracks.clear();
	m_trackViewIndex.clear();
	m_composerTrackIndex.clear();
	_tracks.clear();
	_artracks.clear();
	_viewtracks.clear();

	if (debugMsg)
		printf("deleting _midis\n");
	_midis.clearDelete();

	if (debugMsg)
		printf("deleting _waves\n");
	_waves.clearDelete();

	if (debugMsg)
		printf("deleting _inputs\n");
	_inputs.clearDelete(); // audio input ports

	if (debugMsg)
		printf("deleting _outputs\n");
	_outputs.clearDelete(); // audio output ports

	if (debugMsg)
		printf("deleting _groups\n");
	_groups.clearDelete(); // mixer groups

	if (debugMsg)
		printf("deleting _auxs\n");
	_auxs.clearDelete(); // aux sends

    //if (debugMsg)
    //	printf("deleting _synthIs\n");
    //_synthIs.clearDelete(); // each ~SynthI() -> deactivate3() -> ~SynthIF()

	tempomap.clear();
	AL::sigmap.clear();

	if (debugMsg)
		printf("deleting undoList, clearing redoList\n");
	undoList->clearDelete();
	redoList->clear(); // Check this - Should we do a clearDelete? IIRC it was OK this way - no clearDelete in case of same items in both lists.

	_markerList->clear();

	_tviews.clear();

	if (debugMsg)
		printf("deleting transforms\n");
	clearMidiTransforms(); // Deletes stuff.
	clearMidiInputTransforms(); // Deletes stuff.

	if (debugMsg)
		printf("deleting midiport controllers\n");
	// Clear all midi port controllers and values.
	for (int i = 0; i < MIDI_PORTS; ++i)
	{
		//Clear out the patch sequences 
		midiPorts[i].patchSequences()->clear();

		// Remove the controllers and the values.
		midiPorts[i].controller()->clearDelete(true);
	}

	// Can't do this here. Jack isn't running. Fixed. Test OK so far.
#if 1
	if (debugMsg)
		printf("deleting midi devices except synths\n");
	for (iMidiDevice imd = midiDevices.begin(); imd != midiDevices.end(); ++imd)
	{
		// Since Syntis are midi devices, there's no need to delete them below.
        if ((*imd)->isSynthPlugin())
			continue;
		delete (*imd);
	}
	midiDevices.clear(); // midi devices
#endif

	if (debugMsg)
		printf("deleting midi instruments\n");
	for (iMidiInstrument imi = midiInstruments.begin(); imi != midiInstruments.end(); ++imi)
		delete (*imi);
	midiInstruments.clear(); // midi devices

	// Nothing required for ladspa plugin list, and rack instances of them
	//  are handled by ~AudioTrack.

	invalid = true;
	if (debugMsg)
		printf("...finished cleaning up.\n");
}

void Song::addMasterTrack()
{
	if(!m_masterId)
	{//Create the default oom verb aux track if it dont exist, no undo
		Track* t = addTrackByName("Master", Track::AUDIO_OUTPUT, -1, false, false);
		if(t)
		{
			m_masterId = t->id();
			//Route master to system playback
			if(audioDevice && audioDevice->deviceType() == MidiDevice::JACK_MIDI)
			{//TODO: Fix this when we support more than Jack and ALSA
				void* p = audioDevice->findPort("system:playback_1");
				if(p)
				{
					Route src((AudioTrack*)t, 0);
					Route dst(p, 0);
					audio->msgAddRoute(src, dst);
				}
				void* p2 = audioDevice->findPort("system:playback_2");
				if(p2)
				{
					Route src((AudioTrack*)t, 1);
					Route dst(p2, 1);
					audio->msgAddRoute(src, dst);
				}
			}
			else
			{//Do the ALSA stuff
			}
		}
	}
}

void Song::addOOMVerb()
{
	if(!m_oomVerbId)
	{//Create the default oom verb aux track if it dont exist, no undo
		Track* t = addTrackByName("OOStudio Verb", Track::AUDIO_AUX, -1, false, true);
		if(t)
		{
			m_oomVerbId = t->id();
		}
	}
}

void Song::playMonitorEvent(int fd)
{
	int size = sizeof(MonitorData);
	//char buffer[16];
	char buffer[size];

	//int n = ::read(fd, buffer, 16);
	int n = ::read(fd, buffer, size);
	if (n < 0)
	{
		printf("Song: playMonitorEvent(): READ PIPE failed: %s\n",
				strerror(errno));
		return;
	}
	processMonitorMessage(buffer);
}
void Song::processMonitorMessage(const void* m)
{
	MonitorData* mdata = (MonitorData*)m;
	if(mdata)
	{
		//FIXME: For NRPN Support this needs to take into account the Controller type
		switch(mdata->dataType)
		{
			case MIDI_INPUT:
			{
				MidiPlayEvent ev(0, mdata->port, mdata->channel, ME_CONTROLLER, mdata->controller, mdata->value, mdata->track);
				ev.setEventSource(MonitorSource);
				midiPorts[ev.port()].sendEvent(ev);
				return;
			}
			break;
			case MIDI_LEARN:
			{
				//Values are: Port, Channel, CC
				emit midiLearned(mdata->port, mdata->channel, mdata->controller);
			}
			break;
			case MIDI_LEARN_NRPN:
			{//The fifth param is just a padding
				//Values are: Port, Channel, MSB, LSB 
				emit midiLearned(mdata->port, mdata->channel, mdata->msb, mdata->lsb);
			}
			break;
		}
	}
}

//---------------------------------------------------------
//   seqSignal
//    sequencer message to GUI
//    execution environment: gui thread
//---------------------------------------------------------

void Song::seqSignal(int fd)/*{{{*/
{
	char buffer[16];

	int n = ::read(fd, buffer, 16);
	if (n < 0)
	{
		printf("Song: seqSignal(): READ PIPE failed: %s\n",
				strerror(errno));
		return;
	}
	for (int i = 0; i < n; ++i)
	{
		// printf("seqSignal to gui:<%c>\n", buffer[i]);
		switch (buffer[i])
		{
			case '0': // STOP
				stopRolling();
				break;
			case '1': // PLAY
				setStopPlay(true);
				break;
			case '2': // record
				setRecord(true);
				break;
			case '3': // START_PLAY + jack STOP
				abortRolling();
				break;
			case 'P': // alsa ports changed
				rescanAlsaPorts();
				break;
			case 'G':
				clearRecAutomation(true);
				setPos(0, audio->tickPos(), true, false, true);
				break;
			case 'S': // shutdown audio
				oom->seqStop();

			{
				// give the user a sensible explanation
				jackErrorBox = new QMessageBox(QMessageBox::Critical, tr("Jack shutdown!"),
				//int btn = QMessageBox::critical(oom, tr("Jack shutdown!"),
						tr("Jack has detected a performance problem which has lead to\n"
						"OOMidi being disconnected.\n"
						"This could happen due to a number of reasons:\n"
						"- a performance issue with your particular setup.\n"
						"- a bug in OOMidi (or possibly in another connected software).\n"
						"- a random hiccup which might never occur again.\n"
						"- jack was voluntary stopped by you or someone else\n"
						"- jack crashed\n"
						"If there is a persisting problem you are much welcome to discuss it\n"
						"on the OOMidi mailinglist.\n"
						"(there is information about joining the mailinglist on the OOMidi\n"
						" homepage which is available through the help menu)\n"
						"\n"
						"To proceed check the status of Jack and try to restart it and then .\n"
						"click \"Audio > Restart Audio\" menu."), QMessageBox::Close, oom);
				jackErrorBox->exec();
				/*if (btn == 0)
				{
					printf("restarting!\n");
					oom->seqRestart();
				}*/
			}

				break;
			case 'f': // start freewheel
				if (debugMsg)
					printf("Song: seqSignal: case f: setFreewheel start\n");

				// Enabled by Tim. p3.3.6
				if (config.freewheelMode)
					audioDevice->setFreewheel(true);

				break;

			case 'F': // stop freewheel
				if (debugMsg)
					printf("Song: seqSignal: case F: setFreewheel stop\n");

				// Enabled by Tim. p3.3.6
				if (config.freewheelMode)
					audioDevice->setFreewheel(false);

				audio->msgPlay(false);
#if 0
				if (record())
					audio->recordStop();
				setStopPlay(false);
#endif
				break;

			case 'C': // Graph changed
				if (audioDevice)
					audioDevice->graphChanged();
				break;

			case 'R': // Registration changed
				if (audioDevice)
					audioDevice->registrationChanged();
				break;

			case 'E':
			{
				update(SC_RACK);
			}
				break;
			default:
				printf("unknown Seq Signal <%c>\n", buffer[i]);
				break;
		}
	}
}/*}}}*/

void Song::closeJackBox()
{
	if(jackErrorBox)
	{
		jackErrorBox->close();
		jackErrorBox = 0;
	}
}

//---------------------------------------------------------
//   recordEvent
//---------------------------------------------------------

void Song::recordEvent(MidiTrack* mt, Event& event)/*{{{*/
{
	//---------------------------------------------------
	//    if tick points into a part,
	//          record to that part
	//    else
	//          create new part
	//---------------------------------------------------

	unsigned tick = event.tick();
	PartList* pl = mt->parts();
	MidiPart* part = 0;
	iPart ip;
	for (ip = pl->begin(); ip != pl->end(); ++ip)
	{
		part = (MidiPart*) (ip->second);
		unsigned partStart = part->tick();
		unsigned partEnd = partStart + part->lenTick();
		if (tick >= partStart && tick < partEnd)
			break;
	}
	updateFlags |= SC_EVENT_INSERTED;
	if (ip == pl->end())
	{
		// create new part
		part = new MidiPart(mt);
		int startTick = roundDownBar(tick);
		//int endTick   = roundUpBar(tick);
		int endTick = roundUpBar(tick + 1);
		part->setTick(startTick);
		part->setLenTick(endTick - startTick);
		part->setName(mt->name());
		event.move(-startTick);
		part->events()->add(event);
		audio->msgAddPart(part);
		return;
	}
	part = (MidiPart*) (ip->second);
	tick -= part->tick();
	event.setTick(tick);

	Event ev;
	if (event.type() == Controller)
	{
		EventRange range = part->events()->equal_range(tick);
		for (iEvent i = range.first; i != range.second; ++i)
		{
			ev = i->second;
			// At the moment, Song::recordEvent() is only called by the 'Rec' buttons in the
			//  midi track info panel. So only controller types are fed to it. If other event types
			//  are to be passed, we will have to expand on this to check if equal. Instead, maybe add an isEqual() to Event class.
			//if((ev.type() == Controller && event.type() == Controller || ev.type() == Controller && event.type() == Controller)
			//   && ev.dataA() == event.dataA() && ev.dataB() == event.dataB())
			if (ev.type() == Controller && ev.dataA() == event.dataA())
			{
				// Don't bother if already set.
				if (ev.dataB() == event.dataB())
					return;
				// Indicate do undo, and do port controller values and clone parts.
				audio->msgChangeEvent(ev, event, part, true, true, true);
				return;
			}
		}
	}

	// Indicate do undo, and do port controller values and clone parts.
	//audio->msgAddEvent(event, part);
	audio->msgAddEvent(event, part, true, true, true);
}/*}}}*/

void Song::recordEvent(MidiPart* part, Event& event)/*{{{*/
{
	//---------------------------------------------------
	//    if tick points into a part,
	//          record to that part
	//    else
	//          create new part
	//---------------------------------------------------

	unsigned tick = event.tick();
	int diff = event.endTick() - part->lenTick();
	if (diff > 0)
	{// too short part? extend it
		Part* newPart = part->clone();
		newPart->setLenTick(newPart->lenTick() + diff);
		// Indicate no undo, and do port controller values but not clone parts.
		audio->msgChangePart(part, newPart, false, true, false);
		updateFlags |= SC_PART_MODIFIED;
		part = (MidiPart*)newPart;
	}
	updateFlags |= SC_EVENT_INSERTED;

	tick -= part->tick();
	event.setTick(tick);

	Event ev;
	if (event.type() == Controller)
	{
		EventRange range = part->events()->equal_range(tick);
		for (iEvent i = range.first; i != range.second; ++i)
		{
			ev = i->second;
			// At the moment, Song::recordEvent() is only called by the 'Rec' buttons in the
			//  midi track info panel. So only controller types are fed to it. If other event types
			//  are to be passed, we will have to expand on this to check if equal. Instead, maybe add an isEqual() to Event class.
			if (ev.type() == Controller && ev.dataA() == event.dataA())
			{
				// Don't bother if already set.
				if (ev.dataB() == event.dataB())
					return;
				// Indicate do undo, and do port controller values and clone parts.
				audio->msgChangeEvent(ev, event, part, true, true, true);
				return;
			}
		}
	}

	// Indicate do undo, and do port controller values and clone parts.
	audio->msgAddEvent(event, part, true, true, true);
}/*}}}*/

//---------------------------------------------------------
//   execAutomationCtlPopup
//---------------------------------------------------------

int Song::execAutomationCtlPopup(AudioTrack* track, const QPoint& menupos, int acid)
{
	enum
	{
		HEADER, PREV_EVENT, NEXT_EVENT, SEP2, ADD_EVENT, CLEAR_EVENT, CLEAR_RANGE, CLEAR_ALL_EVENTS
	};
	QMenu* menu = new QMenu;

	int count = 0;
	bool isEvent = false, canSeekPrev = false, canSeekNext = false, canEraseRange = false;
	bool canAdd = false;
	double ctlval = 0.0;
	if (track)
	{
		ciCtrlList icl = track->controller()->find(acid);
		if (icl != track->controller()->end())
		{
			CtrlList *cl = icl->second;
			canAdd = true;
			ctlval = cl->curVal();
			count = cl->size();
			if (count)
			{
				int frame = pos[0].frame();

				iCtrl s = cl->lower_bound(frame);
				iCtrl e = cl->upper_bound(frame);

				isEvent = (s != cl->end() && s->second.getFrame() == frame);

				canSeekPrev = s != cl->begin();
				canSeekNext = e != cl->end();

				s = cl->lower_bound(pos[1].frame());

				canEraseRange = s != cl->end()
						&& (int) pos[2].frame() > s->second.getFrame();
			}
		}
	}

	menu->addAction(new MenuTitleItem(tr("Automation:"), menu));

	QAction* prevEvent = menu->addAction(tr("previous event"));
	prevEvent->setData(PREV_EVENT);
	prevEvent->setEnabled(canSeekPrev);

	QAction* nextEvent = menu->addAction(tr("next event"));
	nextEvent->setData(NEXT_EVENT);
	nextEvent->setEnabled(canSeekNext);

	menu->addSeparator();

	QAction* addEvent = new QAction(menu);
	menu->addAction(addEvent);
	if (isEvent)
		addEvent->setText(tr("set event"));
	else
		addEvent->setText(tr("add event"));
	addEvent->setData(ADD_EVENT);
	addEvent->setEnabled(canAdd);

	QAction* eraseEventAction = menu->addAction(tr("erase event"));
	eraseEventAction->setData(CLEAR_EVENT);
	eraseEventAction->setEnabled(isEvent);

	QAction* eraseRangeAction = menu->addAction(tr("erase range"));
	eraseRangeAction->setData(CLEAR_RANGE);
	eraseRangeAction->setEnabled(canEraseRange);

	QAction* clearAction = menu->addAction(tr("clear automation"));
	clearAction->setData(CLEAR_ALL_EVENTS);
	clearAction->setEnabled((bool)count);

	QAction* act = menu->exec(menupos);
	if (!act || !track)
	{
		delete menu;
		return -1;
	}

	int sel = act->data().toInt();
	delete menu;

	switch (sel)
	{
		case ADD_EVENT:
			audio->msgAddACEvent(track, acid, pos[0].frame(), ctlval);
			break;
		case CLEAR_EVENT:
			audio->msgEraseACEvent(track, acid, pos[0].frame());
			break;

		case CLEAR_RANGE:
			audio->msgEraseRangeACEvents(track, acid, pos[1].frame(), pos[2].frame());
			break;

		case CLEAR_ALL_EVENTS:
			if (QMessageBox::question(oom, QString("OOMidi"),
					tr("Clear all controller events?"), tr("&Ok"), tr("&Cancel"),
					QString::null, 0, 1) == 0)
				audio->msgClearControllerEvents(track, acid);
			break;

		case PREV_EVENT:
			audio->msgSeekPrevACEvent(track, acid);
			break;

		case NEXT_EVENT:
			audio->msgSeekNextACEvent(track, acid);
			break;

		default:
			return -1;
			break;
	}

	return sel;
}

//---------------------------------------------------------
//   execMidiAutomationCtlPopup
//---------------------------------------------------------

int Song::execMidiAutomationCtlPopup(MidiTrack* track, MidiPart* part, const QPoint& menupos, int ctlnum)
{
	if (!track && !part)
		return -1;

	enum
	{
		HEADER, ADD_EVENT, CLEAR_EVENT
	};
	QMenu* menu = new QMenu;

	bool isEvent = false;

	MidiTrack* mt;
	if (track)
		mt = track;
	else
		mt = (MidiTrack*) part->track();
	int portno = mt->outPort();
	int channel = mt->outChannel();
	MidiPort* mp = &midiPorts[portno];

	int dctl = ctlnum;
	// Is it a drum controller, according to the track port's instrument?
	MidiController *mc = mp->drumController(ctlnum);
	if (mc)
	{
		// Change the controller event's index into the drum map to an instrument note.
		int note = ctlnum & 0x7f;
		dctl &= ~0xff;
		channel = drumMap[note].channel;
		mp = &midiPorts[drumMap[note].port];
		dctl |= drumMap[note].anote;
	}

	//printf("Song::execMidiAutomationCtlPopup ctlnum:%d dctl:%d anote:%d\n", ctlnum, dctl, drumMap[ctlnum & 0x7f].anote);

	unsigned tick = cpos();

	if (!part)
	{
		PartList* pl = mt->parts();
		iPart ip;
		for (ip = pl->begin(); ip != pl->end(); ++ip)
		{
			MidiPart* tpart = (MidiPart*) (ip->second);
			unsigned partStart = tpart->tick();
			unsigned partEnd = partStart + tpart->lenTick();
			if (tick >= partStart && tick < partEnd)
			{
				// Prefer a selected part, otherwise keep looking...
				if (tpart->selected())
				{
					part = tpart;
					break;
				}
				else
					// Remember the first part found...
					if (!part)
					part = tpart;
			}
		}
	}

	Event ev;
	if (part)
	{
		unsigned partStart = part->tick();
		unsigned partEnd = partStart + part->lenTick();
		if (tick >= partStart && tick < partEnd)
		{
			EventRange range = part->events()->equal_range(tick - partStart);
			for (iEvent i = range.first; i != range.second; ++i)
			{
				ev = i->second;
				if (ev.type() == Controller)
				{
					//printf("Song::execMidiAutomationCtlPopup ev.dataA:%d\n", ev.dataA());
					if (ev.dataA() == ctlnum)
					{
						isEvent = true;
						break;
					}
				}
			}
		}
	}

	QAction* addEvent = new QAction(menu);
	menu->addAction(addEvent);
	if (isEvent)
		addEvent->setText(tr("set event"));
	else
		addEvent->setText(tr("add event"));
	addEvent->setData(ADD_EVENT);
	addEvent->setEnabled(true);

	QAction* eraseEventAction = menu->addAction(tr("erase event"));
	eraseEventAction->setData(CLEAR_EVENT);
	eraseEventAction->setEnabled(isEvent);

	QAction* act = menu->exec(menupos);
	if (!act)
	{
		delete menu;
		return -1;
	}

	int sel = act->data().toInt();
	delete menu;

	switch (sel)
	{
		case ADD_EVENT:
		{
			//int val = mp->hwCtrlState(channel, ctlnum);
			int val = mp->hwCtrlState(channel, dctl);
			if (val == CTRL_VAL_UNKNOWN)
				return -1;
			Event e(Controller);
			//e.setA(dctl);
			e.setA(ctlnum);
			e.setB(val);
			// Do we replace an old event?
			if (isEvent)
			{
				// Don't bother if already set.
				if (ev.dataB() == val)
					return -1;

				e.setTick(tick - part->tick());
				// Indicate do undo, and do port controller values and clone parts.
				audio->msgChangeEvent(ev, e, part, true, true, true);
			}
			else
			{
				// Store a new event...
				if (part)
				{
					e.setTick(tick - part->tick());
					// Indicate do undo, and do port controller values and clone parts.
					audio->msgAddEvent(e, part, true, true, true);
				}
				else
				{
					// Create a new part...
					part = new MidiPart(mt);
					int startTick = roundDownBar(tick);
					int endTick = roundUpBar(tick + 1);
					part->setTick(startTick);
					part->setLenTick(endTick - startTick);
					part->setName(mt->name());
					e.setTick(tick - startTick);
					part->events()->add(e);
					// Allow undo.
					audio->msgAddPart(part);
				}
			}
		}
			break;
		case CLEAR_EVENT:
			// Indicate do undo, and do port controller values and clone parts.
			audio->msgDeleteEvent(ev, part, true, true, true);
			break;

		default:
			return -1;
			break;
	}

	return sel;
}

//---------------------------------------------------------
//   updateSoloStates
//    This will properly set all soloing variables (including other tracks) based entirely
//     on the current values of all the tracks' _solo members.
//---------------------------------------------------------

void Song::updateSoloStates()
{
	Track::clearSoloRefCounts();
	for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
		(*i)->setInternalSolo(0);
	for (ciTrack i = _tracks.begin(); i != _tracks.end(); ++i)
		(*i)->updateSoloStates(true);
}

//---------------------------------------------------------
//   clearRecAutomation
//---------------------------------------------------------

void Song::clearRecAutomation(bool clearList)
{
	// Clear all pan/vol pressed and touched flags, and all rec event lists, if needed.
	for (iTrack it = tracks()->begin(); it != tracks()->end(); ++it)
		((Track*) (*it))->clearRecAutomation(clearList);
}

//---------------------------------------------------------
//   processAutomationEvents
//---------------------------------------------------------

void Song::processAutomationEvents()
{
	// Just clear all pressed and touched flags, not rec event lists.
	clearRecAutomation(false);
	if (!automation)
		return;
	for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
	{
		if (!(*i)->isMidiTrack())
			// Process (and clear) rec events.
			((AudioTrack*) (*i))->processAutomationEvents();
	}
}

//---------------------------------------------------------
//   abortRolling
//---------------------------------------------------------

void Song::abortRolling()
{
	if (record())
		audio->recordStop();
	setStopPlay(false);
}

//---------------------------------------------------------
//   stopRolling
//---------------------------------------------------------

void Song::stopRolling()
{
	abortRolling();
	processAutomationEvents();
}

//---------------------------------------------------------
//   connectJackRoutes
//---------------------------------------------------------

void Song::connectJackRoutes(AudioTrack* track, bool disconnect)
{
	switch (track->type())
	{
		case Track::AUDIO_OUTPUT:
		{
			AudioOutput* ao = (AudioOutput*) track;
			// This will re-register the track's jack ports.
			if (!disconnect)
				ao->setName(ao->name());
			// Now reconnect the output routes.
			if (checkAudioDevice() && audio->isRunning())
			{
				for (int ch = 0; ch < ao->channels(); ++ch)
				{
					RouteList* ir = ao->outRoutes();
					for (iRoute ii = ir->begin(); ii != ir->end(); ++ii)
					{
						Route r = *ii;
						if ((r.type == Route::JACK_ROUTE) && (r.channel == ch))
						{
							if (disconnect)
								audioDevice->disconnect(ao->jackPort(ch), r.jackPort);
							else
								audioDevice->connect(ao->jackPort(ch), r.jackPort);
							break;
						}
					}
					if (disconnect)
					{
						audioDevice->unregisterPort(ao->jackPort(ch));
						ao->setJackPort(ch, 0);
					}
				}
			}
		}
			break;
		case Track::AUDIO_INPUT:
		{
			AudioInput* ai = (AudioInput*) track;
			// This will re-register the track's jack ports.
			if (!disconnect)
				ai->setName(ai->name());
			// Now reconnect the input routes.
			if (checkAudioDevice() && audio->isRunning())
			{
				for (int ch = 0; ch < ai->channels(); ++ch)
				{
					RouteList* ir = ai->inRoutes();
					for (iRoute ii = ir->begin(); ii != ir->end(); ++ii)
					{
						Route r = *ii;
						if ((r.type == Route::JACK_ROUTE) && (r.channel == ch))
						{
							if (disconnect)
								audioDevice->disconnect(r.jackPort, ai->jackPort(ch));
							else
								audioDevice->connect(r.jackPort, ai->jackPort(ch));
							break;
						}
					}
					if (disconnect)
					{
						audioDevice->unregisterPort(ai->jackPort(ch));
						ai->setJackPort(ch, 0);
					}
				}
			}
		}
			break;
		default:
			break;
	}
}

//---------------------------------------------------------
// insertTrackView
//    add a new trackview for the Composer
//---------------------------------------------------------

void Song::insertTrackView(TrackView* tv, int idx)
{
	_tviews[tv->id()] = tv;
	m_trackViewIndex.insert(idx, tv->id());
	//TODO: This should not be called update(SC_VIEW_ADDED) instead
	updateTrackViews();
}

//---------------------------------------------------------
//   cmdRemoveTrackView
//---------------------------------------------------------

void Song::cmdRemoveTrackView(qint64 id)
{
	removeTrackView(id);
	updateFlags |= SC_TRACKVIEW_REMOVED;
}

//---------------------------------------------------------
// removeTrackView
//    add a new trackview for the Composer
//---------------------------------------------------------

void Song::removeTrackView(qint64 id)
{
	_tviews.erase(_tviews.find(id));
	m_trackViewIndex.removeAll(id);
	update(SC_TRACKVIEW_REMOVED);
	updateTrackViews();
}

//---------------------------------------------------------
// addNewTrackView
//    add a new trackview for the Composer
//---------------------------------------------------------

TrackView* Song::addNewTrackView()
{
	TrackView* tv = addTrackView();
	return tv;
}

//---------------------------------------------------------
//    addTrackView
//    called from GUI context
//---------------------------------------------------------

TrackView* Song::addTrackView()/*{{{*/
{
	TrackView* tv = new TrackView();
	tv->setDefaultName();
	_tviews[tv->id()] = tv;
	m_trackViewIndex.append(tv->id());
	//msgInsertTrackView(tv, -1, true);

	return tv;
}/*}}}*/

//---------------------------------------------------------
//   findTrackView
//    find track view by name
//---------------------------------------------------------

TrackView* Song::findTrackViewById(qint64 id) const
{
	if(_tviews.contains(id))
		return _tviews.value(id);
	return 0;
}

//---------------------------------------------------------
//   findTrackViewById
//    find internal trackview by id
//---------------------------------------------------------

TrackView* Song::findAutoTrackViewById(qint64 id) const
{
	if(_autotviews.contains(id))
		return _autotviews.value(id);
	return 0;
}

//---------------------------------------------------------
//   findAutoTrackView
//    find track view by name
//---------------------------------------------------------

TrackView* Song::findAutoTrackView(const QString& name) const
{
	QHashIterator<qint64, TrackView*> iter(_autotviews);
	while(iter.hasNext())
	{	
		iter.next();
		if (iter.value()->viewName() == name)
			return iter.value();
	}
	return 0;
}


//---------------------------------------------------------
//   findTrackViewByTrackId
//    find track view by track ID
//---------------------------------------------------------

TrackView* Song::findTrackViewByTrackId(qint64 tid)
{
	QHashIterator<qint64, TrackView*> iter(_tviews);
	while(iter.hasNext())
	{	
		iter.next();
		QList<qint64> *list = iter.value()->trackIndexList();
		if(list->contains(tid))
			return iter.value();
	}
	return 0;
}

//---------------------------------------------------------
// updateTrackViews
//---------------------------------------------------------
void Song::updateTrackViews()
{
	//printf("Song::updateTrackViews()\n");
	_viewtracks.clear();
	m_trackIndex.clear();
	m_composerTracks.clear();
	m_viewTracks.clear();
	//Create omnipresent Master track at top of all list.
	Track* master = findTrack("Master");
	if(master)
	{
		_viewtracks.push_back(master);
		m_composerTracks[master->id()] = master;
		m_viewTracks[master->id()] = master;
		m_trackIndex.insert(0, master->id());
	}
	viewselected = false;
	bool customview = false;
	bool workview = false;
	bool commentview = false;
	TrackView* wv = _autotviews.value(m_workingViewId);
	TrackView* cv = _autotviews.value(m_commentViewId);
	if(wv && wv->selected())
	{
		workview = true;
		viewselected = true;
	}
	if(cv && cv->selected())
	{
		commentview = true;
		viewselected = true;
	}
	foreach(qint64 tvid, m_trackViewIndex)
	{
		TrackView* it = _tviews.value(tvid);
		if(it && it->selected())
		{
			//qDebug("Song::updateTrackViews Found TrackView %s", it->viewName().toUtf8().constData());
			customview = true;
			viewselected = true;
			QList<qint64> *tlist = it->trackIndexList();
			QMap<qint64, TrackView::TrackViewTrack*> *tvlist = it->tracks();
			for(int c = 0; c < tlist->size(); ++c)
			{
				qint64 tid = tlist->at(c);
				if(tid == m_masterId)
					continue;
				TrackView::TrackViewTrack *tvt = tvlist->value(tid);
				if(tvt)
				{
					//qDebug("Song::updateTrackViews found TrackView::TrackViewTrack for view ~~~~~~~~~~~%lld", tid);
					if(tvt->is_virtual)
					{//Do nothing here, this is handled in TrackViewDock::updateTrackView
						//qDebug("Song::updateTrackViews found virtual track skipping here ~~~~~~~~~~~%lld", tvt->id);
						continue;
					}
					else
					{
						Track *t = findTrackById(tid);
						if(t)
						{
							//qDebug("Song::updateTrackViews found track in song ~~~~~~~~~~~ %s", t->name().toUtf8().constData());
							bool found = false;
							t->setSelected(false);
							if(workview && t->parts()->empty()) {
								continue;
							}
							//printf("Adding track to view %s\n", (*t)->name().toStdString().c_str());
							if(m_viewTracks.contains(tid))
							{
								found = true;
								//Make sure to record arm the ones that were in other views as well
								t->setRecordFlag1(it->record());
								t->setRecordFlag2(it->record());
							}
							if(!found)
							{
								//qDebug("Song::updateTrackViews adding track view ~~~~~~~~~~~ %s", t->name().toUtf8().constData());
								_viewtracks.push_back(t);
								m_viewTracks[tid] = t;
								t->setRecordFlag1(it->record());
								t->setRecordFlag2(it->record());
								m_trackIndex.append(it->id());
							}
						}
					}
				}
			}
		}
	}
	
	foreach(qint64 tvid, m_autoTrackViewIndex)
	{
		TrackView* ait = _autotviews.value(tvid);
		if(customview && (ait && ait->id() == m_workingViewId))
		{
			//qDebug("Song::updateTrackViews: skipping working View");
			continue;
		}
		if(ait && ait->selected())/*{{{*/
		{
			if(debugMsg)
				qDebug("Song::updateTrackViews: Selected View: %s", ait->viewName().toUtf8().constData());
			viewselected = true;
			QList<qint64> *tlist = ait->trackIndexList();
			QMap<qint64, TrackView::TrackViewTrack*> *tvlist = ait->tracks();
			for(int c = 0; c < tlist->size(); ++c)
			{
				qint64 i = tlist->at(c);
				TrackView::TrackViewTrack *tvt = tvlist->value(i);
				if(tvt)
				{//There will never be virtual tracks in these views
					Track* t = m_tracks.value(tvt->id);
					if(t)
					{
						t->setSelected(false);
						if(t == master)
							continue;
						if(!m_viewTracks.contains(t->id()))
						{
							if((t->type() == Track::MIDI || t->type() == Track::WAVE) && workview && t->parts()->empty())
								continue;
							if(t->comment().isEmpty() && commentview)
								continue;
							_viewtracks.push_back(t);
							m_viewTracks[t->id()] = t;
							m_trackIndex.append(t->id());
						}
					}
				}
			}
		}/*}}}*/
	}
	
	if(!viewselected)
	{
		//Make the viewtracks the artracks
		for(ciTrack it = _artracks.begin(); it != _artracks.end(); ++it)
		{
			_viewtracks.push_back((*it));
			m_viewTracks[(*it)->id()] = (*it);
			m_trackIndex.append((*it)->id());
		}
	}
	//printf("Song::updateTrackViews() took %f seconds to run\n", end-start);
	disarmAllTracks();
	if(!invalid)
		emit composerViewChanged();
}

//---------------------------------------------------------
//   insertTrack
//---------------------------------------------------------

void Song::insertTrack(Track* track, int idx)
{
	insertTrack1(track, idx);
	insertTrackRealtime(track, idx); // audio->msgInsertTrack(track, idx, false);
}

//---------------------------------------------------------
//   insertTrack1
//    non realtime part of insertTrack
//---------------------------------------------------------

void Song::insertTrack1(Track* track, int /*idx*/)
{
	//printf("Song::insertTrack1 track:%lx\n", track);

	switch (track->type())
	{
		case Track::AUDIO_SOFTSYNTH:
		{
            //SynthI* s = (SynthI*) track;
            //Synth* sy = s->synth();
            //if (!s->isActivated())
            //{
            //	s->initInstance(sy, s->name());
            //}
		}
			break;
		default:
			break;
	}

	//printf("Song::insertTrack1 end of function\n");

}

//---------------------------------------------------------
//   insertTrackRealtime
//    realtime part
//---------------------------------------------------------

void Song::insertTrackRealtime(Track* track, int idx)
{
	//printf("Song::insertTrackRealtime track:%lx\n", track);

	int n;
	iTrack ia;
	switch (track->type())
	{
		case Track::MIDI:
		case Track::DRUM:
			_midis.push_back((MidiTrack*) track);
			ia = _artracks.index2iterator(idx);
			_artracks.insert(ia, track);
			addPortCtrlEvents(((MidiTrack*) track));
			m_composerTracks[track->id()] = track;
			m_composerTrackIndex.insert(idx, track->id());
			_autotviews.value(m_workingViewId)->addTrack(track->id());

			break;
		case Track::WAVE:
			_waves.push_back((WaveTrack*) track);
			ia = _artracks.index2iterator(idx);
			_artracks.insert(ia, track);
			m_composerTracks[track->id()] = track;
			m_composerTrackIndex.insert(idx, track->id());
			_autotviews.value(m_workingViewId)->addTrack(track->id());
			break;
		case Track::AUDIO_OUTPUT:
			_outputs.push_back((AudioOutput*) track);
			// set default master & monitor if not defined
			if (audio->audioMaster() == 0)
				audio->setMaster((AudioOutput*) track);
			if (audio->audioMonitor() == 0)
				audio->setMonitor((AudioOutput*) track);
			_autotviews.value(m_outputViewId)->addTrack(track->id());
			break;
		case Track::AUDIO_BUSS:
			_groups.push_back((AudioBuss*) track);
			_autotviews.value(m_bussViewId)->addTrack(track->id());
			break;
		case Track::AUDIO_AUX:
			_auxs.push_back((AudioAux*) track);
			_autotviews.value(m_auxViewId)->addTrack(track->id());
			break;
		case Track::AUDIO_INPUT:
			_inputs.push_back((AudioInput*) track);
			_autotviews.value(m_inputViewId)->addTrack(track->id());
			break;
		default:
			fprintf(stderr, "unknown track type %d\n", track->type());
			// abort();
			return;
	}

	//
	// initialize missing aux send
	//
	iTrack i = _tracks.index2iterator(idx);
	//printf("Song::insertTrackRealtime inserting into _tracks...\n");

	_tracks.insert(i, track);
	m_tracks[track->id()] = track;
	m_trackIndex.insert(idx, track->id());
	_autotviews.value(m_commentViewId)->addTrack(track->id());
	//printf("Song::insertTrackRealtime inserted\n");

	n = _auxs.size();
	for (iTrack i = _tracks.begin(); i != _tracks.end(); ++i)
	{
		if ((*i)->isMidiTrack())
			continue;
		//WaveTrack* wt = (WaveTrack*) * i;
		AudioTrack* wt = (AudioTrack*) * i;
		if (wt->hasAuxSend())
		{
			wt->addAuxSend();
		}
	}

	//
	//  add routes
	//

	if (track->type() == Track::AUDIO_OUTPUT)
	{
		const RouteList* rl = track->inRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->outRoutes()->push_back(src);
		}
	}
	else if (track->type() == Track::AUDIO_INPUT)
	{
		const RouteList* rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->inRoutes()->push_back(src);
		}
	}
	else if (track->isMidiTrack())
	{
		const RouteList* rl = track->inRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			//printf("Song::insertTrackRealtime %s in route port:%d\n", track->name().toLatin1().constData(), r->midiPort);
			Route src(track, r->channel);
			midiPorts[r->midiPort].outRoutes()->push_back(src);
		}
		rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			//printf("Song::insertTrackRealtime %s out route port:%d\n", track->name().toLatin1().constData(), r->midiPort);
			Route src(track, r->channel);
			midiPorts[r->midiPort].inRoutes()->push_back(src);
		}
	}
	else
	{
		const RouteList* rl = track->inRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->outRoutes()->push_back(src);
		}
		rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->inRoutes()->push_back(src);
		}
	}

	//printf("Song::insertTrackRealtime end of function\n");

}

//---------------------------------------------------------
//   removeTrack
//---------------------------------------------------------

void Song::removeTrack(Track* track)
{
	removeTrack1(track);
	audio->msgRemoveTrack(track);
	//delete track;
	update(SC_TRACK_REMOVED);
}

//---------------------------------------------------------
//   removeTrack1
//    non realtime part of removeTrack
//---------------------------------------------------------

void Song::removeTrack1(Track* track)
{
	switch (track->type())
	{
		case Track::WAVE:
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
		case Track::AUDIO_BUSS:
		case Track::AUDIO_AUX:
		case Track::AUDIO_SOFTSYNTH:
			((AudioTrack*) track)->deleteAllEfxGuis();
			break;
		default:
			break;
	}

	switch (track->type())
	{
		case Track::AUDIO_OUTPUT:
		case Track::AUDIO_INPUT:
			connectJackRoutes((AudioTrack*) track, true);
			break;
		default:
			break;
	}
}

//---------------------------------------------------------
//   removeTrackRealtime
//    called from RT context
//---------------------------------------------------------

void Song::removeTrackRealtime(Track* track)
{
	//printf("Song::removeTrackRealtime track:%s\n", track->name().toLatin1().constData());
	midiMonitor->msgDeleteMonitoredTrack(track);

	switch (track->type())
	{
		case Track::MIDI:
		case Track::DRUM:
			removePortCtrlEvents(((MidiTrack*) track));
			unchainTrackParts(track, true);

			_midis.erase(track);
			_artracks.erase(track);
			m_composerTracks.erase(m_composerTracks.find(track->id()));
			_autotviews.value(m_workingViewId)->removeTrack(track->id());
			break;
		case Track::WAVE:
			unchainTrackParts(track, true);

			_waves.erase(track);
			_artracks.erase(track);
			m_composerTracks.erase(m_composerTracks.find(track->id()));
			_autotviews.value(m_workingViewId)->removeTrack(track->id());
			break;
		case Track::AUDIO_OUTPUT:
			_outputs.erase(track);
			_autotviews.value(m_outputViewId)->removeTrack(track->id());
			break;
		case Track::AUDIO_INPUT:
			_inputs.erase(track);
			_autotviews.value(m_inputViewId)->removeTrack(track->id());
			break;
		case Track::AUDIO_BUSS:
			_groups.erase(track);
			_autotviews.value(m_bussViewId)->removeTrack(track->id());
			break;
		case Track::AUDIO_AUX:
			_auxs.erase(track);
			_autotviews.value(m_auxViewId)->removeTrack(track->id());
			break;
		default:
			break;
	}
	_tracks.erase(track);
	m_tracks.erase(m_tracks.find(track->id()));
	m_trackIndex.removeAll(track->id());
	_autotviews.value(m_commentViewId)->removeTrack(track->id());
	TrackView* tv = findTrackViewByTrackId(track->id());
	bool updateview = false;
	while(tv)
	{
		updateview = true;
		tv->removeTrack(track->id());
		tv = findTrackViewByTrackId(track->id());
	}
	updateTrackViews();


	//
	//  remove routes
	//

	if (track->type() == Track::AUDIO_OUTPUT)
	{
		//qDebug("Song::removeTrackRealtime: ~~~~~~~~~~~~~~~~~~~~Removing route for output track");
		const RouteList* rl = track->inRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->outRoutes()->removeRoute(src);
		}
	}
	else if (track->type() == Track::AUDIO_INPUT)
	{
		//qDebug("Song::removeTrackRealtime: ~~~~~~~~~~~~~~~~~~~~Removing route for input track");
		const RouteList* rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->inRoutes()->removeRoute(src);
		}
	}
	else if (track->isMidiTrack())
	{
		//qDebug("Song::removeTrackRealtime: ~~~~~~~~~~~~~~~~~~~~Removing input routes for midi track");
		const RouteList* rl = track->inRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			//printf("Song::removeTrackRealtime %s in route port:%d\n", track->name().toLatin1().constData(), r->midiPort);
			Route src(track, r->channel);
			midiPorts[r->midiPort].outRoutes()->removeRoute(src);
		}
		//qDebug("Song::removeTrackRealtime: ~~~~~~~~~~~~~~~~~~~~Removing output routes for midi track");
		rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			//printf("Song::removeTrackRealtime %s out route port:%d\n", track->name().toLatin1().constData(), r->midiPort);  
			Route src(track, r->channel);
			midiPorts[r->midiPort].inRoutes()->removeRoute(src);
		}
	}
	else
	{
		//qDebug("Song::removeTrackRealtime: ~~~~~~~~~~~~~~~~~~~~Removing input route for %d track", track->type());
		const RouteList* rl = track->inRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			//printf("Song::removeTrackRealtime %s in route track:%s\n", track->name().toLatin1().constData(), r->track->name().toLatin1().constData()); 
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->outRoutes()->removeRoute(src);
		}
		//qDebug("Song::removeTrackRealtime: ~~~~~~~~~~~~~~~~~~~~Removing output route for %d track", track->type());
		rl = track->outRoutes();
		for (ciRoute r = rl->begin(); r != rl->end(); ++r)
		{
			//printf("Song::removeTrackRealtime %s out route track:%s\n", track->name().toLatin1().constData(), r->track->name().toLatin1().constData()); 
			Route src(track, r->channel, r->channels);
			src.remoteChannel = r->remoteChannel;
			r->track->inRoutes()->removeRoute(src);
		}
	}
}

//---------------------------------------------------------
//   executeScript
//---------------------------------------------------------

void Song::executeScript(const char* scriptfile, PartList* parts, int quant, bool onlyIfSelected)
{
	// a simple format for external processing
	// will be extended if there is a need
	//
	// Semantics:
	// PARTLEN <len in ticks>
	// BEATLEN <len in ticks>
	// QUANTLEN <len in ticks>
	// NOTE <tick> <nr> <len in ticks> <velocity>
	// CONTROLLER <tick> <a> <b> <c>
	//
	song->startUndo(); // undo this entire block
	for (iPart i = parts->begin(); i != parts->end(); i++)
	{
		//const char* tmp = tmpnam(NULL);
		char tmp[16] = "oom-tmp-XXXXXX";
		int fd = mkstemp(tmp);
		printf("script input filename=%s\n", tmp);
		//FILE *fp = fopen(tmp, "w");
		FILE *fp = fdopen(fd, "w");
		MidiPart *part = (MidiPart*) (i->second);
		int partStart = part->endTick() - part->lenTick();
		int z, n;
		AL::sigmap.timesig(0, z, n);
		fprintf(fp, "TIMESIG %d %d\n", z, n);
		fprintf(fp, "PART %d %d\n", partStart, part->lenTick());
		fprintf(fp, "BEATLEN %d\n", AL::sigmap.ticksBeat(0));
		fprintf(fp, "QUANTLEN %d\n", quant);

		//for (iCItem i = items.begin(); i != items.end(); ++i) {
		for (iEvent e = part->events()->begin(); e != part->events()->end(); e++)
		{
			Event ev = e->second;

			if (ev.isNote())
			{
				if (onlyIfSelected && ev.selected() == false)
					continue;

				fprintf(fp, "NOTE %d %d %d %d\n", ev.tick(), ev.dataA(), ev.lenTick(), ev.dataB());
				// Indicate no undo, and do not do port controller values and clone parts.
				audio->msgDeleteEvent(ev, part, false, false, false);
			}
			else if (ev.type() == Controller)
			{
				fprintf(fp, "CONTROLLER %d %d %d %d\n", ev.tick(), ev.dataA(), ev.dataB(), ev.dataC());
				// Indicate no undo, and do not do port controller values and clone parts.
				audio->msgDeleteEvent(ev, part, false, false, false);
			}
		}
		fclose(fp);

		// Call external program, let it manipulate the file
		int pid = fork();
		if (pid == 0)
		{
			if (execlp(scriptfile, scriptfile, tmp, NULL) == -1)
			{
				perror("Failed to launch script!");
				// Get out of here

				// cannot report error through gui, we are in another fork!
				//@!TODO: Handle unsuccessful attempts
				exit(99);
			}
			exit(0);
		}
		else if (pid == -1)
		{
			perror("fork failed");
		}
		else
		{
			int status;
			waitpid(pid, &status, 0);
			if (WEXITSTATUS(status) != 0)
			{
				QMessageBox::warning(oom, tr("OOMidi - external script failed"),
						tr("OOMidi was unable to launch the script\n")
						);
				endUndo(SC_EVENT_REMOVED);
				return;
			}
			else
			{ // d0 the fun55or5!
				// TODO: Create a new part, update the entire editor from it, hehh....

				QFile file(tmp);
				if (file.open(QIODevice::ReadOnly))
				{
					QTextStream stream(&file);
					QString line;
					while (!stream.atEnd())
					{
						line = stream.readLine(); // line of text excluding '\n'
						if (line.startsWith("NOTE"))
						{
							QStringList sl = line.split(" ");

							Event e(Note);
							int tick = sl[1].toInt();
							int pitch = sl[2].toInt();
							int len = sl[3].toInt();
							int velo = sl[4].toInt();
							printf("tick=%d pitch=%d velo=%d len=%d\n", tick, pitch, velo, len);
							e.setTick(tick);
							e.setPitch(pitch);
							e.setVelo(velo);
							e.setLenTick(len);
							// Indicate no undo, and do not do port controller values and clone parts.
							audio->msgAddEvent(e, part, false, false, false);
						}
						if (line.startsWith("CONTROLLER"))
						{
							QStringList sl = line.split(" ");

							Event e(Controller);
							int tick = sl[1].toInt();
							int a = sl[2].toInt();
							int b = sl[3].toInt();
							int c = sl[4].toInt();
							printf("tick=%d a=%d b=%d c=%d\n", tick, a, b, c);
							e.setA(a);
							e.setB(b);
							e.setB(c);
							// Indicate no undo, and do not do port controller values and clone parts.
							audio->msgAddEvent(e, part, false, false, false);
						}
					}
					file.close();
				}
			}
		}
		remove(tmp);
	}

	endUndo(SC_EVENT_REMOVED);
}

void Song::populateScriptMenu(QMenu* menuPlugins, QObject* receiver)
{
	//
	// List scripts
	//
	QString distScripts = oomGlobalShare + "/scripts";

	QString userScripts = configPath + "/scripts";

	QFileInfo distScriptsFi(distScripts);
	if (distScriptsFi.isDir())
	{
		QDir dir = QDir(distScripts);
		dir.setFilter(QDir::Executable | QDir::Files);
		deliveredScriptNames = dir.entryList();
	}
	QFileInfo userScriptsFi(userScripts);
	if (userScriptsFi.isDir())
	{
		QDir dir(userScripts);
		dir.setFilter(QDir::Executable | QDir::Files);
		userScriptNames = dir.entryList();
	}

	QSignalMapper* distSignalMapper = new QSignalMapper(this);
	QSignalMapper* userSignalMapper = new QSignalMapper(this);

	if (deliveredScriptNames.size() > 0 || userScriptNames.size() > 0)
	{
		//menuPlugins = new QPopupMenu(this);
		//menuBar()->insertItem(tr("&Plugins"), menuPlugins);
		int id = 0;
		if (deliveredScriptNames.size() > 0)
		{
			for (QStringList::Iterator it = deliveredScriptNames.begin(); it != deliveredScriptNames.end(); it++, id++)
			{
				//menuPlugins->insertItem(*it, this, SLOT(execDeliveredScript(int)), 0, id);
				//menuPlugins->insertItem(*it, this, slot_deliveredscripts, 0, id);
				QAction* act = menuPlugins->addAction(*it);
				connect(act, SIGNAL(triggered()), distSignalMapper, SLOT(map()));
				distSignalMapper->setMapping(act, id);
			}
			menuPlugins->addSeparator();
		}
		if (userScriptNames.size() > 0)
		{
			for (QStringList::Iterator it = userScriptNames.begin(); it != userScriptNames.end(); it++, id++)
			{
				//menuPlugins->insertItem(*it, this, slot_userscripts, 0, id);
				QAction* act = menuPlugins->addAction(*it);
				connect(act, SIGNAL(triggered()), userSignalMapper, SLOT(map()));
				userSignalMapper->setMapping(act, id);
			}
			menuPlugins->addSeparator();
		}
		connect(distSignalMapper, SIGNAL(mapped(int)), receiver, SLOT(execDeliveredScript(int)));
		connect(userSignalMapper, SIGNAL(mapped(int)), receiver, SLOT(execUserScript(int)));
	}
	return;
}

//---------------------------------------------------------
//   getScriptPath
//---------------------------------------------------------

QString Song::getScriptPath(int id, bool isdelivered)
{
	if (isdelivered)
	{
		QString path = oomGlobalShare + "/scripts/" + deliveredScriptNames[id];
		return path;
	}

	QString path = configPath + "/scripts/" + userScriptNames[id - deliveredScriptNames.size()];
	return path;
}


TrackList Song::getSelectedTracks()
{
	TrackList list;

	for (iTrack t = _viewtracks.begin(); t != _viewtracks.end(); ++t)
	{
		Track* tr = *t;
		if (tr->selected())
		{
			list.push_back(tr);
		}
	}
	
	return list;
}

void Song::setTrackHeights(TrackList &list, int height)
{
        for (iTrack t = list.begin(); t != list.end(); ++t)
        {
                Track* tr = *t;
                tr->setHeight(height);
        }

        song->update(SC_TRACK_MODIFIED);
}

void Song::toggleFeedback(bool f)
{	
	midiMonitor->msgToggleFeedback(f);
}

void Song::movePlaybackToPart(Part* part)/*{{{*/
{
	bool snap = tconfig().get_property("PerformerEdit", "snaptopart", true).toBool();
	if(audio->isPlaying() || !snap)
		return;
	if(part)
	{
		unsigned tick = part->tick();
		EventList* el = part->events();
		if(el->empty())
		{//move pb to part start
			Pos p(tick, true);
			song->setPos(0, p, true, true, true);
		}
		else
		{
			for(iEvent i = el->begin(); i != el->end(); ++i)
			{
				Event ev = i->second;
				if(ev.isNote())
				{
					Pos p(tick+ev.tick(), true);
					song->setPos(0, p, true, true, true);
					break;
				}
			}
		}
	}
}/*}}}*/

QList<Part*> Song::selectedParts()
{
	QList<Part*> selected;
	for (iTrack it = _viewtracks.begin(); it != _viewtracks.end(); ++it)
	{
		PartList* pl = (*it)->parts();
		for (iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			if (ip->second->selected())
			{
				selected.append(ip->second);
			}
		}
	}
	return selected;
}
