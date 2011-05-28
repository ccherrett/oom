//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include <QAbstractItemModel>
#include "ccdelegate.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "midiport.h"
#include "minstrument.h"
#include "mididev.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "midictrl.h"
#include "ccedit.h"
#include "ccinfo.h"
#include "track.h"
#include "song.h"

CCInfoDelegate::CCInfoDelegate(QObject* parent) : QItemDelegate(parent)
{
}

QWidget *CCInfoDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex & index ) const
{
	if(index.isValid())
	{
		const QAbstractItemModel* mod = index.model();
		if(mod)
		{
			//int port = mod->data(index, PortRole).toInt();
			//int chan = mod->data(index, ChannelRole).toInt();
			int control = mod->data(index, ControlRole).toInt();
			//int cc = mod->data(index, CCRole).toInt();
			Track* t = song->findTrack(mod->data(index, TrackRole).toString());
			CCInfo *info = t->midiAssign()->midimap.value(control);//new CCInfo(t, port, chan, control, cc);
			CCEdit *editor = new CCEdit(parent, info);
			return editor;
		}
	}

	return 0;
}

void CCInfoDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	CCEdit *ccedit = static_cast<CCEdit*>(editor);
	const QAbstractItemModel* mod = index.model();
	if(ccedit && mod)
	{
		//int port = mod->data(index, PortRole).toInt();
		//int chan = mod->data(index, ChannelRole).toInt();
		int control = mod->data(index, ControlRole).toInt();
		//int cc = mod->data(index, CCRole).toInt();
		Track* t = song->findTrack(mod->data(index, TrackRole).toString());
		//CCInfo *info = new CCInfo(t, port, chan, control, cc);
		CCInfo *info = t->midiAssign()->midimap.value(control);//new CCInfo(t, port, chan, control, cc);
		ccedit->setInfo(info);
	}
}

void CCInfoDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	CCEdit *ccedit = static_cast<CCEdit*>(editor);
	if(ccedit)
	{
		CCInfo *info = ccedit->info();
		if(info)
		{
			model->setData(index, info->port(), PortRole);
			model->setData(index, info->channel(), ChannelRole);
			model->setData(index, info->controller(), ControlRole);
			model->setData(index, info->assignedControl(), CCRole);
			model->setData(index, midiControlSortIndex(info->controller()), CCSortRole);
			QString str;//(midiCtrlName(info->controller()));
			str.append("( ").append(midiControlToString(info->controller())).append(" )");
			if(info->assignedControl() >= 0)
				str.append(" Assigned to CC: ").append(QString::number(info->assignedControl())).append(" on Chan: ").append(QString::number(info->channel()+1));
			model->setData(index, str, Qt::DisplayRole);
		}
	}
}

void CCInfoDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
	//editor->setStyleSheet("QWidget#CCEditBase { background-color: #595966; }");
}

