//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_INS_TREE_
#define _OOM_INS_TREE_

#include <QTreeView>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QStandardItem>
#include <QModelIndex>
#include <QFocusEvent>
#include <QSize>

class MidiTrack;
class InstrumentTree : public QTreeView
{
	Q_OBJECT
	MidiTrack* m_track;
	bool m_popup;
	QStandardItemModel *_patchModel;
	QItemSelectionModel *_patchSelModel;

public:
	InstrumentTree(QWidget* parent = 0, MidiTrack* track = 0, bool popup = false);
	void setTrack(MidiTrack* t) { 
		m_track = t;
		updateModel();
	}
	MidiTrack* track() { return m_track; }
private:
	void updateModel();
	void updateHeader();
protected:
	virtual void focusOutEvent(QFocusEvent*);
	QSize sizeHint();
private slots:
	void patchDoubleClicked(QModelIndex);
	void patchClicked(QModelIndex);
signals:
	void patchSelected(int program, QString name);
	void treeFocusLost();
};

#endif
