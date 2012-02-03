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
#include "minstrument.h"
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
			//QString tname = mod->data(trackfield, Qt::DisplayRole).toString();
			int prog = mod->data(index, ProgramRole).toInt();
			QString instrName = mod->data(index, InstrumentNameRole).toString();
			QString pname = mod->data(index, Qt::DisplayRole).toString();
			MidiInstrument* instr = 0;
			for (iMidiInstrument i = midiInstruments.begin(); i != midiInstruments.end(); ++i)/*{{{*/
			{
				if ((*i)->iname() == instrName)
				{
					instr = *i;
					break;
				}
			}/*}}}*/
			if(instr)
			{
				InstrumentCombo *m_editor = new InstrumentCombo(parent, instr, prog, pname);
				m_editor->updateValue(prog, pname);
				return m_editor;
			}
		}
	}

	return 0;
}

void InstrumentDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	InstrumentCombo *combo = static_cast<InstrumentCombo*>(editor);
	const QAbstractItemModel* mod = index.model();
	if(combo && mod)
	{
		int prog = mod->data(index, ProgramRole).toInt();
		QString pname = mod->data(index, Qt::DisplayRole).toString();
		combo->updateValue(prog, pname);
	}
}

void InstrumentDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	InstrumentCombo *combo = static_cast<InstrumentCombo*>(editor);
	if(combo)
	{
		int value = combo->getProgram();
		QString n = combo->getProgramName();

		model->setData(index, n, Qt::DisplayRole);
		model->setData(index, value, ProgramRole);
	}
}

void InstrumentDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

