//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_MIDIPRESETT_DELEGATE_
#define _OOM_MIDIPRESET_DELEGATE_

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QMenu>

#ifndef MidiPortNameRole
#define MidiPortNameRole Qt::UserRole+3
#endif
#ifndef MidiPortRole
#define MidiPortRole Qt::UserRole+4
#endif
#ifndef MidiChannelRole
#define MidiChannelRole Qt::UserRole+5
#endif
#ifndef MidiControlRole
#define MidiControlRole Qt::UserRole+6
#endif
#ifndef MidiPresetRole
#define MidiPresetRole Qt::UserRole+7
#endif

class MidiPresetDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	MidiPresetDelegate(QObject *parent = 0);

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &opt, const QModelIndex &ind) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
											  
};

#endif

