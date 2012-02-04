//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_TRACK_MANAGER_
#define _OOM_TRACKS_MANAGER_

#include <QMap>
#include <QString>
#include <QPair>
#include "utils.h"

struct VirtualTrack {

	VirtualTrack()
	{
		id = create_id();
		type = -1;
		useOutput = false;
		useInput = false;
		useBuss = false;
		useMonitor = false;
		inputChannel = -1;
		outputChannel = -1;
		instrumentType = -1;
		autoCreateInstrument = false;
		createMidiInputDevice = false;
		createMidiOutputDevice = false;
	}
	qint64 id;
	int type;
	QString name;
	QString instrumentName;
	int instrumentType;
	bool autoCreateInstrument;
	bool createMidiInputDevice;
	bool createMidiOutputDevice;
	bool useInput;
	bool useOutput;
	bool useMonitor;
	bool useBuss;
	int inputChannel;
	int outputChannel;
	QPair<int, QString> inputConfig;
	QPair<int, QString> outputConfig;
	QPair<int, QString> instrumentConfig;
	QPair<int, QString> monitorConfig;
    QPair<int, QString> monitorConfig2;
	QPair<int, QString> bussConfig;
};

class TrackManager : public QObject{
	Q_OBJECT

	int m_insertPosition;
	
	int m_allChannelBit;

	int m_midiInPort;
	int m_midiOutPort;
	
	QMap<int, QString> m_currentMidiInputList;
	QMap<int, QString> m_currentMidiOutputList;
	
	int getFreeMidiPort();
	
	void createMonitorInputTracks(VirtualTrack*);
	void removeMonitorInputTracks(VirtualTrack*);
	bool loadInstrument(VirtualTrack*);
	bool unloadInstrument(VirtualTrack*);

signals:
	void trackAdded(qint64);

public:
	enum { LS_INSTRUMENT, SYNTH_INSTRUMENT, GM_INSTRUMENT};
	TrackManager();
	~TrackManager(){}
	void setPosition(int);
	qint64 addTrack(VirtualTrack*);
	bool removeTrack(VirtualTrack*);
};

#endif
