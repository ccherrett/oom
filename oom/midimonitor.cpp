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
#include "utils.h"
#include "midimonitor.h"
#include "ccinfo.h"
#include "gconfig.h"

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
    m_feedbackMode = FEEDBACK_MODE_READ;
    m_feedbackTimeout = 0;

    updateNow = false;
    updateNowTimer.setInterval(50);
    updateNowTimer.start();

    connect(&updateNowTimer, SIGNAL(timeout()), this, SLOT(updateSongNow()));
    connect(song, SIGNAL(playChanged(bool)), this, SLOT(songPlayChanged()));

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

void MidiMonitor::setFeedbackMode(FeedbackMode mode)
{
    //qWarning("Feedback mode changed %i", mode);

    m_feedbackMode = mode;
}

void MidiMonitor::updateLater()
{
    updateNow = true;
}

void MidiMonitor::updateSongNow()
{
    bool updateEvents;

    if (m_feedback)
    {
        switch(m_feedbackMode)
        {
        case FEEDBACK_MODE_READ:
            updateEvents = false;
            break;
        case FEEDBACK_MODE_WRITE:
        case FEEDBACK_MODE_TOUCH:
        case FEEDBACK_MODE_AUDITION:
            m_feedbackTimeout += 1;
            if (m_feedbackTimeout == 6) // 6x50ms = 300ms
            {
                m_feedbackTimeout = 0;
                updateEvents = true;
            }
            break;
        }
    }
    else
        updateEvents = true;

    if (updateEvents)
    {
        unsigned tick = song->cpos();

        //qWarning("Audition test - %i", m_lastFeedbackMessages.count());

        for (int i=0; i < m_lastFeedbackMessages.count(); i++)
        {
            LastFeedbackMessage* msg = &m_lastFeedbackMessages[i];
            LastMidiInMessage* lastMsg = getLastMidiInMessage(msg->controller);

            if (lastMsg)
            {
                // ignore message if in write mode
                if (m_feedback && m_feedbackMode == FEEDBACK_MODE_WRITE)
                    continue;

                // ignore message if last midi-in was less than 384 ticks ago
                if (lastMsg->lastTick <= tick && tick - lastMsg->lastTick < 384)
                    continue;

                // if message > 384 ticks and audition is on, stop playback
                if (m_feedback && m_feedbackMode == FEEDBACK_MODE_AUDITION)
                {
                    //qWarning("Audition stop!");
                    song->stopRolling();
                    song->setPos(Song::CPOS, Pos(lastMsg->lastTick, true), true, true, true);
                    break;
                }
            }

            //qWarning("Audition continue");

            MidiPlayEvent ev(0, msg->port, msg->channel, ME_CONTROLLER, msg->controller, msg->value);
            ev.setEventSource(MonitorSource);
            midiPorts[ev.port()].device()->putEvent(ev);
        }
    }

    if (updateNow)
    {
        updateNow = false;
        song->update(SC_EVENT_INSERTED);
    }
}

void MidiMonitor::songPlayChanged()
{
    //qWarning("songPlayChanged()");
    m_lastMidiInMessages.clear();
    m_lastFeedbackMessages.clear();
    m_feedbackTimeout = 0;
}

LastMidiInMessage* MidiMonitor::getLastMidiInMessage(int controller)
{
    for (int i=0; i < m_lastMidiInMessages.count(); i++)
    {
        LastMidiInMessage* msg = &m_lastMidiInMessages[i];
        if (msg->controller == controller)
            return msg;
    }
    return 0;
}

LastMidiInMessage* MidiMonitor::getLastMidiInMessage(int port, int channel, int controller)
{
    for (int i=0; i < m_lastMidiInMessages.count(); i++)
    {
        LastMidiInMessage* msg = &m_lastMidiInMessages[i];
        if (msg->port == port && msg->channel == channel && msg->controller == controller)
            return msg;
    }
    return 0;
}

void MidiMonitor::setLastMidiInMessage(int port, int channel, int controller, int value, unsigned tick)
{
    for (int i=0; i < m_lastMidiInMessages.count(); i++)
    {
        LastMidiInMessage* msg = &m_lastMidiInMessages[i];
        if (msg->port == port && msg->channel == channel && msg->controller == controller)
        {
            msg->lastValue = value;
            msg->lastTick = tick;
            return;
        }
    }

    LastMidiInMessage newMsg;
    newMsg.channel = channel;
    newMsg.port = port;
    newMsg.controller = controller;
    newMsg.lastValue = value;
    newMsg.lastTick = tick;

    m_lastMidiInMessages.append(newMsg);
}

