//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include <QAbstractItemModel>
#include "midipresetdelegate.h"
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

MidiPresetDelegate::MidiPresetDelegate(QObject* parent) : QItemDelegate(parent)
{
}

QWidget *MidiPresetDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex & index ) const
{
	if(index.isValid())
	{
		int row = index.row();
		QModelIndex trackfield = index.sibling(row, 2);
		const QAbstractItemModel* mod = index.model();
		if(mod && trackfield.isValid())
		{
			int port = mod->data(trackfield, MidiPortRole).toInt();
			int preset = mod->data(index, MidiPresetRole).toInt();
			MidiPort* mport = &midiPorts[port];
			if(mport)
			{
				QComboBox *combo = new QComboBox(parent);
				combo->insertItem(0, "None", 0);
				QHashIterator<int, QString> iter(*mport->presets());
				int c = 1;
				while(iter.hasNext())
				{
					iter.next();
					combo->insertItem(c, QString::number(iter.key()), iter.key());
					if(iter.key() == preset)
						combo->setCurrentIndex(c);
					++c;
				}
				return combo;
			}
		}
	}

	return 0;
}

void MidiPresetDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QComboBox *combo = static_cast<QComboBox*>(editor);
	const QAbstractItemModel* mod = index.model();
	if(combo && mod)
	{
		int preset = mod->data(index, MidiPresetRole).toInt();
		int val = 0;
		if(preset)
		{
			val = combo->findText(QString::number(preset));
		}
		combo->setCurrentIndex(val);
	}
}

void MidiPresetDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QComboBox *combo = static_cast<QComboBox*>(editor);
	if(combo)
	{
		int i = combo->currentIndex();
		int preset = combo->itemData(i).toInt();

		QString name("None");
		if(preset)
			name = QString::number(preset);
		model->setData(index, name, Qt::DisplayRole);
		model->setData(index, preset, MidiPresetRole);
	}
}

void MidiPresetDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

