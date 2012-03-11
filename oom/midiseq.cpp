//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: midiseq.cpp,v 1.30.2.21 2009/12/20 05:00:35 terminator356 Exp $
//
//    high priority task for scheduling midi events
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <QMessageBox>

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <math.h>

#include "globals.h"
#include "midi.h"
#include "midiseq.h"
#include "midiport.h"
#include "mididev.h"
#include "midictrl.h"
#include "audio.h"
#include "plugin.h"
#include "driver/alsamidi.h"
#include "driver/jackmidi.h"
#include "sync.h"
#include "song.h"
#include "gconfig.h"

MidiSeq* midiSeq;
int MidiSeq::ticker = 0;
volatile bool midiBusy = false;


//---------------------------------------------------------
//   readMsg
//---------------------------------------------------------

static void readMsg(void* p, void*)
{

	MidiSeq* at = (MidiSeq*) p;
	at->readMsg();
}

//---------------------------------------------------------
//   processMsg
//---------------------------------------------------------

void MidiSeq::processMsg(const ThreadMsg* m)
{
	AudioMsg* msg = (AudioMsg*) m;
	switch (msg->id)
	{
		case SEQM_PRELOAD_PROGRAM:
			audio->preloadControllers();
			break;
		case SEQM_SEEK:
			processSeek();
			break;
		case MS_STOP:
			processStop();
			break;
		case MS_SET_RTC:
			doSetuid();
			setRtcTicks();
			undoSetuid();
			break;
		case MS_UPDATE_POLL_FD:
			updatePollFd();
			break;
		case SEQM_ADD_TRACK:
			song->insertTrackRealtime(msg->track, msg->ival);
			updatePollFd();
			break;
		case SEQM_REMOVE_TRACK:
			song->cmdRemoveTrack(msg->track);
			updatePollFd();
		case SEQM_REMOVE_TRACK_GROUP:
		{	
			for(int i = 0; i < msg->list.size(); i++)
			{
				Track* track  = song->findTrackById(msg->list.at(i));
				if(track && track->id() != song->masterId() && track->id() != song->oomVerbId())
					song->cmdRemoveTrack(track);
			}
			updatePollFd();
		}
		break;
			break;
		case SEQM_CHANGE_TRACK:
			song->changeTrack((Track*) (msg->p1), (Track*) (msg->p2));
			updatePollFd();
			break;
		case SEQM_ADD_PART:
			song->cmdAddPart((Part*) msg->p1);
			break;
		case SEQM_REMOVE_PART:
			song->cmdRemovePart((Part*) msg->p1);
			break;
		case SEQM_REMOVE_PART_LIST:
		{
			for(int i = 0; i < msg->plist.size(); i++)
			{
				song->cmdRemovePart((Part*) msg->plist.at(i));
			}
		}
		break;
		case SEQM_CHANGE_PART:
			song->cmdChangePart((Part*) msg->p1, (Part*) msg->p2, msg->a, msg->b);
			break;
		case SEQM_SET_TRACK_OUT_CHAN:
		{
			MidiTrack* track = (MidiTrack*) (msg->p1);
			track->setOutChanAndUpdate(msg->a);
		}
			break;
		case SEQM_SET_TRACK_OUT_PORT:
		{
			MidiTrack* track = (MidiTrack*) (msg->p1);
			track->setOutPortAndUpdate(msg->a);
		}
			break;
		case SEQM_REMAP_PORT_DRUM_CTL_EVS:
			song->remapPortDrumCtrlEvents(msg->ival, msg->a, msg->b, msg->c);
			break;
		case SEQM_CHANGE_ALL_PORT_DRUM_CTL_EVS:
			song->changeAllPortDrumCtrlEvents((bool)msg->a, (bool)msg->b);
			break;
		case SEQM_SET_MIDI_DEVICE:
			((MidiPort*) (msg->p1))->setMidiDevice((MidiDevice*) (msg->p2));
			updatePollFd();
			break;
		case SEQM_IDLE:
			idle = msg->a;
			break;
		default:
			printf("MidiSeq::processMsg() unknown id %d\n", msg->id);
			break;
	}
}

//---------------------------------------------------------
//   processStop
//---------------------------------------------------------

