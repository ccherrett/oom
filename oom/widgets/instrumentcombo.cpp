//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=========================================================

#include "instrumentcombo.h"
#include "instrumenttree.h"
#include "instrumentmenu.h"
#include "minstrument.h"
#include "track.h"
#include "song.h"
#include "midiport.h"
#include "minstrument.h"
#include "midictrl.h"
#include "midi.h"
#include <QtGui>

InstrumentCombo::InstrumentCombo(QWidget *parent, MidiInstrument* instrument, int prog, QString pname) : QComboBox(parent)
{
	m_instrument = instrument;
	tree = 0;
	m_program = prog;
	m_name = pname;

	setEditable(false);
	setMaxCount(1);
	installEventFilter(this);
}

void InstrumentCombo::mousePressEvent(QMouseEvent*)
{
	QMenu *p = new QMenu(this);
	if(!tree)
	{
	//printf("InstrumentCombo::mousePressEvent() instrumentName: %s\n",m_instrument->name().toUtf8().constData());
		tree = new InstrumentMenu(p, m_instrument);
		connect(tree, SIGNAL(patchSelected(int, QString)), this, SLOT(updateValue(int, QString)));
		connect(tree, SIGNAL(patchSelected(int, QString)), this, SIGNAL(patchSelected(int, QString)));
	}
	p->addAction(tree);
	p->exec(QCursor::pos());
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
	clear();
	addItem(m_name, m_program);
}

void InstrumentCombo::setProgram(int prog)
{
	m_program = prog;
}

void InstrumentCombo::setProgramName(QString pname)
{
	m_name = pname;
}
