//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: sync.cpp,v 1.6.2.12 2009/06/20 22:20:41 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#include <cmath>
#include "sync.h"
#include "song.h"
#include "utils.h"
#include "midiport.h"
#include "mididev.h"
#include "globals.h"
#include "midiseq.h"
#include "audio.h"
#include "audiodev.h"
#include "gconfig.h"
#include "xml.h"
#include "midi.h"

int volatile curMidiSyncInPort = -1;

bool debugSync = true;

int mtcType = 1;
MTC mtcOffset;
BValue extSyncFlag(0, "extSync"); // false - MASTER, true - SLAVE
BValue useJackTransport(0, "useJackTransport");
bool volatile jackTransportMaster = true;

static MTC mtcCurTime;
static int mtcState; // 0-7 next expected quarter message
static bool mtcValid;
static int mtcLost;
static bool mtcSync; // receive complete mtc frame?

static bool playPendingFirstClock = false;
unsigned int syncSendFirstClockDelay = 1; // In milliseconds.
static unsigned int curExtMidiSyncTick = 0;
unsigned int volatile lastExtMidiSyncTick = 0;
double volatile curExtMidiSyncTime = 0.0;
double volatile lastExtMidiSyncTime = 0.0;

// From the "Introduction to the Volatile Keyword" at Embedded dot com
/* A variable should be declared volatile whenever its value could change unexpectedly. 
 ... <such as> global variables within a multi-threaded application    
 ... So all shared global variables should be declared volatile */
unsigned int volatile midiExtSyncTicks = 0;

//---------------------------------------------------------
//  MidiSyncInfo
//---------------------------------------------------------

MidiSyncInfo::MidiSyncInfo()
{
	_port = -1;
	_idOut = 127;
	_idIn = 127;
	_sendMC = false;
	_sendMRT = false;
	_sendMMC = false;
	_sendMTC = false;
	_recMC = false;
	_recMRT = false;
	_recMMC = false;
	_recMTC = false;

	_lastClkTime = 0.0;
	_lastTickTime = 0.0;
	_lastMRTTime = 0.0;
	_lastMMCTime = 0.0;
	_lastMTCTime = 0.0;
	_clockTrig = false;
	_tickTrig = false;
	_MRTTrig = false;
	_MMCTrig = false;
	_MTCTrig = false;
	_clockDetect = false;
	_tickDetect = false;
	_MRTDetect = false;
	_MMCDetect = false;
	_MTCDetect = false;
	_recMTCtype = 0;
	_recRewOnStart = true;
	_actDetectBits = 0;
	for (int i = 0; i < MIDI_CHANNELS; ++i)
	{
		_lastActTime[i] = 0.0;
		_actTrig[i] = false;
		_actDetect[i] = false;
	}
}

//---------------------------------------------------------
//   operator =
//---------------------------------------------------------

MidiSyncInfo& MidiSyncInfo::operator=(const MidiSyncInfo &sp)
{
	copyParams(sp);

	_lastClkTime = sp._lastClkTime;
	_lastTickTime = sp._lastTickTime;
	_lastMRTTime = sp._lastMRTTime;
	_lastMMCTime = sp._lastMMCTime;
	_lastMTCTime = sp._lastMTCTime;
	_clockTrig = sp._clockTrig;
	_tickTrig = sp._tickTrig;
	_MRTTrig = sp._MRTTrig;
	_MMCTrig = sp._MMCTrig;
	_MTCTrig = sp._MTCTrig;
	_clockDetect = sp._clockDetect;
	_tickDetect = sp._tickDetect;
	_MRTDetect = sp._MRTDetect;
	_MMCDetect = sp._MMCDetect;
	_MTCDetect = sp._MTCDetect;
	_recMTCtype = sp._recMTCtype;
	for (int i = 0; i < MIDI_CHANNELS; ++i)
	{
		_lastActTime[i] = sp._lastActTime[i];
		_actTrig[i] = sp._actTrig[i];
		_actDetect[i] = sp._actDetect[i];
	}
	return *this;
}

//---------------------------------------------------------
//   copyParams
//---------------------------------------------------------

