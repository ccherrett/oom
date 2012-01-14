//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#ifndef _OOM_CREATE_TRACKS_DIALOG_
#define _OOM_CREATE_TRACKS_DIALOG_

#include "ui_createtrackbase.h"

class QShowEvent;

class CreateTrackDialog : public QDialog, public Ui::CreateTrackBase {
	Q_OBJECT

	int m_insertType;
	int m_insertPosition;
	bool m_createMidiInputDevice;
	bool m_createMidiOutputDevice;
	bool m_midiSameIO;
	int m_midiInPort;
	int m_midiOutPort;
	bool m_createTrackOnly;
	int m_showJackAliases;
	
	void importInputs();
	void importOutputs();
	void setTitleText();
	void populateInputList();
	void populateOutputList();
	void populateNewInputList();
	void populateNewOutputList();
	void populateInstrumentList();
	void populateMonitorList();
	void populateBussList();
	void hideMidiElements();
	void showAllElements();
	int getFreeMidiPort();
	void createMonitorInputTracks(QString);

private slots:
	void addTrack();
	void updateInputSelected(bool);
	void updateOutputSelected(bool);
	void trackTypeChanged(int);
	void monitorChecked(bool);

protected:
	virtual void showEvent(QShowEvent*);

signals:
	void trackAdded(QString);

public:
	CreateTrackDialog(int type = 0, int pos = -1, QWidget* parent = 0);
	~CreateTrackDialog(){}
};

#endif
