//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef _OOM_CC_DELEGATE_
#define _OOM_CC_DELEGATE_

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include <QMenu>

#define PortNameRole Qt::UserRole+3
#define PortRole Qt::UserRole+4
#define ChannelRole Qt::UserRole+5
#define ControlRole Qt::UserRole+6
#define CCRole Qt::UserRole+7
#define TrackRole Qt::UserRole+8
#define CCSortRole Qt::UserRole+9


class CCInfoDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	CCInfoDelegate(QObject *parent = 0);

	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &opt, const QModelIndex &ind) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
											  
};

#endif

