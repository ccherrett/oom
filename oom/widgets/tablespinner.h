//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_TABLE_SPINNER_
#define _OOM_TABLE_SPINNER_

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QSpinBox>

class TableSpinnerDelegate : public QItemDelegate
{
	Q_OBJECT

	int minVal;
	int maxVal;

public:
	TableSpinnerDelegate(QObject *parent = 0, int min = -126, int max = 127);

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &opt, const QModelIndex &ind) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
											  
};

#endif
