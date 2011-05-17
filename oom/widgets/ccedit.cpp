//================================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//================================================================

#include "midictrl.h"
#include "song.h"
#include "ccinfo.h"
#include "ccedit.h"
#include "midimonitor.h"

CCEdit::CCEdit(QWidget* parent, CCInfo* n) : QFrame(parent)
{
	setupUi(this);
	m_info = n;
	m_controlcombo->addItem(tr("Off"), -1);
	for(int i = 0; i < 128; ++i)
	{
		QString ctl(QString::number(i)+": ");
		m_controlcombo->addItem(ctl.append(midiCtrlName(i)), i);
	}
	updateValues();
	connect(m_learn, SIGNAL(clicked()), this, SLOT(startLearning()));
	connect(m_channel, SIGNAL(valueChanged(int)), this, SLOT(channelChanged(int)));
	connect(m_controlcombo, SIGNAL(currentIndexChanged(int)), this, SLOT(controlChanged(int)));
	connect(m_chkRecord, SIGNAL(toggled(bool)), this, SLOT(recordOnlyChanged(bool)));
}

CCEdit::CCEdit(QWidget* parent) : QFrame(parent)
{
	setupUi(this);
	m_info = 0;
	m_controlcombo->addItem(tr("Off"), -1);
	for(int i = 0; i < 128; ++i)
	{
		QString ctl(QString::number(i)+": ");
		m_controlcombo->addItem(ctl.append(midiCtrlName(i)), i);
		//m_controlcombo->addItem(midiCtrlName(i));
	}
	connect(m_learn, SIGNAL(clicked()), this, SLOT(startLearning()));
	connect(m_channel, SIGNAL(valueChanged(int)), this, SLOT(channelChanged(int)));
	connect(m_controlcombo, SIGNAL(currentIndexChanged(int)), this, SLOT(controlChanged(int)));
	connect(m_chkRecord, SIGNAL(toggled(bool)), this, SLOT(recordOnlyChanged(bool)));
	updateValues();
}

CCEdit::~CCEdit()
{
}

void CCEdit::updateValues()
{
	if(!m_info)
	{
		QString l;
		l.append(QString::number(0)).append(": ").append(midiCtrlName(0));
		label->setText(l);
		m_controlcombo->blockSignals(true);
		m_controlcombo->setCurrentIndex(0);
		m_controlcombo->blockSignals(false);

		m_channel->blockSignals(true);
		m_channel->setValue(1);
		m_channel->blockSignals(false);

		m_chkRecord->blockSignals(true);
		m_chkRecord->setChecked(false);
		m_chkRecord->blockSignals(false);
	}
	else
	{
		QString l;
		if(m_info->controller() == CTRL_RECORD)
			l.append(tr("Track Record"));
		else if(m_info->controller() == CTRL_MUTE)
			l.append(tr("Track Mute"));
		else if(m_info->controller() == CTRL_SOLO)
			l.append(tr("Track Solo"));
		else
			l.append(QString::number(m_info->controller())).append(": ").append(midiCtrlName(m_info->controller()));
		label->setText(l);
		m_controlcombo->blockSignals(true);
		m_controlcombo->setCurrentIndex(m_info->assignedControl()+1);
		m_controlcombo->blockSignals(false);
		m_channel->blockSignals(true);
		m_channel->setValue(m_info->channel()+1);
		m_channel->blockSignals(false);
		m_chkRecord->blockSignals(true);
		m_chkRecord->setChecked(m_info->recordOnly());
		m_chkRecord->blockSignals(false);
	}
}

void CCEdit::recordOnlyChanged(bool state)
{
	if(!m_info)
		return;
	m_info->setRecordOnly(state);
}

void CCEdit::channelChanged(int val)
{
	if(!m_info)
		return;
	m_info->setChannel(val - 1);
}

void CCEdit::controlChanged(int val)
{
	if(!m_info)
		return;
	int oldcc = m_info->assignedControl();
	m_info->setAssignedControl(m_controlcombo->itemData(val).toInt());
	midiMonitor->msgModifyTrackController(m_info->track(), oldcc, m_info);
}

void CCEdit::startLearning()
{
	if(!m_info)
		return;
	connect(song, SIGNAL(midiLearned(int, int, int)), this, SLOT(doLearn(int, int, int)));
	midiMonitor->msgStartLearning(m_info->port());
}

void CCEdit::doLearn(int port, int chan, int cc)
{
	//Stop listening to learning signals
	disconnect(song, SIGNAL(midiLearned(int, int, int)), this, SLOT(doLearn(int, int, int)));
	if(!m_info)
		return;
	if(m_info->port() != port)
		return; //For now we use the port assigned to the track
	int oldcc = m_info->assignedControl();
	printf("Midi Learned: port: %d, channel: %d, CC: %d\n", port, chan, cc);
	m_info->setPort(port);
	m_info->setChannel(chan);
	m_info->setAssignedControl(cc);
	midiMonitor->msgModifyTrackController(m_info->track(), oldcc, m_info);
	updateValues();

	emit valuesUpdated(m_info);
}
