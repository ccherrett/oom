//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include <QAbstractItemModel>
#include "midiportdelegate.h"
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
#include "song.h"

MidiPortDelegate::MidiPortDelegate(QObject* parent) : QItemDelegate(parent)
{
}

QWidget *MidiPortDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex & index ) const
{
	if(index.isValid())
	{
		const QAbstractItemModel* mod = index.model();
		if(mod)
		{
			int port = mod->data(index, MidiPortRole).toInt();
			QComboBox *combo = new QComboBox(parent);
			for (int i = 0; i < MIDI_PORTS; ++i)
			{
				QString name;
				name.sprintf("%d:%s", i + 1, midiPorts[i].portname().toLatin1().constData());
				combo->insertItem(i, name);
				if (i == port)
					combo->setCurrentIndex(i);
			}
			return combo;
		}
	}

	return 0;
}

void MidiPortDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QComboBox *combo = static_cast<QComboBox*>(editor);
	const QAbstractItemModel* mod = index.model();
	if(combo && mod)
	{
		int prog = mod->data(index, MidiPortRole).toInt();
		combo->setCurrentIndex(prog);
	}
}

void MidiPortDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QComboBox *combo = static_cast<QComboBox*>(editor);
	if(combo)
	{
		int i = combo->currentIndex();
		QString name;
		name.sprintf("%d:%s", i + 1, midiPorts[i].portname().toLatin1().constData());

		model->setData(index, name, Qt::DisplayRole);
		model->setData(index, i, MidiPortRole);
	}
}

void MidiPortDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

