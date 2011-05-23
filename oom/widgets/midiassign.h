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
class QCloseEvent;
class QPushButton;
class MPConfig;
class MidiSyncConfig;
class AudioPortConfig;

class MidiAssignDialog : public QDialog, public Ui::MidiAssignBase
{
	Q_OBJECT

	QStandardItemModel *m_model;
	QStandardItemModel *m_ccmodel;
	QItemSelectionModel *m_selmodel;

	QStandardItemModel *m_mpmodel;
	QItemSelectionModel *m_mpselmodel;
	QStandardItemModel *m_presetmodel;
	
	QStringList m_assignlabels;
	QStringList m_cclabels;
	QStringList m_mplabels;
	QStringList m_presetlabels;

	QStringList _trackTypes;
	QList<int> m_allowed;
	Track* m_selected;
	MidiPort* m_selectport;
	int m_lasttype;
	QPushButton* m_btnReset;
	MPConfig *midiPortConfig;
	MidiSyncConfig* midiSyncConfig;
	AudioPortConfig* audioPortConfig;

public:
	MidiAssignDialog(QWidget* parent = 0);
	~MidiAssignDialog();
	
	MPConfig *getMidiPortConfig()
	{
		return midiPortConfig;
	}
	MidiSyncConfig* getMidiSyncConfig()
	{
		return midiSyncConfig;
	}
	AudioPortConfig* getAudioPortConfig()
	{
		return audioPortConfig;
	}

private slots:
	void btnResetClicked();
	void populateMidiPorts();
	void cmbTypeSelected(int);
	void updateTableHeader();
	void updateCCTableHeader();
	void itemChanged(QStandardItem*);
	void itemSelected(const QItemSelection&, const QItemSelection&); //Update the ccEdit table
	//Deals with the m_ccEdit table on a per track basis
	void btnAddController();
	void btnDeleteController();
	void btnUpdateDefault();
	void currentTabChanged(int);

	//midi port presets
	void updateMPTableHeader();
	void midiPresetChanged(QStandardItem*);
	//update the presets table
	void midiPortSelected(const QItemSelection&, const QItemSelection&);
	void btnAddMidiPreset();
	void btnDeleteMidiPresets();

public slots:
	void switchTabs(int tab = 0);

protected:
	virtual void showEvent(QShowEvent*);
	virtual void closeEvent(QCloseEvent*);
};
#endif
