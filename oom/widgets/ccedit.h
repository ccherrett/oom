//=================================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams and Christopher Cherrett
//=================================================================

#ifndef _OOM_CCEDIT_H_
#define _OOM_CCEDIT_H_

#include "ui_cceditbase.h"

class Track;
class CCInfo;

class CCEdit : public QFrame, public Ui::CCEditBase
{
	Q_OBJECT
	CCInfo* m_info;

public:
	CCEdit(QWidget* parent = 0, CCInfo* i = 0);
	CCEdit(QWidget* parent = 0);
	virtual ~CCEdit();

	CCInfo* info() { return m_info; }
	void setInfo(CCInfo* i) { 
		m_info = i;
		updateValues();
	}

private slots:
	void doLearn(int port, int chan, int cc, int lsb = -1);
	void startLearning();
	void updateValues();
	void channelChanged(int val);
	void controlChanged(int val);
	void recordOnlyChanged(bool);
	void toggleChanged(bool);
	void toggleNRPN(bool);
	void msbChanged(int val);
	void lsbChanged(int val);

signals:
	void valuesUpdated(CCInfo* i);

};

#endif
