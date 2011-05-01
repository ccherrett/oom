//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_MIDIPORT_DELEGATE_
#define _OOM_MIDIPORT_DELEGATE_

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QMenu>

#define MidiPortNameRole Qt::UserRole+3
#define MidiPortRole Qt::UserRole+4
#define MidiChannelRole Qt::UserRole+5
#define MidiControlRole Qt::UserRole+6

class MidiPortDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	MidiPortDelegate(QObject *parent = 0);

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &opt, const QModelIndex &ind) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
											  
};

#endif

