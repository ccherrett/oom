//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include <QtGui>
#include <QAbstractItemModel>
#include "instrumentdelegate.h"
#include "instrumentcombo.h"
#include "track.h"
#include "song.h"

InstrumentDelegate::InstrumentDelegate(QObject* parent) : QItemDelegate(parent)
{
}

QWidget *InstrumentDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &/* option */, const QModelIndex & index ) const
{
	int row = index.row();
	QModelIndex trackfield = index.sibling(row, 0);
	if(trackfield.isValid())
	{
		const QAbstractItemModel* mod = index.model();
		if(mod)
		{
			//MidiTrack* track = dynamic_cast<MidiTrack*>(mod->data(trackfield, InstrumentRole));
			QString tname = mod->data(trackfield, Qt::DisplayRole).toString();
			Track* t = song->findTrack(tname);
			if(t && t->isMidiTrack())
			{
				int prog = mod->data(index, ProgramRole).toInt();
				QString pname = mod->data(index, Qt::DisplayRole).toString();
				InstrumentCombo *m_editor = new InstrumentCombo(parent, (MidiTrack*)t, prog, pname);
				m_editor->updateValue(prog, pname);
				//m_editor->setProgram(prog);
				//m_editor->setProgramName(pname);
				return m_editor;
			}
		}
	}

	return 0;
}

void InstrumentDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	int value = index.model()->data(index, Qt::EditRole).toInt();

	InstrumentCombo *combo = static_cast<InstrumentCombo*>(editor);
	combo->setProgram(value);
}

void InstrumentDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	InstrumentCombo *combo = static_cast<InstrumentCombo*>(editor);
	if(combo)
	{
		int value = combo->getProgram();
		QString n = combo->getProgramName();

		model->setData(index, n, Qt::EditRole);
		model->setData(index, value, ProgramRole);
	}
}

void InstrumentDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

