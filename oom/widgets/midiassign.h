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
class QItemSelectionModel;
class QItemSelection;
class MidiPort;
class QString;
class QShowEvent;


class MidiAssignDialog : public QDialog, public Ui::MidiAssignBase
{
	Q_OBJECT

	QStandardItemModel *m_model;
	QStandardItemModel *m_ccmodel;
	QItemSelectionModel *m_selmodel;
	QStringList m_midicontrols;
	QStringList _trackTypes;
	QList<int> m_allowed;
	Track* m_selected;
	int m_lasttype;

public:
	MidiAssignDialog(QWidget* parent = 0);

private slots:
	void cmbTypeSelected(int);
	void updateTableHeader();
	void updateCCTableHeader();
	void btnCloseClicked();
	void itemChanged(QStandardItem*);
	void itemSelected(const QItemSelection&, const QItemSelection&); //Update the ccEdit table
	//Deals with the m_ccEdit table on a per track basis
	void controllerChanged(QStandardItem*);
	void btnAddController();
	void btnDeleteController();
	void btnUpdateDefault();

protected:
	virtual void showEvent(QShowEvent*);
};
#endif
