//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_INSTRUMENT_DELEGATE_
#define _OOM_INSTRUMENT_DELEGATE_

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QMenu>

#define InstrumentRole Qt::UserRole+1
#define ProgramRole Qt::UserRole+2

class InstrumentDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	InstrumentDelegate(QObject *parent = 0);

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &opt, const QModelIndex &ind) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
											  
};

#endif

