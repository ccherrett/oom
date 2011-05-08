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
#include <QList>

class QDialog;
class Track;
class QStandardItem;
class QStandardItemModel;
class MidiPort;
class QString;


class MidiAssignDialog : public QDialog, public Ui::MidiAssignBase
{
	Q_OBJECT

	QStandardItemModel *m_model;
	QStringList m_midicontrols;
	QStringList _trackTypes;
	QList<int> m_allowed;

public:
	MidiAssignDialog(QWidget* parent = 0);

private slots:
	void portChanged(int);
	void channelChanged(int);
	void cmbTypeSelected(int);
	void updateTableHeader();
	void btnCloseClicked();
	void itemChanged(QStandardItem*);
};
#endif
