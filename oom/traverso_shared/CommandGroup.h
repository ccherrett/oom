//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//
//
//  (C) Copyright 2011 Remon Sijrier
//=========================================================

#ifndef OOM_COMMAND_GROUP_H
#define OOM_COMMAND_GROUP_H

#include "OOMCommand.h"

#include <QList>

class CommandGroup : public OOMCommand
{
	Q_OBJECT
public :
	CommandGroup(const QString& des)
		: OOMCommand(des)
	{
	}
	~CommandGroup();

	int do_action();
	int undo_action();

	void add_command(OOMCommand* cmd) {
		Q_ASSERT(cmd);
		m_commands.append(cmd);
	}
private :
	QList<OOMCommand* >	m_commands;

};

#endif



