
#ifndef _OOM_MIDIMON_
#define _OOM_MIDIMON_

#include "thread.h"
#include "mpevent.h"

#include <QHash>
#include <QMultiHash>
#include <QList>
#include <QTimer>

class Track;
class MidiTrack;
class MidiAssignData;
class QString;
class CCInfo;

enum { 
	MONITOR_AUDIO_OUT,	//Used to process outgoing midi from audio tracks
	MONITOR_MIDI_IN,	//Used to process incomming midi going to midi tracks/controllers
	MONITOR_MIDI_OUT,	//Used to process outgoing midi from midi tracks
	MONITOR_MIDI_OUT_EVENT,	//Used to process outgoing midi from midi tracks
	MONITOR_MODIFY_CC,
	MONITOR_DEL_CC,
	MONITOR_MODIFY_PORT,
	MONITOR_ADD_TRACK,
	MONITOR_DEL_TRACK,
	MONITOR_TOGGLE_FEEDBACK,
	MONITOR_LEARN,
	MONITOR_SEND_PRESET
};

enum FeedbackMode {
    FEEDBACK_MODE_READ = 0,
    FEEDBACK_MODE_WRITE,
    FEEDBACK_MODE_TOUCH,
    FEEDBACK_MODE_AUDITION
};

enum MonitorDataType {
	MIDI_LEARN = 0,
	MIDI_LEARN_NRPN,
	MIDI_INPUT
};

struct MonitorData
{
	MonitorDataType dataType;
	Track* track;
	int channel;
	int port;
	int controller;
	int value;
	int msb;
	int lsb;
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
	CCInfo* info;
};

struct LastMidiInMessage
{
    int port;
    int channel;
    int controller;
    int lastValue;
    unsigned lastTick;
};

struct LastFeedbackMessage
{
    int port;
    int channel;
    int controller;
    int value;
};

class MidiMonitor : public Thread 
{
	Q_OBJECT

	//Keep track if we are currently writing to the config
	//This is so we dont try to processing read/write on the containers
	//QHash/QList at the same time from different threads
	bool m_editing; 
	bool m_feedback;
	bool m_learning;
	int m_learnport;

    QList<LastMidiInMessage> m_lastMidiInMessages;
    QList<LastFeedbackMessage> m_lastFeedbackMessages;
    FeedbackMode m_feedbackMode;
    unsigned m_feedbackTimeout;

    bool updateNow;
    QTimer updateNowTimer;

	QMultiHash<int, QString> m_inputports;
	QMultiHash<int, QString> m_outputports;
	QHash<QString, MidiAssignData*> m_assignments; //list of managed assignments
	//Holds CCInfo/CC map for fast lookups at run time
	QMultiHash<int, CCInfo*> m_midimap;

    int fromThreadFdw, fromThreadFdr; // message pipes
    int sigFd; // pipe fd for messages to gui

	virtual void processMsg1(const void*);
	void addMonitoredTrack(Track*);
    void deleteMonitoredTrack(Track*);
    void updateLater();

    LastMidiInMessage* getLastMidiInMessage(int controller);
    LastMidiInMessage* getLastMidiInMessage(int port, int channel, int controller);
    void setLastMidiInMessage(int port, int channel, int controller, int value, unsigned tick);
    void deletePreviousMidiInEvents(MidiTrack* track, int controller, unsigned tick);

    LastFeedbackMessage* getLastFeedbackMessage(int port, int channel, int controller);
    void setLastFeedbackMessage(int port, int channel, int controller, int value);

public:
	MidiMonitor(const char* name);
	~MidiMonitor();

	virtual void start(int);

    void setFeedbackMode(FeedbackMode mode);
	FeedbackMode feedbackMode(){return m_feedbackMode;}

	void msgSendMidiInputEvent(MEvent&);
	void msgSendMidiOutputEvent(Track*,  int ctl, int val);
	void msgSendMidiOutputEvent(MidiPlayEvent ev);
	void msgSendAudioOutputEvent(Track*, int ctl, double val);
	void msgModifyTrackController(Track*, int ctl, CCInfo* cc);
	void msgDeleteTrackController(CCInfo* cc);
	void msgModifyTrackPort(Track*, int port);
	void msgAddMonitoredTrack(Track*);
	void msgDeleteMonitoredTrack(Track*);
	void msgToggleFeedback(bool);
	void msgStartLearning(int port);

	bool isFeedbackEnabled()
	{
		return m_feedback;
	}

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

	bool isManagedController(int);

	void populateList();

signals:
	void playMidiEvent(MidiPlayEvent*);

private slots:
    void updateSongNow();
    void songPlayChanged();
};

extern MidiMonitor *midiMonitor;
#endif
