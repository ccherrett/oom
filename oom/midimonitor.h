
#ifndef _OOM_MIDIMON_
#define _OOM_MIDIMON_

#include "thread.h"
#include "mpevent.h"

#include <QHash>
#include <QMultiHash>
#include <QList>

class Track;
class MidiAssignData;
class QString;

enum { 
	MONITOR_AUDIO_OUT,	//Used to process outgoing midi from audio tracks
	MONITOR_MIDI_IN,	//Used to process incomming midi going to midi tracks/controllers
	MONITOR_MIDI_OUT,	//Used to process outgoing midi from midi tracks
	MONITOR_MODIFY_CC,
	MONITOR_MODIFY_PORT,
	MONITOR_ADD_TRACK,
	MONITOR_DEL_TRACK,
	MONITOR_TOGGLE_FEEDBACK
};

//This container holds all the types that can be handled
//by thread this monitor
struct MonitorMsg : public ThreadMsg
{
	Track* track;
	int ctl;
	int port;
	double aval; //Audio value
	int mval;	//Midi value
	MidiAssignData* data;
	MEvent mevent;
	unsigned pos; //song position at time of the event
};

class MidiMonitor : public Thread 
{
	Q_OBJECT

	//Keep track if we are currently writing to the config
	//This is so we dont try to processing read/write on the containers
	//QHash/QList at the same time from different threads
	bool m_editing; 
	bool m_feedback;

	QMultiHash<int, QString> m_inputports;
	QMultiHash<int, QString> m_outputports;
	//Contains 0 - 127 with a list of tracks listening to this cc
	QMultiHash<int, QString> m_portccmap; 
	QHash<QString, MidiAssignData*> m_assignments; //list of managed assignments
    int fromThreadFdw, fromThreadFdr; // message pipe
    int sigFd; // pipe fd for messages to gui

	virtual void processMsg1(const void*);
	void addMonitoredTrack(Track*);
	void deleteMonitoredTrack(Track*);

public:
	MidiMonitor(const char* name);
	~MidiMonitor();

	virtual void start(int);

	void msgSendMidiInputEvent(MEvent&);
	void msgSendMidiOutputEvent(Track*,  int ctl, int val);
	void msgSendAudioOutputEvent(Track*, int ctl, double val);
	void msgModifyTrackController(Track*, int ctl, int cc);
	void msgModifyTrackPort(Track*, int port);
	void msgAddMonitoredTrack(Track*);
	void msgDeleteMonitoredTrack(Track*);
	void msgToggleFeedback(bool);

	bool isAssigned(QString track)
	{
		return !m_assignments.isEmpty() && m_assignments.contains(track);
	}

	bool isManagedInputPort(int port)
	{
		return !m_inputports.isEmpty() && m_inputports.contains(port);
	}

	bool isManagedInputPort(int port, QString track)
	{
		return !m_inputports.isEmpty() && m_inputports.contains(port, track);
	}

	bool isManagedOutputPort(int port)
	{
		return !m_outputports.isEmpty() && m_outputports.contains(port);
	}

	bool isManagedOutputPort(int port, QString track)
	{
		return !m_outputports.isEmpty() && m_outputports.contains(port, track);
	}

	bool isManagedInputController(int ctl)
	{
		return !m_portccmap.isEmpty() && m_portccmap.contains(ctl);
	}

	bool isManagedInputController(int ctl, QString track)
	{
		return !m_portccmap.isEmpty() && m_portccmap.contains(ctl, track);
	}

	bool isManagedController(int);

	void populateList();
signals:
	void playMidiEvent(MidiPlayEvent*);
};

extern MidiMonitor *midiMonitor;
#endif
