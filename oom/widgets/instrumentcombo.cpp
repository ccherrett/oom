//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include "instrumentcombo.h"
#include "instrumenttree.h"
#include "track.h"
#include "song.h"
#include "midiport.h"
#include "minstrument.h"
#include "midictrl.h"
#include "midi.h"
#include <QtGui>

InstrumentCombo::InstrumentCombo(QWidget *parent, MidiTrack* track, int prog, QString pname) : QComboBox(parent)
{
	m_track = track;
	tree = 0;
	m_program = prog;
	m_name = pname;

	setEditable(true);
	installEventFilter(this);
}

void InstrumentCombo::mousePressEvent(QMouseEvent* event)
{
	if(!lineEdit()->rect().contains(event->pos()))
	{//We are clicking the arrow so display the list
		if(!tree)
		{
			tree = new InstrumentTree(this, m_track, true);
			connect(tree, SIGNAL(patchSelected(int, QString)), this, SLOT(updateValue(int, QString)));
			connect(tree, SIGNAL(patchSelected(int, QString)), this, SIGNAL(patchSelected(int, QString)));
			connect(tree, SIGNAL(treeFocusLost()), this, SIGNAL(stopEditing()));
		}
		tree->move(lineEdit()->rect().bottomLeft());
		tree->show();
	}
}

bool InstrumentCombo::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::KeyPress) 
	{
		return true;
	}
	return QObject::eventFilter(obj, event);
}

void InstrumentCombo::updateValue(int prog, QString name)
{
	m_program = prog;
	m_name = name;
	emit stopEditing();
}

void InstrumentCombo::setProgram(int prog)
{
	m_program = prog;
	if(m_track)
	{
		int port = m_track->outPort();
		int channel = m_track->outChannel();
		MidiInstrument* instr = midiPorts[port].instrument();
		if(instr)
		{
			QString name = instr->getPatchName(channel, prog, song->mtype(), m_track->type() == Track::DRUM);
			setProgramName(name);
		}
	}
}
