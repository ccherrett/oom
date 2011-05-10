#include <QObject>
#include <QSocketNotifier>
#include <QByteArray>

#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <values.h>
#include <cmath>
#include <errno.h>
#include <fcntl.h>

#include "globals.h"
#include "globaldefs.h"
#include "config.h"
#include "midiport.h"
#include "mididev.h"
#include "midictrl.h"
#include "midi.h"
#include "audio.h"
#include "song.h"
#include "widgets/utils.h"
#include "midimonitor.h"

MidiMonitor* midiMonitor;

static void readMsgMM(void* m, void*)
{
	MidiMonitor* mm = (MidiMonitor*) m;
	mm->readMsg1(sizeof (MonitorMsg));
}

MidiMonitor::MidiMonitor(const char* name) : Thread(name)
{
	//Start with this mode on so we dont process any events at all untill populateList is called
	m_editing = true;
	int filedes[2]; // 0 - reading   1 - writing/*{{{*/
	if (pipe(filedes) == -1)
	{
		perror("creating pipe0");
		exit(-1);
	}
	fromThreadFdw = filedes[1];
	fromThreadFdr = filedes[0];
	int rv = fcntl(fromThreadFdw, F_SETFL, O_NONBLOCK);
	if (rv == -1)
		perror("set pipe O_NONBLOCK");

	if (pipe(filedes) == -1)
	{
		perror("creating pipe1");
		exit(-1);
	}
	sigFd = filedes[1];
	QSocketNotifier* ss = new QSocketNotifier(filedes[0], QSocketNotifier::Read);
	song->connect(ss, SIGNAL(activated(int)), song, SLOT(playMonitorEvent(int)));/*}}}*/
}

MidiMonitor::~MidiMonitor()
{
	//flush all our list and delete anything that will hang the program
}

void MidiMonitor::start(int priority)/*{{{*/
{
	clearPollFd();
	addPollFd(toThreadFdr, POLLIN, ::readMsgMM, this, 0);
	Thread::start(priority);
}/*}}}*/

