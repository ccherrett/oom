//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================


#ifndef _OOM_MASSIGNDIALOG_
#define _OOM_MASSIGNDIALOG_

#include "ui_midiassignbase.h"
#include <QStringList>

class QDialog;
class Track;
class QStandardItem;
class QStandardItemModel;
class MidiPort;
class QString;

struct MidiAssignData {
	Track* track;
	MidiPort* port;
	int channel;
	int volume;
	int pan;
	int reverb;
	int chorus;
	int var;
	int rec;
	int mute;
	int solo;
	bool enabled;
};

class MidiAssignDialog : public QDialog, public Ui::MidiAssignBase
{
	Q_OBJECT

	QStandardItemModel *m_model;
	QStringList m_midicontrols;
	QStringList _trackTypes;

public:
	MidiAssignDialog(QWidget* parent = 0);

private slots:
	void portChanged(int);
	void channelChanged(int);
	void cmbTypeSelected(int);
	void updateTableHeader();
	void btnCloseClicked();
};
#endif
