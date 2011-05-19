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
#include "ccinfo.h"

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
	m_learning = false;
	m_learnport = -1;

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
	if(!isRunning())
		return;
	MonitorMsg msg;
	msg.id = MONITOR_MIDI_IN;
	msg.mevent = event;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgSendMidiOutputEvent(Track* track,  int ctl, int val)/*{{{*/
{
	if(!isRunning())
		return;
	//printf("MidiMonitor::msgSendMidiOutputEvent \n");
	MonitorMsg msg;
	msg.id = MONITOR_MIDI_OUT;
	msg.track = track;
	msg.ctl = ctl;
	msg.mval = val;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgSendAudioOutputEvent(Track* track, int ctl, double val)/*{{{*/
{
	if(!isRunning())
		return;
	//printf("MidiMonitor::msgSendAudioOutputEvent\n");
	MonitorMsg msg;
	msg.id = MONITOR_AUDIO_OUT;
	msg.track = track;
	msg.ctl = ctl;
	msg.aval = val;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgModifyTrackController(Track* track, int ctl, CCInfo* cc)/*{{{*/
{
	if(!isRunning())
		return;
	if(track && cc)
	{
		MonitorMsg msg;
		msg.id = MONITOR_MODIFY_CC;
		msg.track = track;
		msg.ctl = ctl;
		msg.info = cc;
		
		sendMsg1(&msg, sizeof (msg));
	}
}/*}}}*/

void MidiMonitor::msgDeleteTrackController(CCInfo* cc)/*{{{*/
{
	if(!isRunning())
		return;
	if(cc)
	{
		MonitorMsg msg;
		msg.id = MONITOR_DEL_CC;
		msg.info = cc;
		
		sendMsg1(&msg, sizeof (msg));
	}
}/*}}}*/

void MidiMonitor::msgModifyTrackPort(Track* track,int port)/*{{{*/
{
	if(!isRunning())
		return;
	MonitorMsg msg;
	msg.id = MONITOR_MODIFY_PORT;
	msg.track = track;
	msg.port = port;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgAddMonitoredTrack(Track* track)/*{{{*/
{
	if(!isRunning())
		return;
	MonitorMsg msg;
	msg.id = MONITOR_ADD_TRACK;
	msg.track = track;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgDeleteMonitoredTrack(Track* track)/*{{{*/
{
	if(!isRunning())
		return;
	MonitorMsg msg;
	msg.id = MONITOR_ADD_TRACK;
	msg.track = track;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgToggleFeedback(bool f)/*{{{*/
{
	if(!isRunning())
		return;
	MonitorMsg msg;
	msg.id = MONITOR_TOGGLE_FEEDBACK;
	msg.mval = (int)f;
	
	sendMsg1(&msg, sizeof (msg));
}/*}}}*/

void MidiMonitor::msgStartLearning(int port)/*{{{*/
{
	if(!isRunning())
		return;
	MonitorMsg msg;
	msg.id = MONITOR_LEARN;
	msg.port = port;
	
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
		case MONITOR_MIDI_IN:	//Used to process incomming midi going to midi tracks/controllers
		{/*{{{*/
			//printf("MidiMonitor::processMsg1() event type:%d port:%d channel:%d CC:%d CCVal:%d \n",
			//	msg->mevent.type(), msg->mevent.port(), msg->mevent.channel(), msg->mevent.dataA(), msg->mevent.dataB());
			if(m_learning && m_learnport == msg->mevent.port())
			{
				QString cmd;
				cmd.append(QString::number(msg->mevent.port())).append(":");
				cmd.append(QString::number(msg->mevent.channel())).append(":");
				cmd.append(QString::number(msg->mevent.dataA())).append("$$");
				QByteArray ba(cmd.toUtf8().constData());
				write(sigFd, ba.constData(), 16);
				m_learning = false;
				m_learnport = -1;
				//TODO: Send an event back to the device
				//This will set toggle enable buttons to off and zero the slider as an indication of learning
				MidiPlayEvent ev(0, msg->mevent.port(), msg->mevent.channel(), ME_CONTROLLER, msg->mevent.dataA(), 0);
				midiPorts[ev.port()].device()->putEvent(ev);
				return;
			}
			if(isManagedInputPort(msg->mevent.port()))
			{
				//printf("MidiMonitor::processMsg1() Processing Midi Input\n");
				//QMultiHash<int, QString>::iterator i = m_portccmap.find(msg->mevent.dataA());
				QMultiHash<int, CCInfo*>::iterator iter = m_midimap.find(msg->mevent.dataA());
				while(iter != m_midimap.end())
				//while (i != m_portccmap.end()) 
				{
					CCInfo* info = iter.value();
					//QString tname(i.value());
					//MidiAssignData* data = m_assignments.value(tname);
					if(info && info->track()->midiAssign()->enabled && info->port() == msg->mevent.port() 
						&& info->channel() == msg->mevent.channel() && info->assignedControl() == msg->mevent.dataA())
					//if(data && data->enabled && data->port == msg->mevent.port() && data->channel == msg->mevent.channel() && !data->midimap.isEmpty())
					{
						//int ctl = data->midimap.key(msg->mevent.dataA());
						int ctl = info->controller();
						if(info->track())
						{
							if(info->recordOnly() && !info->track()->recordFlag())
							{
								++iter;
								continue;
							}
							switch(ctl)/*{{{*/
							{
								case CTRL_VOLUME:
								{
									if(info->track()->isMidiTrack())/*{{{*/
									//if(data->track->isMidiTrack())
									{
										MidiTrack* track = (MidiTrack*)info->track();
										//MidiTrack* track = (MidiTrack*)data->track;
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
										//printf("track volume\n");
										audio->msgSetVolume((AudioTrack*) info->track(), dbToTrackVol(midiToDb(msg->mevent.dataB())));
										((AudioTrack*) info->track())->startAutoRecord(AC_VOLUME, dbToTrackVol(midiToDb(msg->mevent.dataB())));
									}/*}}}*/
								}
								break;
								case CTRL_PANPOT:
								{
									if(info->track()->isMidiTrack())/*{{{*/
									{
										MidiTrack* track = (MidiTrack*)info->track();
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
										//printf("track pan\n");
										audio->msgSetPan(((AudioTrack*) info->track()), midiToTrackPan(msg->mevent.dataB()));
										((AudioTrack*) info->track())->recordAutomation(AC_PAN, midiToTrackPan(msg->mevent.dataB()));
									}/*}}}*/
								}
								break;
								case CTRL_RECORD:
									if(info->fakeToggle())
									{
										if(msg->mevent.dataB())
											song->setRecordFlag(info->track(), !info->track()->recordFlag(), true);
									}
									else
										song->setRecordFlag(info->track(), msg->mevent.dataB() ? true : false, true);
								break;
								case CTRL_MUTE:
									if(info->fakeToggle())
									{
										if(msg->mevent.dataB())
											info->track()->setMute(!info->track()->isMute(), true);
									}
									else
										info->track()->setMute(msg->mevent.dataB() ? true : false, true);
									song->update(SC_MUTE);
								break;
								case CTRL_SOLO:
									if(info->fakeToggle())
									{
										if(msg->mevent.dataB())
											info->track()->setSolo(!info->track()->solo(), true);
									}
									else
										info->track()->setSolo(msg->mevent.dataB() ? true : false, true);
									song->update(SC_SOLO);
								break;
								default:
								//case CTRL_REVERB_SEND:
								//case CTRL_CHORUS_SEND:
								//case CTRL_VARIATION_SEND:
								{
									if(info->track()->isMidiTrack())
									{
										MidiTrack* track = (MidiTrack*)info->track();
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
					++iter;
				}//END while loop
			}
		}/*}}}*/
		break;
		case MONITOR_AUDIO_OUT:	//Used to process outgoing midi from audio tracks
			//printf("MidiMonitor::processMsg1() Audio Output\n");
			if(!m_feedback)
				return;
			if(msg->track /*&& isAssigned(msg->track->name())*/) /*{{{*/
			{
				MidiAssignData* data = m_assignments.value(msg->track->name());
				if(/*!isManagedController(msg->ctl) || */!data->enabled || data->midimap.isEmpty())
					return;
				CCInfo *info = data->midimap.value(msg->ctl);
				if(info && info->assignedControl() >= 0)
				{
					//int ccval = info->assignedControl();
					//if(ccval < 0)
					//	return;
					int val = 0;
					if(msg->ctl == CTRL_PANPOT)
						val = trackPanToMidi(msg->aval);//, midiToTrackPan(trackPanToMidi(val)));
					else
						val = dbToMidi(trackVolToDb(msg->aval));
					//printf("Sending midivalue from audio track: %d\n", val);
					//TODO: Check if feedback is required before bothering with this
					MidiPlayEvent ev(0, info->port(), info->channel(), ME_CONTROLLER, info->assignedControl(), val);
					midiPorts[ev.port()].device()->putEvent(ev);
				}
				//audio->msgPlayMidiEvent(&ev);
			}/*}}}*/
		break;
		case MONITOR_MIDI_OUT:	//Used to process outgoing midi from midi tracks
			//printf("MidiMonitor::processMsg1() Midi Output\n");
			if(!m_feedback)
				return;
			if(msg->track/* && isAssigned(msg->track->name())*/)/*{{{*/
			{
				MidiAssignData* data = msg->track->midiAssign();//m_assignments.value(msg->track->name());
				if(/*!isManagedController(msg->ctl) || */!data->enabled || data->midimap.isEmpty())
					return;
				CCInfo* info = data->midimap.value(msg->ctl);
				if(info && info->assignedControl() >= 0)
				{
					//int ccval = data->midimap.value(msg->ctl);
					//if(ccval < 0)
					//	return;
					//printf("Sending midivalue from audio track: %d\n", msg->mval);
					//TODO: Check if feedback is required before bothering with this
					MidiPlayEvent ev(0, info->port(), info->channel(), ME_CONTROLLER, info->assignedControl(), msg->mval);
					midiPorts[ev.port()].device()->putEvent(ev);
				}
			}/*}}}*/
		break;
		case MONITOR_MODIFY_CC:
		{/*{{{*/
			m_editing = true;
			//MidiAssignData* data = msg->track->midiAssign();
			if(!m_midimap.isEmpty() && m_midimap.contains(msg->ctl, msg->info))
			{
				m_midimap.remove(msg->ctl, msg->info);
			}
			if(msg->info->assignedControl() >= 0)
				m_midimap.insert(msg->info->assignedControl(), msg->info);
			//if(isManagedInputController(msg->mval, msg->track->name()))
			//{
			//	m_portccmap.remove(msg->mval, msg->track->name());
			//}
			//if(data->midimap[msg->ctl] != -1)
			//{
			//	printf("Adding CC to portccmap %d, for controller: %d\n", data->midimap[msg->ctl], msg->ctl);
			//	m_portccmap.insert(data->midimap[msg->ctl], msg->track->name());
			//}
			m_editing = false;
		}/*}}}*/
		break;
		case MONITOR_DEL_CC:
		{/*{{{*/
			m_editing = true;
			if(!m_midimap.isEmpty() && m_midimap.contains(msg->info->assignedControl(), msg->info))
			{
				m_midimap.remove(msg->info->assignedControl(), msg->info);
			}
			m_editing = false;
		}/*}}}*/
		break;
		case MONITOR_MODIFY_PORT:
		{/*{{{*/
			m_editing = true;
			MidiAssignData* data = msg->track->midiAssign();
			if(isManagedInputPort(msg->port, msg->track->name()))
			{
				m_inputports.remove(msg->port, msg->track->name());
			}
			m_inputports.insert(data->port,  msg->track->name());
			m_editing = false;
		}/*}}}*/
		break;
		case MONITOR_ADD_TRACK:
		{/*{{{*/
			m_editing = true;
			addMonitoredTrack(msg->track);
			m_editing = false;
		}/*}}}*/
		break;
		case MONITOR_DEL_TRACK:
		{/*{{{*/
			m_editing = true;
			deleteMonitoredTrack(msg->track);
			m_editing = false;
		}/*}}}*/
		break;
		case MONITOR_TOGGLE_FEEDBACK:
		{
			m_feedback = (bool)msg->mval;
		}
		break;
		case MONITOR_LEARN:
		{
			m_learning = true;
			m_learnport = msg->port;
		}
		break;
		default:
			printf("MidiMonitor::processMsg1: Unknown message id\n");
		break;
	}
}/*}}}*/

//Private members
void MidiMonitor::populateList()/*{{{*/
{
	m_editing = true;
	m_inputports.clear();
	m_outputports.clear();
	m_assignments.clear();
	//m_portccmap.clear();
	for(ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
	{
		addMonitoredTrack((*t));
	}
	m_editing = false;
}/*}}}*/

void MidiMonitor::addMonitoredTrack(Track* t)/*{{{*/
{
	MidiAssignData* data = t->midiAssign();
	m_assignments[t->name()] = data;
	m_inputports.insert(data->port,  t->name());
	QHashIterator<int, CCInfo*> iter(data->midimap);
	while(iter.hasNext())
	{
		iter.next();
		CCInfo* info = iter.value();
		if(info && info->assignedControl() >= 0)
		{
			//m_portccmap.insert(info->assignedControl(), t->name());
			m_midimap.insert(info->assignedControl(), info);
		}
	}
	if(t->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)t;
		m_outputports.insert(track->outPort(), track->name());
	}
}/*}}}*/

void MidiMonitor::deleteMonitoredTrack(Track* t)/*{{{*/
{
	MidiAssignData* data = t->midiAssign();
	if(isAssigned(t->name()))
		m_assignments.remove(t->name());
	if(isManagedInputPort(data->port, t->name()))
		m_inputports.remove(data->port, t->name());
	QHashIterator<int, CCInfo*> iter(data->midimap);
	while(iter.hasNext())
	{
		iter.next();
		CCInfo* info = iter.value();
		if(info && info->assignedControl() >= 0)
		{
			//if(isManagedInputController(info->assignedControl(), t->name()))
			//	m_portccmap.remove(info->assignedControl(), t->name());
			m_midimap.remove(info->assignedControl(), info);
		}
	}
	if(t->isMidiTrack())
	{
		MidiTrack* track = (MidiTrack*)t;
		if(isManagedOutputPort(track->outPort(), track->name()))
			m_outputports.remove(track->outPort(), track->name());
	}
}/*}}}*/

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
