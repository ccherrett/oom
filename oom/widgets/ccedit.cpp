//================================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//================================================================

#include "utils.h"
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
	connect(m_chkToggle, SIGNAL(toggled(bool)), this, SLOT(toggleChanged(bool)));
	connect(m_chkNRPN, SIGNAL(toggled(bool)), this, SLOT(toggleNRPN(bool)));
	connect(m_txtMSB, SIGNAL(valueChanged(int)), this, SLOT(msbChanged(int)));
	connect(m_txtLSB, SIGNAL(valueChanged(int)), this, SLOT(lsbChanged(int)));
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
	connect(m_chkToggle, SIGNAL(toggled(bool)), this, SLOT(toggleChanged(bool)));
	connect(m_chkNRPN, SIGNAL(toggled(bool)), this, SLOT(toggleNRPN(bool)));
	connect(m_txtMSB, SIGNAL(valueChanged(int)), this, SLOT(msbChanged(int)));
	connect(m_txtLSB, SIGNAL(valueChanged(int)), this, SLOT(lsbChanged(int)));
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
		m_channel->blockSignals(true);
		m_chkRecord->blockSignals(true);
		m_chkToggle->blockSignals(true);
		m_txtMSB->blockSignals(true);
		m_txtLSB->blockSignals(true);
		m_chkNRPN->blockSignals(true);

		m_controlcombo->setCurrentIndex(0);
		m_channel->setValue(1);
		m_chkRecord->setChecked(false);
		m_chkToggle->setChecked(false);

		m_chkNRPN->setChecked(false);
		m_txtMSB->setVisible(false);
		m_txtLSB->setVisible(false);
		m_hexLabel->setVisible(false);
		m_txtMSB->setValue(-1);
		m_txtLSB->setValue(-1);
		m_controlcombo->setVisible(true);

		m_controlcombo->blockSignals(false);
		m_channel->blockSignals(false);
		m_chkRecord->blockSignals(false);
		m_chkToggle->blockSignals(false);
		m_txtMSB->blockSignals(false);
		m_txtLSB->blockSignals(false);
		m_chkNRPN->blockSignals(false);
	}
	else
	{
		QString l;
		if(m_info->controller() >= 0 && m_info->controller() <= 127)
			l.append(QString::number(m_info->controller())).append(": ").append(midiCtrlName(m_info->controller()));
		else
			l.append(midiControlToString(m_info->controller()));
		label->setText(l);
		m_controlcombo->blockSignals(true);
		m_channel->blockSignals(true);
		m_chkRecord->blockSignals(true);
		m_chkToggle->blockSignals(true);
		m_txtMSB->blockSignals(true);
		m_txtLSB->blockSignals(true);
		m_chkNRPN->blockSignals(true);

		m_controlcombo->setCurrentIndex(m_info->assignedControl()+1);
		m_channel->setValue(m_info->channel()+1);
		m_chkRecord->setChecked(m_info->recordOnly());
		m_chkToggle->setChecked(m_info->fakeToggle());

		m_chkNRPN->setChecked(m_info->nrpn());
		m_txtMSB->setVisible(m_info->nrpn());
		m_txtLSB->setVisible(m_info->nrpn());
		m_txtMSB->setValue(m_info->msb());
		m_txtLSB->setValue(m_info->lsb());
		m_hexLabel->setVisible(m_info->nrpn());
		m_hexLabel->setText(QString::number(calcNRPN7(m_info->msb(), m_info->lsb())));
		m_controlcombo->setVisible(!m_info->nrpn());

		m_chkRecord->blockSignals(false);
		m_channel->blockSignals(false);
		m_controlcombo->blockSignals(false);
		m_chkToggle->blockSignals(false);
		m_txtMSB->blockSignals(false);
		m_txtLSB->blockSignals(false);
		m_chkNRPN->blockSignals(false);
	}
}

void CCEdit::recordOnlyChanged(bool state)
{
	if(!m_info)
		return;
	m_info->setRecordOnly(state);
}

void CCEdit::toggleChanged(bool state)
{
	if(m_info)
		m_info->setFakeToggle(state);
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

void CCEdit::toggleNRPN(bool f)
{
	if(!m_info)
		return;
	m_info->setNRPN(f);
	m_txtMSB->setVisible(f);
	m_txtLSB->setVisible(f);
	m_hexLabel->setVisible(f);
	m_hexLabel->setText(QString::number(calcNRPN7(m_info->msb(), m_info->lsb())));
	m_controlcombo->setVisible(!f);
}

void CCEdit::msbChanged(int val)
{
	if(!m_info)
		return;

	int oldcc = m_info->assignedControl();
	m_info->setMSB(val);
	m_hexLabel->setText(QString::number(calcNRPN7(m_info->msb(), m_info->lsb())));
	midiMonitor->msgModifyTrackController(m_info->track(), oldcc, m_info);
}

void CCEdit::lsbChanged(int val)
{
	if(!m_info)
		return;

	int oldcc = m_info->assignedControl();
	m_info->setLSB(val);
	m_hexLabel->setText(QString::number(calcNRPN7(m_info->msb(), m_info->lsb())));
	midiMonitor->msgModifyTrackController(m_info->track(), oldcc, m_info);
}

void CCEdit::startLearning()
{
	if(!m_info)
		return;
	connect(song, SIGNAL(midiLearned(int, int, int, int)), this, SLOT(doLearn(int, int, int, int)));
	midiMonitor->msgStartLearning(m_info->port());
}

void CCEdit::doLearn(int port, int chan, int cc, int lsb)
{
	//Stop listening to learning signals
	disconnect(song, SIGNAL(midiLearned(int, int, int, int)), this, SLOT(doLearn(int, int, int, int)));
	if(!m_info)
		return;

	if(m_info->port() != port)
		return; //For now we use the port assigned to the track
	
	if(lsb >= 0) //NRPN learned
	{
		int oldcc = m_info->assignedControl();
		m_info->setPort(port);
		m_info->setChannel(chan);
		m_info->setNRPN(true);
		m_info->setMSB(cc);
		m_info->setLSB(lsb);
		//printf("OLD number: %d\n", oldcc);
		printf("Midi NRPN Learned: port: %d, channel: %d, MSB: %d, LSB: %d\n", port, chan, cc, lsb);
		midiMonitor->msgModifyTrackController(m_info->track(), oldcc, m_info);
	}
	else
	{
		int oldcc = m_info->assignedControl();
		printf("Midi Learned: port: %d, channel: %d, CC: %d\n", port, chan, cc);
		m_info->setPort(port);
		m_info->setChannel(chan);
		m_info->setAssignedControl(cc);
		midiMonitor->msgModifyTrackController(m_info->track(), oldcc, m_info);
	}
	updateValues();

	emit valuesUpdated(m_info);
}