void MidiSeq::processStop()
{
	playStateExt = false; // not playing

	//
	//    stop stuck notes
	//
	for (iMidiDevice id = midiDevices.begin(); id != midiDevices.end(); ++id)
	{
		MidiDevice* md = *id;
		if (md->midiPort() == -1)
			continue;
		MPEventList* pel = md->playEvents();
		MPEventList* sel = md->stuckNotes();
		pel->clear();
		for (iMPEvent i = sel->begin(); i != sel->end(); ++i)
		{
			MidiPlayEvent ev = *i;
			ev.setTime(0);
			pel->add(ev);
		}
		sel->clear();
	}
}

//---------------------------------------------------------
//   processSeek
//---------------------------------------------------------

void MidiSeq::processSeek()
{
	int pos = audio->tickPos();
	if (pos == 0 && !song->record())
		audio->initDevices();

	//---------------------------------------------------
	//    set all controller
	//---------------------------------------------------

	for (iMidiDevice i = midiDevices.begin(); i != midiDevices.end(); ++i)
	{
		MidiDevice* dev = *i;
		int port = dev->midiPort();
		if (port == -1)
			continue;
		MidiPort* mp = &midiPorts[port];
		MidiCtrlValListList* cll = mp->controller();

		MPEventList* el = dev->playEvents();

		if (audio->isPlaying())
		{
			// stop all notes
			el->clear();
			MPEventList* sel = dev->stuckNotes();
			for (iMPEvent i = sel->begin(); i != sel->end(); ++i)
			{
				MidiPlayEvent ev = *i;
				ev.setTime(0);
				el->add(ev);
			}
			sel->clear();
		}

		for (iMidiCtrlValList ivl = cll->begin(); ivl != cll->end(); ++ivl)
		{
			MidiCtrlValList* vl = ivl->second;
			iMidiCtrlVal imcv = vl->iValue(pos);
			if (imcv != vl->end())
			{
				Part* p = imcv->second.part;
				unsigned t = (unsigned) imcv->first;
				// Do not add values that are outside of the part.
				if (p && t >= p->tick() && t < (p->tick() + p->lenTick()))
					el->add(MidiPlayEvent(0, port, ivl->first >> 24, ME_CONTROLLER, vl->num(), imcv->second.val, p->track()));
			}
		}
	}
}

//---------------------------------------------------------
//   MidiSeq
//---------------------------------------------------------

MidiSeq::MidiSeq(const char* name)
: Thread(name)
{
	prio = 0;

	idle = false;
	midiClock = 0;
	mclock1 = 0.0;
	mclock2 = 0.0;
	songtick1 = songtick2 = 0;
	lastTempo = 0;
	storedtimediffs = 0;
	playStateExt = false; // not playing
	doSetuid();
	timerFd = selectTimer();
	undoSetuid();

}

//---------------------------------------------------------
//   ~MidiSeq
//---------------------------------------------------------

MidiSeq::~MidiSeq()
{
	delete timer;
}

//---------------------------------------------------------
//   selectTimer()
//   select one of the supported timers to use during this run
//---------------------------------------------------------

signed int MidiSeq::selectTimer()
{
	int tmrFd;

	printf("Trying RTC timer...\n");
	timer = new RtcTimer();
	tmrFd = timer->initTimer();
	if (tmrFd != -1)
	{ // ok!
		printf("got timer = %d\n", tmrFd);
		return tmrFd;
	}
	delete timer;

	printf("Trying ALSA timer...\n");
	timer = new AlsaTimer();
	tmrFd = timer->initTimer();
	if (tmrFd != -1)
	{ // ok!
		printf("got timer = %d\n", tmrFd);
		return tmrFd;
	}
	delete timer;
	timer = NULL;
	QMessageBox::critical(0, /*tr*/(QString("Failed to start timer!")),
			/*tr*/(QString("No functional timer was available.\n"
			"RTC timer not available, check if /dev/rtc is available and readable by current user\n"
			"Alsa timer not available, check if module snd_timer is available and /dev/snd/timer is available")));
	printf("No functional timer available!!!\n");
	exit(1);
}

//---------------------------------------------------------
//   threadStart
//    called from loop()
//---------------------------------------------------------

void MidiSeq::threadStart(void*)
{
	int policy;
	if ((policy = sched_getscheduler(0)) < 0)
	{
		printf("Cannot get current client scheduler: %s\n", strerror(errno));
	}
	if (policy != SCHED_FIFO)
		printf("midi thread %d _NOT_ running SCHED_FIFO\n", getpid());
	updatePollFd();
}

//---------------------------------------------------------
//   alsaMidiRead
//---------------------------------------------------------

static void alsaMidiRead(void*, void*)
{
	// calls itself midiDevice->recordEvent(MidiRecordEvent):
	alsaProcessMidiInput();
}

