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

struct OOVirtualTrack {

	OOVirtualTrack()
	{
		type = -1;
		useOutput = false;
		useInput = false;
		useBuss = false;
		useMonitor = false;
		inputChannel = -1;
		outputChannel = -1;
		autoCreateInstrument = false;
		createMidiInputDevice = false;
		createMidiOutputDevice = false;
	}
	int type;
	QString name;
	QString instrumentName;
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
	QPair<int, QString> bussConfig;
};

class OOTrackManager : public QObject{
	Q_OBJECT

	int m_insertPosition;
	
	int m_allChannelBit;

	int m_midiInPort;
	int m_midiOutPort;
	
	QMap<int, QString> m_currentMidiInputList;
	QMap<int, QString> m_currentMidiOutputList;
	
	int getFreeMidiPort();
	
	void createMonitorInputTracks(OOVirtualTrack*);
	bool loadInstrument(OOVirtualTrack*);

signals:
	void trackAdded(QString);

public:
	OOTrackManager();
	~OOTrackManager(){}
	void setPosition(int);
	bool addTrack(OOVirtualTrack*);
};

#endif
