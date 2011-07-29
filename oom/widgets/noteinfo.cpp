//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: noteinfo.cpp,v 1.4.2.1 2008/08/18 00:15:26 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//=========================================================

#include <QtGui>

#include "config.h"
#include "noteinfo.h"
#include "awl/posedit.h"
//#include "awl/pitchedit.h"
#include "song.h"
#include "globals.h"
///#include "posedit.h"
#include "pitchedit.h"
#include "traverso_shared/TConfig.h"

//---------------------------------------------------
//    NoteInfo
//---------------------------------------------------

NoteInfo::NoteInfo(QWidget* parent)
: QWidget(parent)
{
	deltaMode = false;

	m_layout = new QVBoxLayout(this);

	selTime = new Awl::PosEdit;
	selTime->setObjectName("Start");
	addTool(tr("Start"), selTime);

	selLen = new QSpinBox();
	selLen->setRange(0, 100000);
	selLen->setSingleStep(1);
	addTool(tr("Len"), selLen);

	selPitch = new PitchEdit;
	addTool(tr("Pitch"), selPitch);

	selVelOn = new QSpinBox();
	selVelOn->setRange(0, 127);
	selVelOn->setSingleStep(1);
	addTool(tr("Velo On"), selVelOn);

	selVelOff = new QSpinBox();
	selVelOff->setRange(0, 127);
	selVelOff->setSingleStep(1);
	addTool(tr("Velo Off"), selVelOff);

	m_renderAlpha = new QSpinBox();
	m_renderAlpha->setRange(0, 255);
	m_renderAlpha->setSingleStep(1);
	int alpha = tconfig().get_property("PerformerEdit", "renderalpha", 50).toInt();
	m_renderAlpha->setValue(alpha);

	addTool(tr("BG Brightness"), m_renderAlpha);
	QSpacerItem* vSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	m_layout->addItem(vSpacer);

	connect(selLen, SIGNAL(valueChanged(int)), SLOT(lenChanged(int)));
	connect(selPitch, SIGNAL(valueChanged(int)), SLOT(pitchChanged(int)));
	connect(selVelOn, SIGNAL(valueChanged(int)), SLOT(velOnChanged(int)));
	connect(selVelOff, SIGNAL(valueChanged(int)), SLOT(velOffChanged(int)));
	connect(m_renderAlpha, SIGNAL(valueChanged(int)), SLOT(alphaChanged(int)));
	connect(selTime, SIGNAL(valueChanged(const Pos&)), SLOT(timeChanged(const Pos&)));
}

void NoteInfo::addTool(QString label, QWidget *tool)
{
	QHBoxLayout *box = new QHBoxLayout();
	box->addWidget(new QLabel(label));
	box->addWidget(tool);
	m_layout->addLayout(box);
}

//---------------------------------------------------------
// alphaChanged
//---------------------------------------------------------
void NoteInfo::alphaChanged(int alpha)
{
	tconfig().set_property("PerformerEdit", "renderalpha", alpha);
	tconfig().save();
	emit alphaChanged();
}

void NoteInfo::enableTools(bool state)
{
	selTime->setEnabled(state);
	selLen->setEnabled(state);
	selPitch->setEnabled(state);
	selVelOn->setEnabled(state);
	selVelOff->setEnabled(state);
}

//---------------------------------------------------------
//   setDeltaMode
//---------------------------------------------------------

void NoteInfo::setDeltaMode(bool val)
{
	deltaMode = val;
	selPitch->setDeltaMode(val);
	if (val)
	{
		selLen->setRange(-100000, 100000);
		selVelOn->setRange(-127, 127);
		selVelOff->setRange(-127, 127);
	}
	else
	{
		selLen->setRange(0, 100000);
		selVelOn->setRange(0, 127);
		selVelOff->setRange(0, 127);
	}
}

//---------------------------------------------------------
//   lenChanged
//---------------------------------------------------------

void NoteInfo::lenChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_LEN, val);
}

//---------------------------------------------------------
//   velOnChanged
//---------------------------------------------------------

void NoteInfo::velOnChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_VELON, val);
}

//---------------------------------------------------------
//   velOffChanged
//---------------------------------------------------------

void NoteInfo::velOffChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_VELOFF, val);
}

//---------------------------------------------------------
//   pitchChanged
//---------------------------------------------------------

void NoteInfo::pitchChanged(int val)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_PITCH, val);
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void NoteInfo::setValue(ValType type, int val)
{
	blockSignals(true);
	switch (type)
	{
		case VAL_TIME:
			selTime->setValue(val);
			break;
		case VAL_LEN:
			selLen->setValue(val);
			break;
		case VAL_VELON:
			selVelOn->setValue(val);
			break;
		case VAL_VELOFF:
			selVelOff->setValue(val);
			break;
		case VAL_PITCH:
			selPitch->setValue(val);
			break;
	}
	blockSignals(false);
}

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void NoteInfo::setValues(unsigned tick, int val2, int val3, int val4,
		int val5)
{
	blockSignals(true);
	if (selTime->pos().tick() != tick)
		selTime->setValue(tick);
	if (selLen->value() != val2)
		selLen->setValue(val2);
	if (selPitch->value() != val3)
		selPitch->setValue(val3);
	if (selVelOn->value() != val4)
		selVelOn->setValue(val4);
	if (selVelOff->value() != val5)
		selVelOff->setValue(val5);
	blockSignals(false);
}

//---------------------------------------------------------
//   timeChanged
//---------------------------------------------------------

void NoteInfo::timeChanged(const Pos& pos)
{
	if (!signalsBlocked())
		emit valueChanged(VAL_TIME, pos.tick());
}

