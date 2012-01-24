//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//    add ctrl value command class
//
//  (C) Copyright 2011 Remon Sijrier
//=========================================================

#ifndef ADD_CTRL_VALUES_H
#define ADD_CTRL_VALUES_H

#include "OOMCommand.h"

#include "ctrl.h"

class AddRemoveCtrlValues : public OOMCommand
{
        Q_OBJECT

public:
	AddRemoveCtrlValues(
		CtrlList* cl,
		QList<CtrlVal> ctrlValues,
		int type);

	AddRemoveCtrlValues(
		CtrlList* cl,
		QList<CtrlVal> ctrlValues,
		QList<CtrlVal> newCtrlValues,
		int type = MODIFY);

	AddRemoveCtrlValues(
		CtrlList* cl,
		CtrlVal ctrlValues,
		int type);

        int do_action();
        int undo_action();

private:
	CtrlList*		m_cl;
	QList<CtrlVal>		m_ctrlValues;
	QList<CtrlVal>		m_newCtrlValues;
	int			m_type;
	double m_startValue;
};

#endif // ADD_CTRL_VALUE_H