void MidiMonitor::deletePreviousMidiInEvents(MidiTrack* track, int controller, unsigned tick)
{
    LastMidiInMessage* lastMsg = getLastMidiInMessage(track->outPort(), track->outChannel(), controller);

    if (lastMsg && lastMsg->lastTick > 0 && lastMsg->lastTick < tick && tick - lastMsg->lastTick < 384)
    {
        PartList* pl = track->parts();

        if (pl && pl->empty() == false)
        {
            Part* part = pl->findAtTick(lastMsg->lastTick);

            if (part)
            {
                QList<unsigned> ticksToReplace;
                const EventList* el = part->cevents();

                for (ciEvent ce = el->begin(); ce != el->end(); ++ce)
                {
                    const Event& ev = ce->second;

                    if (ev.type() == Controller && ev.dataA() == controller)
                        ticksToReplace.append(ev.tick());
                }

                unsigned iTick, lenTick = part->lenTick();

                for (int i=0; i < ticksToReplace.count(); i++)
                {
                    iTick = ticksToReplace.at(i);

                    if (iTick < lastMsg->lastTick)
                        continue;
                    if (iTick > tick)
                        continue;
                    if (iTick > lenTick)
                        continue;

                    Event event(Controller);
                    event.setTick(iTick);
                    event.setA(controller);
                    event.setB(lastMsg->lastValue);
                    audio->msgAddEventCheck(track, event, false, true, false, false);
                }
            }
        }
    }
}

LastFeedbackMessage* MidiMonitor::getLastFeedbackMessage(int port, int channel, int controller)
{
    for (int i=0; i < m_lastFeedbackMessages.count(); i++)
    {
        LastFeedbackMessage* msg = &m_lastFeedbackMessages[i];
        if (msg->port == port && msg->channel == channel && msg->controller == controller)
            return msg;
    }
    return 0;
}