void MidiMonitor::msgSendMidiInputEvent(MEvent& event)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_MIDI_IN;
	msg.mevent = event;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgSendMidiOutputEvent(Track* track,  int ctl, int val)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_MIDI_OUT;
	msg.track = track;
	msg.ctl = ctl;
	msg.mval = val;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgSendAudioOutputEvent(Track* track, int ctl, double val)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_AUDIO_OUT;
	msg.track = track;
	msg.ctl = ctl;
	msg.aval = val;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgModifyTrackController(Track* track,int ctl ,int cc)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_MODIFY_CC;
	msg.track = track;
	msg.ctl = ctl;
	msg.mval = cc;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgModifyTrackPort(Track* track,int port)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_MODIFY_PORT;
	msg.track = track;
	msg.port = port;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgAddMonitoredTrack(Track* track)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_ADD_TRACK;
	msg.track = track;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgDeleteMonitoredTrack(Track* track)/*{{{*/
{
	MonitorMsg msg;
	msg.id = MONITOR_ADD_TRACK;
	msg.track = track;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::processMsg1(const void* m)/*{{{*/
{
	if(m_editing)
		return;
	const MonitorMsg* msg = (MonitorMsg*)m;
	int type = msg->id;
	switch(type)
	{
		case MONITOR_AUDIO_OUT:	//Used to process outgoing midi from audio tracks
			//printf("MidiMonitor::processMsg1() Audio Output\n");
			if(msg->track && isAssigned(msg->track->name()))/*{{{*/
			{
				MidiAssignData* data = m_assignments.value(msg->track->name());
				if(!isManagedController(msg->ctl) || !data->enabled)
					return;
				int ccval = data->midimap.value(msg->ctl);
				if(ccval < 0)
					return;
				int val = 0;
				if(msg->ctl == CTRL_PANPOT)
					val = trackPanToMidi(msg->aval);//, midiToTrackPan(trackPanToMidi(val)));
				else
					val = dbToMidi(trackVolToDb(msg->aval));
				//printf("Sending midivalue from audio track: %d\n", val);
				//TODO: Check if feedback is required before bothering with this
				MidiPlayEvent ev(0, data->port, data->channel, ME_CONTROLLER, ccval, val);
				midiPorts[ev.port()].device()->putEvent(ev);
				//audio->msgPlayMidiEvent(&ev);
			}/*}}}*/
		break;
		case MONITOR_MIDI_IN:	//Used to process incomming midi going to midi tracks/controllers
		{/*{{{*/
			//printf("MidiMonitor::processMsg1() event type:%d port:%d channel:%d CC:%d CCVal:%d \n",
			//	msg->mevent.type(), msg->mevent.port(), msg->mevent.channel(), msg->mevent.dataA(), msg->mevent.dataB());
			if(isManagedInputPort(msg->mevent.port()))
			{
				//printf("MidiMonitor::processMsg1() Processing Midi Input\n");
				QMultiHash<int, QString>::iterator i = m_portccmap.find(msg->mevent.dataA());
				while (i != m_portccmap.end()) 
				{
					QString tname(i.value());
					MidiAssignData* data = m_assignments.value(tname);
					if(data && data->enabled && data->port == msg->mevent.port() && data->channel == msg->mevent.channel() && !data->midimap.isEmpty())
					{
						int ctl = data->midimap.key(msg->mevent.dataA());
						if(data->track)
						{
							switch(ctl)/*{{{*/
							{
								case CTRL_VOLUME:
								{
									if(data->track->isMidiTrack())/*{{{*/
									{
										MidiTrack* track = (MidiTrack*)data->track;
										QString cmd;
										//cmd.append(QString::number(msg->mevent.time())).append(":");
										cmd.append(QString::number(track->outPort())).append(":");
										cmd.append(QString::number(track->outChannel())).append(":");
										cmd.append(QString::number(ctl)).append(":");
										cmd.append(QString::number(msg->mevent.dataB())).append("$$");
										QByteArray ba(cmd.toUtf8().constData());
										//printf("ByteArray size in MidiMonitor: %d\n", ba.size());
										write(sigFd, ba.constData(), 16);
									}
									else
									{
										audio->msgSetVolume((AudioTrack*) data->track, dbToTrackVol(midiToDb(msg->mevent.dataB())));
										((AudioTrack*) data->track)->startAutoRecord(AC_VOLUME, dbToTrackVol(midiToDb(msg->mevent.dataB())));
									}/*}}}*/
								}
								break;
								case CTRL_PANPOT:
								{
									if(data->track->isMidiTrack())/*{{{*/
									{
										MidiTrack* track = (MidiTrack*)data->track;
										QString cmd;
										//cmd.append(QString::number(msg->mevent.time())).append(":");
										cmd.append(QString::number(track->outPort())).append(":");
										cmd.append(QString::number(track->outChannel())).append(":");
										cmd.append(QString::number(ctl)).append(":");
										cmd.append(QString::number(msg->mevent.dataB())).append("$$");
										QByteArray ba(cmd.toUtf8().constData());
										//printf("ByteArray size in MidiMonitor: %d\n", ba.size());
										write(sigFd, ba.constData(), 16);
									}
									else
									{
										audio->msgSetPan(((AudioTrack*) data->track), midiToTrackPan(msg->mevent.dataB()));
										((AudioTrack*) data->track)->recordAutomation(AC_PAN, midiToTrackPan(msg->mevent.dataB()));
									}/*}}}*/
								}
								break;
								case CTRL_RECORD:
									song->setRecordFlag(data->track, !data->track->recordFlag(), true);
								break;
								case CTRL_MUTE:
									data->track->setMute(!data->track->isMute(), true);//msg->mevent.dataB() ? true : false, true);
									song->update(SC_MUTE);
								break;
								case CTRL_SOLO:
									data->track->setSolo(!data->track->solo(), true);//msg->mevent.dataB() ? true : false, true);
									song->update(SC_SOLO);
								break;
								case CTRL_REVERB_SEND:
								case CTRL_CHORUS_SEND:
								case CTRL_VARIATION_SEND:
								{
									if(data->track->isMidiTrack())
									{
										MidiTrack* track = (MidiTrack*)data->track;
										QString cmd;
										//cmd.append(QString::number(msg->mevent.time())).append(":");
										cmd.append(QString::number(track->outPort())).append(":");
										cmd.append(QString::number(track->outChannel())).append(":");
										cmd.append(QString::number(ctl)).append(":");
										cmd.append(QString::number(msg->mevent.dataB())).append("$$");
										QByteArray ba(cmd.toUtf8().constData());
										//printf("ByteArray size in MidiMonitor: %d\n", ba.size());
										write(sigFd, ba.constData(), 16);
									}
								}
								break;
							}/*}}}*/
						}
					}
					++i;
				}//END while loop
			}
		}/*}}}*/
		break;
		case MONITOR_MIDI_OUT:	//Used to process outgoing midi from midi tracks
			//printf("MidiMonitor::processMsg1() Midi Output\n");
			if(msg->track && isAssigned(msg->track->name()))/*{{{*/
			{
				MidiAssignData* data = m_assignments.value(msg->track->name());
				if(!isManagedController(msg->ctl) || !data->enabled || data->midimap.isEmpty())
					return;
				int ccval = data->midimap.value(msg->ctl);
				if(ccval < 0)
					return;
				//printf("Sending midivalue from audio track: %d\n", msg->mval);
				//TODO: Check if feedback is required before bothering with this
				MidiPlayEvent ev(0, data->port, data->channel, ME_CONTROLLER, ccval, msg->mval);
				midiPorts[ev.port()].device()->putEvent(ev);
				//audio->msgPlayMidiEvent(&ev);
			}/*}}}*/
		break;
		case MONITOR_MODIFY_CC:
		{
			m_editing = true;
			MidiAssignData* data = msg->track->midiAssign();
			if(isManagedInputController(msg->mval, msg->track->name()))
			{
				m_portccmap.remove(msg->mval, msg->track->name());
			}
			if(data->midimap[msg->ctl] != -1)
			{
				printf("Adding CC to portccmap %d, for controller: %d\n", data->midimap[msg->ctl], msg->ctl);
				m_portccmap.insert(data->midimap[msg->ctl], msg->track->name());
			}
			m_editing = false;
		}
		break;
		case MONITOR_MODIFY_PORT:
		{
			m_editing = true;
			MidiAssignData* data = msg->track->midiAssign();
			if(isManagedInputPort(msg->port, msg->track->name()))
			{
				m_inputports.remove(msg->port, msg->track->name());
			}
			m_inputports.insert(data->port,  msg->track->name());
			m_editing = false;
		}
		break;
		case MONITOR_ADD_TRACK:
		{
			m_editing = true;
			addMonitoredTrack(msg->track);
			m_editing = false;
		}
		break;
		case MONITOR_DEL_TRACK:
		{
			m_editing = true;
			deleteMonitoredTrack(msg->track);
			m_editing = false;
		}
		break;
		default:
			printf("MidiMonitor::processMsg1: Unknown message id\n");
		break;
	}
}/*}}}*/


void MidiMonitor::populateList()/*{{{*/
{
	m_editing = true;
	m_inputports.clear();
	m_outputports.clear();
	m_assignments.clear();
	m_portccmap.clear();
	for(ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
	{
		addMonitoredTrack((*t));
	}
	m_editing = false;
}/*}}}*/

void MidiMonitor::addMonitoredTrack(Track* t)
{
	MidiAssignData* data = t->midiAssign();
	m_assignments[t->name()] = data;
	m_inputports.insert(data->port,  t->name());
	QHashIterator<int, int> iter(data->midimap);
	while(iter.hasNext())
	{
		iter.next();
		if(iter.value() >= 0)
			m_portccmap.insert(iter.value(), t->name());
	}
	if(t->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)t;
		m_outputports.insert(track->outPort(), track->name());
	}
}

void MidiMonitor::deleteMonitoredTrack(Track* t)
{
	MidiAssignData* data = t->midiAssign();
	if(isAssigned(t->name()))
		m_assignments.remove(t->name());
	if(isManagedInputPort(data->port, t->name()))
		m_inputports.remove(data->port, t->name());
	QHashIterator<int, int> iter(data->midimap);
	while(iter.hasNext())
	{
		iter.next();
		if(iter.value() >= 0)
		{
			if(isManagedInputController(iter.value(), t->name()))
				m_portccmap.remove(iter.value(), t->name());
		}
	}
	if(t->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)t;
		if(isManagedOutputPort(track->outPort(), track->name()))
			m_outputports.remove(track->outPort(), track->name());
	}
}

bool MidiMonitor::isManagedController(int ctl)/*{{{*/
{
	bool rv = false;
	switch(ctl)
	{
		case CTRL_VOLUME:
		case CTRL_PANPOT:
		case CTRL_REVERB_SEND:
		case CTRL_CHORUS_SEND:
		case CTRL_VARIATION_SEND:
		case CTRL_RECORD:
		case CTRL_MUTE:
		case CTRL_SOLO:
			rv = true;
		break;
	}
	return rv;
}/*}}}*/