MidiSyncInfo& MidiSyncInfo::copyParams(const MidiSyncInfo &sp)
{
	_idOut = sp._idOut;
	_idIn = sp._idIn;
	_sendMC = sp._sendMC;
	_sendMRT = sp._sendMRT;
	_sendMMC = sp._sendMMC;
	_sendMTC = sp._sendMTC;
	setMCIn(sp._recMC);
	_recMRT = sp._recMRT;
	_recMMC = sp._recMMC;
	_recMTC = sp._recMTC;
	_recRewOnStart = sp._recRewOnStart;
	return *this;
}

//---------------------------------------------------------
//  setTime
//---------------------------------------------------------

void MidiSyncInfo::setTime()
{
	// Note: CurTime() makes a system call to gettimeofday(),
	//  which apparently can be slow in some cases. So I avoid calling this function
	//  too frequently by calling it (at the heartbeat rate) in Song::beat().  T356
	double t = curTime();

	if (_clockTrig)
	{
		_clockTrig = false;
		_lastClkTime = t;
	}
	else
		if (_clockDetect && (t - _lastClkTime >= 1.0)) // Set detect indicator timeout to about 1 second.
	{
		_clockDetect = false;
		// Give up the current midi sync in port number if we took it...
		if (curMidiSyncInPort == _port)
			curMidiSyncInPort = -1;
	}

	if (_tickTrig)
	{
		_tickTrig = false;
		_lastTickTime = t;
	}
	else
		if (_tickDetect && (t - _lastTickTime) >= 1.0) // Set detect indicator timeout to about 1 second.
		_tickDetect = false;

	if (_MRTTrig)
	{
		_MRTTrig = false;
		_lastMRTTime = t;
	}
	else
		if (_MRTDetect && (t - _lastMRTTime) >= 1.0) // Set detect indicator timeout to about 1 second.
	{
		_MRTDetect = false;
	}

	if (_MMCTrig)
	{
		_MMCTrig = false;
		_lastMMCTime = t;
	}
	else
		if (_MMCDetect && (t - _lastMMCTime) >= 1.0) // Set detect indicator timeout to about 1 second.
	{
		_MMCDetect = false;
	}

	if (_MTCTrig)
	{
		_MTCTrig = false;
		_lastMTCTime = t;
	}
	else
		if (_MTCDetect && (t - _lastMTCTime) >= 1.0) // Set detect indicator timeout to about 1 second.
	{
		_MTCDetect = false;
		// Give up the current midi sync in port number if we took it...
		if (curMidiSyncInPort == _port)
			curMidiSyncInPort = -1;
	}

	for (int i = 0; i < MIDI_CHANNELS; i++)
	{
		if (_actTrig[i])
		{
			_actTrig[i] = false;
			_lastActTime[i] = t;
		}
		else
			if (_actDetect[i] && (t - _lastActTime[i]) >= 1.0) // Set detect indicator timeout to about 1 second.
		{
			_actDetect[i] = false;
			_actDetectBits &= ~(1 << i);
		}
	}
}

//---------------------------------------------------------
//  setMCIn
//---------------------------------------------------------

void MidiSyncInfo::setMCIn(const bool v)
{
	_recMC = v;
	// If sync receive was turned off, clear the current midi sync in port number so another port can grab it.
	if (!_recMC && _port != -1 && curMidiSyncInPort == _port)
		curMidiSyncInPort = -1;
}

//---------------------------------------------------------
//  setMRTIn
//---------------------------------------------------------

void MidiSyncInfo::setMRTIn(const bool v)
{
	_recMRT = v;
}

//---------------------------------------------------------
//  setMMCIn
//---------------------------------------------------------

void MidiSyncInfo::setMMCIn(const bool v)
{
	_recMMC = v;
}

//---------------------------------------------------------
//  setMTCIn
//---------------------------------------------------------

void MidiSyncInfo::setMTCIn(const bool v)
{
	_recMTC = v;
	// If sync receive was turned off, clear the current midi sync in port number so another port can grab it.
	if (!_recMTC && _port != -1 && curMidiSyncInPort == _port)
		curMidiSyncInPort = -1;
}

