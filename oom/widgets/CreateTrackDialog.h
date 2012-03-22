//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_CREATE_TRACKS_DIALOG_
#define _OOM_CREATE_TRACKS_DIALOG_

#include "ui_createtrackbase.h"
#include <QMap>
#include "TrackManager.h"

#define CTDInstrumentTypeRole Qt::UserRole+4
#define CTDInstrumentNameRole Qt::UserRole+5
#define CTDDeviceTypeRole Qt::UserRole+6
#define CTDDeviceNameRole Qt::UserRole+7

class QShowEvent;
class QSize;
class Knob;
class DoubleLabel;

class CreateTrackDialog : public QDialog, public Ui::CreateTrackBase {
	Q_OBJECT

	int m_height;
	int m_width;

	int m_insertType;
	int m_insertPosition;
	bool m_templateMode;
	
	bool m_createMidiInputDevice;
	bool m_createMidiOutputDevice;
	bool m_midiSameIO;
	
	int m_allChannelBit;

	int m_midiInPort;
	int m_midiOutPort;
	
	bool m_createTrackOnly;
	int m_showJackAliases;
	bool m_instrumentLoaded;
	int m_instrumentMap;
	bool m_existingMap;
	
	QMap<int, QString> m_currentMidiInputList;
	QMap<int, QString> m_currentMidiOutputList;
	QStringList m_currentInput;
	QStringList m_currentOutput;

	VirtualTrack *m_vtrack;

	Knob* m_panKnob;
	DoubleLabel* m_panLabel;
	Knob* m_auxKnob;
	DoubleLabel* m_auxLabel;
    
	//SynthPluginDevice *m_lastSynth;

	void importInputs();
	void importOutputs();

	void populateInputList();
	void populateOutputList();
	void populateNewInputList();
	void populateNewOutputList();
	void populateInstrumentList();
	void populateMonitorList();
	void populateBussList();
	
	void updateVisibleElements();
	
	void initDefaults();
	void cleanup();

private slots:
	void addTrack();
	void cancelSelected();
	void updateInputSelected(bool);
	void updateOutputSelected(bool);
	void updateBussSelected(bool);
	void updateInstrument(int);
	void trackTypeChanged(int);
	void trackNameEdited();

	void showInputSettings();

protected:
	virtual void showEvent(QShowEvent*);

signals:
	void trackAdded(qint64);

public:
	CreateTrackDialog(int type = 0, int pos = -1, QWidget* parent = 0);
	CreateTrackDialog(VirtualTrack** vt, int type = 0, int pos = -1, QWidget* parent = 0);
	~CreateTrackDialog(){}
	void lockType(bool);
};

#endif