//---------------------------------------------------------
//   midiRead
//---------------------------------------------------------

static void midiRead(void*, void* d)
{
	MidiDevice* dev = (MidiDevice*) d;
	dev->processInput();
}

//---------------------------------------------------------
//   midiWrite
//---------------------------------------------------------

static void midiWrite(void*, void* d)
{
	MidiDevice* dev = (MidiDevice*) d;
	dev->flush();
}

//---------------------------------------------------------
//   updatePollFd
//---------------------------------------------------------

void MidiSeq::updatePollFd()
{
	if (!isRunning())
		return;

	clearPollFd();
	addPollFd(timerFd, POLLIN, midiTick, this, 0);

	if (timerFd == -1)
	{
		fprintf(stderr, "updatePollFd: no timer fd\n");
		if (!debugMode)
			exit(-1);
	}

	addPollFd(toThreadFdr, POLLIN, ::readMsg, this, 0);

	//---------------------------------------------------
	//  midi ports
	//---------------------------------------------------

	for (iMidiDevice imd = midiDevices.begin(); imd != midiDevices.end(); ++imd)
	{
		if(*imd == NULL)
			continue;
		MidiDevice* dev = *imd;
		int port = dev->midiPort();
		const QString name = dev->name();
		if (port == -1)
			continue;
		if ((dev->rwFlags() & 0x2) || (extSyncFlag.value() && (midiPorts[port].syncInfo().MCIn())))
		{
			if (dev->selectRfd() < 0)
			{
				//fprintf(stderr, "WARNING: read-file-descriptor for {%s} is negative\n", name.toLatin1());
			}
			addPollFd(dev->selectRfd(), POLLIN, ::midiRead, this, dev);
		}
		if (dev->bytesToWrite())
		{
			if (dev->selectWfd() < 0)
			{
				//fprintf(stderr, "WARNING: write-file-descriptor for {%s} is negative\n", name.toLatin1());
			}
			addPollFd(dev->selectWfd(), POLLOUT, ::midiWrite, this, dev);
		}
	}
	// special handling for alsa midi:
	// (one fd for all devices)
	//    this allows for processing of some alsa events
	//    even if no alsa driver is active (assigned to a port)
	addPollFd(alsaSelectRfd(), POLLIN, ::alsaMidiRead, this, 0);
}

//---------------------------------------------------------
//   threadStop
//    called from loop()
//---------------------------------------------------------

void MidiSeq::threadStop()
{
	timer->stopTimer();
}

//---------------------------------------------------------
//   setRtcTicks
//    return true on success
//---------------------------------------------------------

bool MidiSeq::setRtcTicks()
{
	timer->setTimerFreq(config.rtcTicks);
	timer->startTimer();
	realRtcTicks = config.rtcTicks;
	return true;
}

//---------------------------------------------------------
//   start
//    return true on error
//---------------------------------------------------------

void MidiSeq::start(int priority)
{
	prio = priority;

	doSetuid();
	setRtcTicks();
	undoSetuid();
	Thread::start(priority);
}

//---------------------------------------------------------
//   processMidiClock
//---------------------------------------------------------

void MidiSeq::processMidiClock()
{
}

//---------------------------------------------------------
//   midiTick
//---------------------------------------------------------

void MidiSeq::midiTick(void* p, void*)
{
	MidiSeq* at = (MidiSeq*) p;
	at->processTimerTick();
	if (TIMER_DEBUG)
	{
		if (MidiSeq::ticker++ > 100)
		{
			printf("tick!\n");
			MidiSeq::ticker = 0;
		}
	}
}

//---------------------------------------------------------
//   processTimerTick
//---------------------------------------------------------