//---------------------------------------------------------
//  trigMCSyncDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMCSyncDetect()
{
	_clockDetect = true;
	_clockTrig = true;
	// Set the current midi sync in port number if it's not taken...
	if (_recMC && curMidiSyncInPort == -1)
		curMidiSyncInPort = _port;
}

//---------------------------------------------------------
//  trigTickDetect
//---------------------------------------------------------

void MidiSyncInfo::trigTickDetect()
{
	_tickDetect = true;
	_tickTrig = true;
}

//---------------------------------------------------------
//  trigMRTDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMRTDetect()
{
	_MRTDetect = true;
	_MRTTrig = true;
}

//---------------------------------------------------------
//  trigMMCDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMMCDetect()
{
	_MMCDetect = true;
	_MMCTrig = true;
}

//---------------------------------------------------------
//  trigMTCDetect
//---------------------------------------------------------

void MidiSyncInfo::trigMTCDetect()
{
	_MTCDetect = true;
	_MTCTrig = true;
	// Set the current midi sync in port number if it's not taken...
	if (_recMTC && curMidiSyncInPort == -1)
		curMidiSyncInPort = _port;
}

//---------------------------------------------------------
//  actDetect
//---------------------------------------------------------

bool MidiSyncInfo::actDetect(const int ch) const
{
	if (ch < 0 || ch >= MIDI_CHANNELS)
		return false;

	return _actDetect[ch];
}

//---------------------------------------------------------
//  trigActDetect
//---------------------------------------------------------

void MidiSyncInfo::trigActDetect(const int ch)
{
	if (ch < 0 || ch >= MIDI_CHANNELS)
		return;

	//_actDetectBits |= bitShiftLU[ch];
	_actDetectBits |= (1 << ch);
	_actDetect[ch] = true;
	_actTrig[ch] = true;
}

//---------------------------------------------------------
//   isDefault
//---------------------------------------------------------