void MidiMonitor::setLastFeedbackMessage(int port, int channel, int controller, int value)
{
    //qWarning("setLastFeedbackMessage();");

    for (int i=0; i < m_lastFeedbackMessages.count(); i++)
    {
        LastFeedbackMessage* msg = &m_lastFeedbackMessages[i];
        if (msg->port == port && msg->channel == channel && msg->controller == controller)
        {
            msg->value = value;
            return;
        }
    }

    LastFeedbackMessage newMsg;
    newMsg.channel = channel;
    newMsg.port = port;
    newMsg.controller = controller;
    newMsg.value = value;

    m_lastFeedbackMessages.append(newMsg);
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

void MidiMonitor::msgSendMidiOutputEvent(MidiPlayEvent ev)/*{{{*/
{
	if(!isRunning())
		return;
	//printf("MidiMonitor::msgSendMidiOutputEvent \n");
	MonitorMsg msg;
	msg.id = MONITOR_MIDI_OUT_EVENT;
	msg.mevent = ev;
	
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
	//if(m_editing)
	//	return;
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
				int a = msg->mevent.dataA();
				//int b = msg->mevent.dataB();

				if (a < CTRL_14_OFFSET)//{{{
				{ // 7 Bit Controller
					MonitorData mdata;
					mdata.dataType = MIDI_LEARN;
					mdata.port = msg->mevent.port();
					mdata.channel = msg->mevent.channel();
					mdata.controller = a;
					write(sigFd, &mdata, sizeof (MonitorData));
					/*QString cmd;
					cmd.append(QString::number(msg->mevent.port())).append(":");
					cmd.append(QString::number(msg->mevent.channel())).append(":");
					cmd.append(QString::number(a)).append("$$");
					QByteArray ba(cmd.toUtf8().constData());
					write(sigFd, ba.constData(), 16);*/
				}
				else if (a < CTRL_RPN_OFFSET)
				{ // 14 bit high resolution controller
					printf("Controller 14bit unsupported\n");
					//int ctrlH = (a >> 8) & 0x7f;
					//int ctrlL = a & 0x7f;
					//int dataH = (b >> 7) & 0x7f;
					//int dataL = b & 0x7f;
				}
				else if (a > CTRL_RPN_OFFSET && a < CTRL_NRPN_OFFSET)
				{ // RPN 7-Bit Controller
					printf("RPN 7 Controller unsupported\n");
					//int ctrlH = (a >> 8) & 0x7f;
					//int ctrlL = a & 0x7f;
				}
				else if (/*a >= CTRL_NRPN_OFFSET && */a < CTRL_INTERNAL_OFFSET)
				{ // NRPN 7-Bit Controller
					printf("NRPN 7 Controller\n");
					int msb = (a >> 8) & 0x7f;
					int lsb = a & 0x7f;
					MonitorData mdata;
					mdata.dataType = MIDI_LEARN_NRPN;
					mdata.port = msg->mevent.port();
					mdata.channel = msg->mevent.channel();
					mdata.msb = msb;
					mdata.lsb = lsb;
					write(sigFd, &mdata, sizeof (MonitorData));
					/*QString cmd;
					cmd.append(QString::number(msg->mevent.port())).append(":");
					cmd.append(QString::number(msg->mevent.channel())).append(":");
					cmd.append(QString::number(msb)).append(":");
					cmd.append(QString::number(lsb)).append(":").append("N").append("$$");
					QByteArray ba(cmd.toUtf8().constData());
					write(sigFd, ba.constData(), 16);*/
				}
				else if (a < CTRL_NRPN14_OFFSET)
				{ // RPN14 Controller
					printf("RPN 14 Controller unsupported\n");
					//int ctrlH = (a >> 8) & 0x7f;
					//int ctrlL = a & 0x7f;
					//int dataH = (b >> 7) & 0x7f;
					//int dataL = b & 0x7f;
				}
				else if (a < CTRL_NONE_OFFSET)
				{ // NRPN14 Controller
					//int ctrlH = (a >> 8) & 0x7f;
					//int ctrlL = a & 0x7f;
					//int dataH = (b >> 7) & 0x7f;
					//int dataL = b & 0x7f;
					printf("NRPN 14 Controller unsupported\n");//: msb: %d, lsb: %d, datamsb: %d, datalsb: %d\n", ctrlH, ctrlL, dataH, dataL);
				} //}}}
				m_learning = false;
				m_learnport = -1;
				//TODO: Send an event back to the device
				//This will set toggle enable buttons to off and zero the slider as an indication of learning
				//We need a way to know if this is a button or not, maybe send another field from the gui
				//since we know in the gui if there is a toggle or not.
				MidiPlayEvent ev(0, msg->mevent.port(), msg->mevent.channel(), ME_CONTROLLER, msg->mevent.dataA(), 0);
				ev.setEventSource(MonitorSource);
				midiPorts[ev.port()].device()->putEvent(ev);
				return;
			}
			if(isManagedInputPort(msg->mevent.port()))
			{
                if (m_feedback && m_feedbackMode == FEEDBACK_MODE_READ)
                    return;

				//printf("MidiMonitor::processMsg1() Processing Midi Input\n");
				QMultiHash<int, CCInfo*>::iterator iter = m_midimap.find(msg->mevent.dataA());
				while(iter != m_midimap.end())
				{
					CCInfo* info = iter.value();
					if(info && info->track()->midiAssign()->enabled && info->port() == msg->mevent.port() 
						&& info->channel() == msg->mevent.channel() && info->assignedControl() == msg->mevent.dataA())
					{
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
								case CTRL_AUX1:
									//Midi tracks dont have Aux Sends
									if(!info->track()->isMidiTrack())/*{{{*/
									{
										QHash<int, qint64>* auxMap = ((AudioTrack*) info->track())->auxControlList();
										if(!auxMap->isEmpty() && auxMap->contains(CTRL_AUX1))
										{
											//printf("auxSend\n");
											((AudioTrack*) info->track())->setAuxSend(auxMap->value(CTRL_AUX1), dbToTrackVol(midiToDb(msg->mevent.dataB())), true);
											song->update(SC_AUX);
										}
									}/*}}}*/
								break;
								case CTRL_AUX2:
									//Midi tracks dont have Aux Sends
									if(!info->track()->isMidiTrack())/*{{{*/
									{
										QHash<int, qint64>* auxMap = ((AudioTrack*) info->track())->auxControlList();
										if(!auxMap->isEmpty() && auxMap->contains(CTRL_AUX2))
										{
											//printf("auxSend\n");
											((AudioTrack*) info->track())->setAuxSend(auxMap->value(CTRL_AUX2), dbToTrackVol(midiToDb(msg->mevent.dataB())), true);
											song->update(SC_AUX);
										}
									}/*}}}*/
								break;
								case CTRL_AUX3:
									//Midi tracks dont have Aux Sends
									if(!info->track()->isMidiTrack())/*{{{*/
									{
										QHash<int, qint64>* auxMap = ((AudioTrack*) info->track())->auxControlList();
										if(!auxMap->isEmpty() && auxMap->contains(CTRL_AUX3))
										{
											//printf("auxSend\n");
											((AudioTrack*) info->track())->setAuxSend(auxMap->value(CTRL_AUX3), dbToTrackVol(midiToDb(msg->mevent.dataB())), true);
											song->update(SC_AUX);
										}
									}/*}}}*/
								break;
								case CTRL_AUX4:
									//Midi tracks dont have Aux Sends
									if(!info->track()->isMidiTrack())/*{{{*/
									{
										QHash<int, qint64>* auxMap = ((AudioTrack*) info->track())->auxControlList();
										if(!auxMap->isEmpty() && auxMap->contains(CTRL_AUX4))
										{
											//printf("auxSend\n");
											((AudioTrack*) info->track())->setAuxSend(auxMap->value(CTRL_AUX4), dbToTrackVol(midiToDb(msg->mevent.dataB())), true);
											song->update(SC_AUX);
										}
									}/*}}}*/
								break;
								case CTRL_VOLUME:
								{
									//FIXME: For NRPN Support this needs to take into account the Controller type
									if(info->track()->isMidiTrack())/*{{{*/
									//if(data->track->isMidiTrack())
									{
										MidiTrack* track = (MidiTrack*)info->track();
										//MidiTrack* track = (MidiTrack*)data->track;
										MonitorData mdata;
										mdata.track = info->track();
										mdata.dataType = MIDI_INPUT;
										mdata.port = track->outPort();
										mdata.channel = track->outChannel();
										mdata.controller = ctl;
										mdata.value = msg->mevent.dataB();

										if(track && track->recordFlag() && audio->isPlaying())
										{
                                            unsigned tick = song->cpos();

                                            // Check for previous events to delete
                                            deletePreviousMidiInEvents(track, ctl, tick);

											Event event(Controller);
											event.setTick(tick);
											event.setA(mdata.controller);
											event.setB(mdata.value);
											//XXX: Buffer this event instead of adding now
                                            audio->msgAddEventCheck(track, event, false, true, false, false);

                                            setLastMidiInMessage(track->outPort(), track->outChannel(), mdata.controller, mdata.value, tick);
                                            updateLater();

											/*PartList* pl = track->parts();
											if(pl && !pl->empty())
											{
												Part* part = pl->findAtTick(tick);
												if(part)
												{
													//printf("Song::processMonitorMessage add event to part\n");
													Event event(Controller);
													event.setTick(tick);
													event.setA(mdata.controller);
													event.setB(mdata.value);
													song->recordEvent((MidiPart*)part, event);
												}
											}*/
										}
										write(sigFd, &mdata, sizeof (MonitorData));
										/*QString cmd;
										//cmd.append(QString::number(msg->mevent.time())).append(":");
										cmd.append(QString::number(track->outPort())).append(":");
										cmd.append(QString::number(track->outChannel())).append(":");
										cmd.append(QString::number(ctl)).append(":");
										cmd.append(QString::number(msg->mevent.dataB())).append("$$");
										QByteArray ba(cmd.toUtf8().constData());
										//printf("ByteArray size in MidiMonitor: %d\n", ba.size());
										write(sigFd, ba.constData(), 16);*/
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
										MonitorData mdata;
										mdata.track = info->track();
										mdata.dataType = MIDI_INPUT;
										mdata.port = track->outPort();
										mdata.channel = track->outChannel();
										mdata.controller = ctl;
										mdata.value = msg->mevent.dataB();
										if(track && track->recordFlag())
										{
											unsigned tick = song->cpos();

                                            // Check for previous events to delete
                                            deletePreviousMidiInEvents(track, ctl, tick);

											Event event(Controller);
											event.setTick(tick);
											event.setA(mdata.controller);
											event.setB(mdata.value);
                                            audio->msgAddEventCheck(track, event, false, true, false, false);

                                            setLastMidiInMessage(track->outPort(), track->outChannel(), mdata.controller, mdata.value, tick);
                                            updateLater();

											/*PartList* pl = track->parts();
											if(pl && !pl->empty())
											{
												Part* part = pl->findAtTick(tick);
												if(part)
												{
													//printf("Song::processMonitorMessage add event to part\n");
													Event event(Controller);
													event.setTick(tick);
													event.setA(mdata.controller);
													event.setB(mdata.value);
													song->recordEvent((MidiPart*)part, event);
												}
											}*/
										}
										write(sigFd, &mdata, sizeof (MonitorData));
										/*QString cmd;
										//cmd.append(QString::number(msg->mevent.time())).append(":");
										cmd.append(QString::number(track->outPort())).append(":");
										cmd.append(QString::number(track->outChannel())).append(":");
										cmd.append(QString::number(ctl)).append(":");
										cmd.append(QString::number(msg->mevent.dataB())).append("$$");
										QByteArray ba(cmd.toUtf8().constData());
										//printf("ByteArray size in MidiMonitor: %d\n", ba.size());
										write(sigFd, ba.constData(), 16);*/
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
								{
									if(info->track()->isMidiTrack())
									{
										MidiTrack* track = (MidiTrack*)info->track();
										MonitorData mdata;
										mdata.track = info->track();
										mdata.dataType = MIDI_INPUT;
										mdata.port = track->outPort();
										mdata.channel = track->outChannel();
										mdata.controller = ctl;
										mdata.value = msg->mevent.dataB();
										if(track && track->recordFlag())
										{
											unsigned tick = song->cpos();

                                            // Check for previous events to delete
                                            deletePreviousMidiInEvents(track, ctl, tick);

											Event event(Controller);
											event.setTick(tick);
											event.setA(mdata.controller);
											event.setB(mdata.value);
                                            audio->msgAddEventCheck(track, event, false, true, false, false);

                                            setLastMidiInMessage(track->outPort(), track->outChannel(), mdata.controller, mdata.value, tick);
                                            updateLater();

											/*PartList* pl = track->parts();
											if(pl && !pl->empty())
											{
												Part* part = pl->findAtTick(tick);
												if(part)
												{
													//printf("Song::processMonitorMessage add event to part\n");
													Event event(Controller);
													event.setTick(tick);
													event.setA(mdata.controller);
													event.setB(mdata.value);
													song->recordEvent((MidiPart*)part, event);
												}
											}*/
										}
										write(sigFd, &mdata, sizeof (MonitorData));
										/*QString cmd;
										//cmd.append(QString::number(msg->mevent.time())).append(":");
										cmd.append(QString::number(track->outPort())).append(":");
										cmd.append(QString::number(track->outChannel())).append(":");
										cmd.append(QString::number(ctl)).append(":");
										cmd.append(QString::number(msg->mevent.dataB())).append("$$");
										QByteArray ba(cmd.toUtf8().constData());
										//printf("ByteArray size in MidiMonitor: %d\n", ba.size());
										write(sigFd, ba.constData(), 16);*/
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
					ev.setEventSource(MonitorSource);
					midiPorts[ev.port()].device()->putEvent(ev);
				}
				//audio->msgPlayMidiEvent(&ev);
			}/*}}}*/
		break;
		case MONITOR_MIDI_OUT_EVENT:	//Used to process outgoing midi from midi tracks
		{
			//printf("MidiMonitor::processMsg1() Midi Output\n");
			if(!m_feedback)
				return;
			Track* track = msg->mevent.track();
			if(track/* && isAssigned(msg->track->name())*/)/*{{{*/
			{
				MidiAssignData* data = track->midiAssign();//m_assignments.value(msg->track->name());
				if(/*!isManagedController(msg->ctl) || */!data->enabled || data->midimap.isEmpty())
					return;
				CCInfo* info = data->midimap.value(msg->mevent.dataA());
				if(info && info->assignedControl() >= 0)
				{
					//int ccval = data->midimap.value(msg->ctl);
					//if(ccval < 0)
					//	return;
					//printf("Sending midivalue from audio track: %d\n", msg->mval);
					//TODO: Check if feedback is required before bothering with this

                    if (m_feedbackMode == FEEDBACK_MODE_READ)
                    {
                        MidiPlayEvent ev(0, info->port(), info->channel(), ME_CONTROLLER, info->assignedControl(), msg->mevent.dataB());
                        ev.setEventSource(MonitorSource);
                        midiPorts[ev.port()].device()->putEvent(ev);
                    }
                    else
                    {
                        setLastFeedbackMessage(info->port(), info->channel(), info->assignedControl(), msg->mevent.dataB());
                        updateLater();
                    }
				}
			}/*}}}*/
		}
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

                    if (m_feedbackMode == FEEDBACK_MODE_READ)
                    {
                        MidiPlayEvent ev(0, info->port(), info->channel(), ME_CONTROLLER, info->assignedControl(), msg->mevent.dataB());
                        ev.setEventSource(MonitorSource);
                        midiPorts[ev.port()].device()->putEvent(ev);
                    }
                    else
                    {
                        setLastFeedbackMessage(info->port(), info->channel(), info->assignedControl(), msg->mevent.dataB());
                        updateLater();
                    }
                }
			}/*}}}*/
		break;
		case MONITOR_MODIFY_CC:
		{/*{{{*/
			m_editing = true;
			if(!m_midimap.isEmpty() && m_midimap.contains(msg->ctl, msg->info))
			{
				m_midimap.remove(msg->ctl, msg->info);
			}
			if(msg->info->assignedControl() >= 0)
				m_midimap.insert(msg->info->assignedControl(), msg->info);
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