void MidiSeq::processTimerTick()
{
	//---------------------------------------------------
	//    read elapsed rtc timer ticks
	//---------------------------------------------------

	// This is required otherwise it freezes.
	unsigned long nn;
	if (timerFd != -1)
	{
		nn = timer->getTimerTicks();
		nn >>= 8;
	}

	if (idle)
	{
		return;
	}
	if (midiBusy)
	{
		// we hit audio: midiSeq->msgProcess
		// miss this timer tick
		return;
	}

	unsigned curFrame = audio->curFrame();

	if (!extSyncFlag.value())
	{
		int curTick = lrint((double(curFrame) / double(sampleRate)) * double(tempomap.globalTempo()) * double(config.division) * 10000.0 / double(tempomap.tempo(song->cpos())));

		if (midiClock > curTick)
			midiClock = curTick;

		int div = config.division / 24;
		if (curTick >= midiClock + div)
		{
			int perr = (curTick - midiClock) / div;

			bool used = false;

			for (int port = 0; port < MIDI_PORTS; ++port)
			{
				MidiPort* mp = &midiPorts[port];

				// No device? Clock out not turned on?
				if (!mp->device() || !mp->syncInfo().MCOut())
					continue;

				used = true;

				mp->sendClock();
			}

			if (debugMsg && used && perr > 1)
				printf("Dropped %d midi out clock(s). curTick:%d midiClock:%d div:%d\n", perr, curTick, midiClock, div);

			// Keeping in mind how (receiving end) Phase Locked Loops (usually) operate...
			// Increment as if we had caught the timer exactly on the mark, even if the timer
			//  has passed beyond the mark, or even beyond 2 * div.
			// If we missed some chances to send clock, resume the count where it would have been,
			//  had we not missed chances.
			// We can't do anything about missed chances except send right away, and make up
			//  for gained time by losing time in the next count...
			// In other words, use equalization periods to counter gained/lost time, so that
			//  ultimately, over time, the receiver remains in phase, despite any short dropouts / phase glitches.
			// (midiClock only increments by div units).
			//
			// Tested: With midi thread set to high priority, very few clock dropouts ocurred (P4 1.6Ghz).
			// But target device tick drifts out of phase with oom tick slowly over time, say 20 bars or so.
			// May need more tweaking, possibly use round with/instead of lrint (above), and/or
			//  do not use equalization periods - set midiClock to fractions of div.
			// Tested: With RTC resolution at 1024, stability was actually better than with 8192!
			// It stayed in sync more than 64 bars...
			//
			//
			// Using equalization periods...
			midiClock += (perr * div);
		}
	}

	int tickpos = audio->tickPos();
	bool extsync = extSyncFlag.value();
	//
	// play all events upto curFrame
	//
	for (iMidiDevice id = midiDevices.begin(); id != midiDevices.end(); ++id)
	{
		MidiDevice* md = *id;
		if (md->deviceType() == MidiDevice::JACK_MIDI)
			continue;
        if (md->isSynthPlugin()) // synths are handled by audio thread
        {
            // TODO - move this to the GUI thread
            ((SynthPluginDevice*)md)->updateNativeGui();
			continue;
        }
		int port = md->midiPort();
		MidiPort* mp = port != -1 ? &midiPorts[port] : 0;
		MPEventList* el = md->playEvents();
		if (el->empty())
			continue;
		iMPEvent i = el->begin(); 
		for (; i != el->end(); ++i)
		{
			// If syncing to external midi sync, we cannot use the tempo map.
			// Therefore we cannot get sub-tick resolution. Just use ticks instead of frames.
			if (i->time() > (extsync ? tickpos : curFrame))
			{
				break; // skip this event
			}

			if (mp)
			{
				if (mp->sendEvent(*i))
					break;
			}
			else
			{
				if (md->putEvent(*i))
					break;
			}
		}
		el->erase(el->begin(), i);
	}
}

//---------------------------------------------------------
//   msgMsg
//---------------------------------------------------------

void MidiSeq::msgMsg(int id)
{
	AudioMsg msg;
	msg.id = id;
	Thread::sendMsg(&msg);
}

//---------------------------------------------------------
//   msgSetMidiDevice
//    to avoid timeouts in the RT-thread, setMidiDevice
//    is done in GUI context after setting the midi thread
//    into idle mode
//---------------------------------------------------------

void MidiSeq::msgSetMidiDevice(MidiPort* port, MidiDevice* device)
{
	AudioMsg msg;
	msg.id = SEQM_IDLE;
	msg.a = true;
	Thread::sendMsg(&msg);

	port->setMidiDevice(device);

	msg.id = SEQM_IDLE;
	msg.a = false;
	Thread::sendMsg(&msg);
}

void MidiSeq::msgPreloadCtrl()
{
	msgMsg(SEQM_PRELOAD_PROGRAM);
}

void MidiSeq::msgSeek()
{
	msgMsg(SEQM_SEEK);
}

void MidiSeq::msgStop()
{
	msgMsg(MS_STOP);
}

void MidiSeq::msgSetRtc()
{
	msgMsg(MS_SET_RTC);
}

void MidiSeq::msgUpdatePollFd()
{
	msgMsg(MS_UPDATE_POLL_FD);
}