bool MidiSyncInfo::isDefault() const
{
	return (_idOut == 127 && _idIn == 127 && !_sendMC && !_sendMRT && !_sendMMC && !_sendMTC &&
			/* !_sendContNotStart && */ !_recMC && !_recMRT && !_recMMC && !_recMTC && _recRewOnStart);
}

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiSyncInfo::read(Xml& xml)
{
	for (;;)
	{
		Xml::Token token(xml.parse());
		const QString & tag(xml.s1());
		switch (token)
		{
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "idOut")
					_idOut = xml.parseInt();
				else if (tag == "idIn")
					_idIn = xml.parseInt();
				else if (tag == "sendMC")
					_sendMC = xml.parseInt();
				else if (tag == "sendMRT")
					_sendMRT = xml.parseInt();
				else if (tag == "sendMMC")
					_sendMMC = xml.parseInt();
				else if (tag == "sendMTC")
					_sendMTC = xml.parseInt();
				else if (tag == "recMC")
					_recMC = xml.parseInt();
				else if (tag == "recMRT")
					_recMRT = xml.parseInt();
				else if (tag == "recMMC")
					_recMMC = xml.parseInt();
				else if (tag == "recMTC")
					_recMTC = xml.parseInt();
				else if (tag == "recRewStart")
					_recRewOnStart = xml.parseInt();
				else
					xml.unknown("midiSyncInfo");
				break;
			case Xml::TagEnd:
				if (tag == "midiSyncInfo")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//  write
//---------------------------------------------------------

//void MidiSyncInfo::write(int level, Xml& xml, MidiDevice* md)

void MidiSyncInfo::write(int level, Xml& xml)
{
	// All defaults? Nothing to write.
	if (isDefault())
		return;

	xml.tag(level++, "midiSyncInfo");

	if (_idOut != 127)
		xml.intTag(level, "idOut", _idOut);
	if (_idIn != 127)
		xml.intTag(level, "idIn", _idIn);

	if (_sendMC)
		xml.intTag(level, "sendMC", true);
	if (_sendMRT)
		xml.intTag(level, "sendMRT", true);
	if (_sendMRT)
		xml.intTag(level, "sendMMC", true);
	if (_sendMTC)
		xml.intTag(level, "sendMTC", true);

	if (_recMC)
		xml.intTag(level, "recMC", true);
	if (_recMRT)
		xml.intTag(level, "recMRT", true);
	if (_recMMC)
		xml.intTag(level, "recMMC", true);
	if (_recMTC)
		xml.intTag(level, "recMTC", true);
	if (!_recRewOnStart)
		xml.intTag(level, "recRewStart", false);

	xml.etag(level, "midiSyncInfo");
}

//---------------------------------------------------------
//  mmcInput
//    Midi Machine Control Input received
//---------------------------------------------------------

void MidiSeq::mmcInput(int port, const unsigned char* p, int n)
{
	if (debugSync)
		printf("mmcInput: n:%d %02x %02x %02x %02x\n",
			n, p[2], p[3], p[4], p[5]);

	MidiPort* mp = &midiPorts[port];
	MidiSyncInfo& msync = mp->syncInfo();
	// Trigger MMC detect in.
	msync.trigMMCDetect();
	// MMC locate SMPTE time code may contain format type bits. Grab them.
	if (p[3] == 0x44 && p[4] == 6 && p[5] == 1)
		msync.setRecMTCtype((p[6] >> 5) & 3);

	// MMC in not turned on? Forget it.
	if (!msync.MMCIn())
		return;

	switch (p[3])
	{
		case 1:
			if (debugSync)
				printf("  MMC: STOP\n");

			playPendingFirstClock = false;

			if (audio->isPlaying())
				audio->msgPlay(false);
			playStateExt = false;
			alignAllTicks();
			break;
		case 2:
			if (debugSync)
				printf("  MMC: PLAY\n");
		case 3:
			if (debugSync)
				printf("  MMC: DEFERRED PLAY\n");
			mtcState = 0;
			mtcValid = false;
			mtcLost = 0;
			mtcSync = false;
			alignAllTicks();
			audio->msgPlay(true);
			playStateExt = true;
			break;

		case 4:
			printf("MMC: FF not implemented\n");
			playPendingFirstClock = false;
			break;
		case 5:
			printf("MMC: REWIND not implemented\n");
			playPendingFirstClock = false;
			break;
		case 6:
			printf("MMC: REC STROBE not implemented\n");
			playPendingFirstClock = false;
			break;
		case 7:
			printf("MMC: REC EXIT not implemented\n");
			playPendingFirstClock = false;
			break;
		case 0xd:
			printf("MMC: RESET not implemented\n");
			playPendingFirstClock = false;
			break;
		case 0x44:
			if (p[5] == 0)
			{
				printf("MMC: LOCATE IF not implemented\n");
				break;
			}
			else if (p[5] == 1)
			{
				if (!checkAudioDevice()) return;
				MTC mtc(p[6] & 0x1f, p[7], p[8], p[9], p[10]);
				int type = (p[6] >> 5) & 3;
				int mmcPos = lrint(mtc.time(type) * sampleRate);

				Pos tp(mmcPos, false);
				audioDevice->seekTransport(tp);
				alignAllTicks();
				if (debugSync)
				{
					//printf("MMC: %f %d seek ", mtc.time(), mmcPos);
					printf("MMC: LOCATE mtc type:%d time:%lf frame:%d mtc: ", type, mtc.time(), mmcPos);
					mtc.print();
					printf("\n");
				}
				break;
			}
		default:
			printf("MMC %x %x, unknown\n", p[3], p[4]);
			break;
	}
}

//---------------------------------------------------------
//   mtcInputQuarter
//    process Quarter Frame Message
//---------------------------------------------------------

void MidiSeq::mtcInputQuarter(int port, unsigned char c)
{
	static int hour, min, sec, frame;

	//printf("MidiSeq::mtcInputQuarter c:%h\n", c);

	int valL = c & 0xf;
	int valH = valL << 4;

	int _state = (c & 0x70) >> 4;
	if (mtcState != _state)
		mtcLost += _state - mtcState;
	mtcState = _state + 1;

	switch (_state)
	{
		case 7:
			hour = (hour & 0x0f) | valH;
			break;
		case 6:
			hour = (hour & 0xf0) | valL;
			break;
		case 5:
			min = (min & 0x0f) | valH;
			break;
		case 4:
			min = (min & 0xf0) | valL;
			break;
		case 3:
			sec = (sec & 0x0f) | valH;
			break;
		case 2:
			sec = (sec & 0xf0) | valL;
			break;
		case 1:
			frame = (frame & 0x0f) | valH;
			break;
		case 0: frame = (frame & 0xf0) | valL;
			break;
	}
	frame &= 0x1f; // 0-29
	sec &= 0x3f; // 0-59
	min &= 0x3f; // 0-59
	int tmphour = hour;
	int type = (hour >> 5) & 3;
	hour &= 0x1f;

	if (mtcState == 8)
	{
		mtcValid = (mtcLost == 0);
		mtcState = 0;
		mtcLost = 0;
		if (mtcValid)
		{
			mtcCurTime.set(hour, min, sec, frame);
			if (port != -1)
			{
				MidiPort* mp = &midiPorts[port];
				MidiSyncInfo& msync = mp->syncInfo();
				msync.setRecMTCtype(type);
				msync.trigMTCDetect();
				// Not for the current in port? External sync not turned on? MTC in not turned on? Forget it.
				if (port == curMidiSyncInPort && extSyncFlag.value() && msync.MTCIn())
				{
					if (debugSync)
						printf("MidiSeq::mtcInputQuarter hour byte:%hx\n", tmphour);
					mtcSyncMsg(mtcCurTime, type, !mtcSync);
				}
			}
			mtcSync = true;
		}
	}
	else if (mtcValid && (mtcLost == 0))
	{
		mtcCurTime.incQuarter(type);
	}
}

//---------------------------------------------------------
//   mtcInputFull
//    process Frame Message
//---------------------------------------------------------

void MidiSeq::mtcInputFull(int port, const unsigned char* p, int n)
{
	if (debugSync)
		printf("mtcInputFull\n");

	if (p[3] != 1)
	{
		if (p[3] != 2)
		{ // silently ignore user bits
			printf("unknown mtc msg subtype 0x%02x\n", p[3]);
			dump(p, n);
		}
		return;
	}
	int hour = p[4];
	int min = p[5];
	int sec = p[6];
	int frame = p[7];

	frame &= 0x1f; // 0-29
	sec &= 0x3f; // 0-59
	min &= 0x3f; // 0-59
	int type = (hour >> 5) & 3;
	hour &= 0x1f;

	mtcCurTime.set(hour, min, sec, frame);
	mtcState = 0;
	mtcValid = true;
	mtcLost = 0;

	if (debugSync)
		printf("mtcInputFull: time:%lf stime:%lf hour byte (all bits):%hx\n", mtcCurTime.time(), mtcCurTime.time(type), p[4]);
	if (port != -1)
	{
		MidiPort* mp = &midiPorts[port];
		MidiSyncInfo& msync = mp->syncInfo();
		msync.setRecMTCtype(type);
		msync.trigMTCDetect();
		if (msync.MTCIn())
		{
			Pos tp(lrint(mtcCurTime.time(type) * sampleRate), false);
			audioDevice->seekTransport(tp);
			alignAllTicks();
		}
	}
}

//---------------------------------------------------------
//   nonRealtimeSystemSysex
//---------------------------------------------------------

void MidiSeq::nonRealtimeSystemSysex(int /*port*/, const unsigned char* p, int n)
{
	switch (p[3])
	{
		case 4:
			printf("NRT Setup\n");
			break;
		default:
			printf("unknown NRT Msg 0x%02x\n", p[3]);
			dump(p, n);
			break;
	}
}

//---------------------------------------------------------
//   setSongPosition
//    MidiBeat is a 14 Bit value. Each MidiBeat spans
//    6 MIDI Clocks. Inother words, each MIDI Beat is a
//    16th note (since there are 24 MIDI Clocks in a
//    quarter note).
//---------------------------------------------------------

void MidiSeq::setSongPosition(int port, int midiBeat)
{
	if (midiInputTrace)
		printf("set song position port:%d %d\n", port, midiBeat);

	midiPorts[port].syncInfo().trigMRTDetect();

	if (!extSyncFlag.value() || !midiPorts[port].syncInfo().MRTIn())
		return;

	// Re-transmit song position to other devices if clock out turned on.
	for (int p = 0; p < MIDI_PORTS; ++p)
		if (p != port && midiPorts[p].syncInfo().MRTOut())
			midiPorts[p].sendSongpos(midiBeat);

	curExtMidiSyncTick = (config.division * midiBeat) / 4;
	lastExtMidiSyncTick = curExtMidiSyncTick;

	Pos pos(curExtMidiSyncTick, true);

	if (!checkAudioDevice()) return;

	audioDevice->seekTransport(pos);
	alignAllTicks(pos.frame());
	if (debugSync)
		printf("setSongPosition %d\n", pos.tick());
}



//---------------------------------------------------------
//   set all runtime variables to the "in sync" value
//---------------------------------------------------------

void MidiSeq::alignAllTicks(int frameOverride)
{
	unsigned curFrame;
	if (!frameOverride)
		curFrame = audio->pos().frame();
	else
		curFrame = frameOverride;

	int tempo = tempomap.tempo(0);

	// use the last old values to give start values for the tripple buffering
	int recTickSpan = recTick1 - recTick2;
	int songTickSpan = (int) (songtick1 - songtick2); //prevent compiler warning:  casting double to int
	storedtimediffs = 0; // pretend there is no sync history

	mclock2 = mclock1 = 0.0; // set all clock values to "in sync"

	recTick = (int) ((double(curFrame) / double(sampleRate)) *
			double(config.division * 1000000.0) / double(tempo) //prevent compiler warning:  casting double to int
			);
	songtick1 = recTick - songTickSpan;
	if (songtick1 < 0)
		songtick1 = 0;
	songtick2 = songtick1 - songTickSpan;
	if (songtick2 < 0)
		songtick2 = 0;
	recTick1 = recTick - recTickSpan;
	if (recTick1 < 0)
		recTick1 = 0;
	recTick2 = recTick1 - recTickSpan;
	if (recTick2 < 0)
		recTick2 = 0;
	if (debugSync)
		printf("alignAllTicks curFrame=%d recTick=%d tempo=%.3f frameOverride=%d\n", curFrame, recTick, (float) ((1000000.0 * 60.0) / tempo), frameOverride);

}

//---------------------------------------------------------
//   realtimeSystemInput
//    real time message received
//---------------------------------------------------------

void MidiSeq::realtimeSystemInput(int port, int c)
{

	if (midiInputTrace)
		printf("realtimeSystemInput port:%d 0x%x\n", port + 1, c);

	MidiPort* mp = &midiPorts[port];

	// Trigger on any tick, clock, or realtime command.
	if (c == ME_TICK) // Tick
		mp->syncInfo().trigTickDetect();
	else
		if (c == ME_CLOCK) // Clock
		mp->syncInfo().trigMCSyncDetect();
	else
		mp->syncInfo().trigMRTDetect(); // Other

	// External sync not on? Clock in not turned on? Otherwise realtime in not turned on?
	if (!extSyncFlag.value())
		return;
	if (c == ME_CLOCK)
	{
		if (!mp->syncInfo().MCIn())
			return;
	}
	else
		if (!mp->syncInfo().MRTIn())
		return;


	switch (c)
	{
		case ME_CLOCK: // midi clock (24 ticks / quarter note)
		{
			// Not for the current in port? Forget it.
			if (port != curMidiSyncInPort)
				break;

			// Re-transmit clock to other devices if clock out turned on.
			// Must be careful not to allow more than one clock input at a time.
			// Would re-transmit mixture of multiple clocks - confusing receivers.
			// Solution: Added curMidiSyncInPort.
			// Maybe in MidiSeq::processTimerTick(), call sendClock for the other devices, instead of here.
			for (int p = 0; p < MIDI_PORTS; ++p)
				if (p != port && midiPorts[p].syncInfo().MCOut())
					midiPorts[p].sendClock();

			if (playPendingFirstClock)
			{
				playPendingFirstClock = false;
				// Hopefully the transport will be ready by now, the seek upon start should mean the
				//  audio prefetch has already finished or at least started...
				// Must comfirm that play does not force a complete prefetch again, but don't think so...
				if (!audio->isPlaying())
					audioDevice->startTransport();
			}
			// This part will be run on the second and subsequent clocks, after start.
			// Can't check audio state, might not be playing yet, we might miss some increments.
			if (playStateExt)
			{
				lastExtMidiSyncTime = curExtMidiSyncTime;
				curExtMidiSyncTime = curTime();
				int div = config.division / 24;
				midiExtSyncTicks += div;
				lastExtMidiSyncTick = curExtMidiSyncTick;
				curExtMidiSyncTick += div;
			}
		}
			break;
		case ME_TICK: // midi tick  (every 10 msec)
			// FIXME: Unfinished? mcStartTick is uninitialized and Song::setPos doesn't set it either. Dangerous to allow this.
			//if (mcStart) {
			//      song->setPos(0, mcStartTick);
			//      mcStart = false;
			//      return;
			//      }
			break;
		case ME_START: // start
			// Re-transmit start to other devices if clock out turned on.
			for (int p = 0; p < MIDI_PORTS; ++p)
			{
				if (p != port && midiPorts[p].syncInfo().MRTOut())
				{
					// If we aren't rewinding on start, there's no point in re-sending start.
					// Re-send continue instead, for consistency.
					if (midiPorts[port].syncInfo().recRewOnStart())
						midiPorts[p].sendStart();
					else
						midiPorts[p].sendContinue();
				}
			}

			if (1 /* !audio->isPlaying()*/ /*state == IDLE*/)
			{
				if (!checkAudioDevice()) return;

				// Rew on start option.
				if (midiPorts[port].syncInfo().recRewOnStart())
				{
					curExtMidiSyncTick = 0;
					lastExtMidiSyncTick = curExtMidiSyncTick;
					//audioDevice->seekTransport(0);
					audioDevice->seekTransport(Pos(0, false));
				}

				alignAllTicks();

				storedtimediffs = 0;
				for (int i = 0; i < 24; i++)
					timediff[i] = 0.0;

				// Changed because msgPlay calls audioDevice->seekTransport(song->cPos())
				//  and song->cPos() may not be changed to 0 yet, causing tranport not to go to 0.
				playPendingFirstClock = true;

				midiExtSyncTicks = 0;
				playStateExt = true;
			}
			break;
		case ME_CONTINUE: // continue
			// Re-transmit continue to other devices if clock out turned on.
			for (int p = 0; p < MIDI_PORTS; ++p)
				if (p != port && midiPorts[p].syncInfo().MRTOut())
					midiPorts[p].sendContinue();

			if (debugSync)
				printf("realtimeSystemInput continue\n");

			if (1 /* !audio->isPlaying() */ /*state == IDLE */)
			{
				// Begin incrementing immediately upon first clock reception.
				playPendingFirstClock = true;

				playStateExt = true;
			}
			break;
		case ME_STOP: // stop
		{
			// Stop the increment right away.
			midiExtSyncTicks = 0;
			playStateExt = false;
			playPendingFirstClock = false;

			// Re-transmit stop to other devices if clock out turned on.
			for (int p = 0; p < MIDI_PORTS; ++p)
				if (p != port && midiPorts[p].syncInfo().MRTOut())
					midiPorts[p].sendStop();

			if (audio->isPlaying() /*state == PLAY*/)
			{
				audio->msgPlay(false);
			}

			if (debugSync)
				printf("realtimeSystemInput stop\n");
		}
			break;
		default:
			break;
	}

}

//---------------------------------------------------------
//   mtcSyncMsg
//    process received mtc Sync
//    seekFlag - first complete mtc frame received after
//                start
//---------------------------------------------------------

void MidiSeq::mtcSyncMsg(const MTC& mtc, int type, bool seekFlag)
{
	double time = mtc.time();
	double stime = mtc.time(type);
	if (debugSync)
		printf("MidiSeq::mtcSyncMsg time:%lf stime:%lf seekFlag:%d\n", time, stime, seekFlag);

	if (seekFlag && audio->isRunning() /*state == START_PLAY*/)
	{
		if (!checkAudioDevice()) return;
		if (debugSync)
			printf("MidiSeq::mtcSyncMsg starting transport.\n");
		audioDevice->startTransport();
		return;
	}
}


