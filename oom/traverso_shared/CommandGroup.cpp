//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//
//
//  (C) Copyright 2011 Remon Sijrier
//=========================================================

#include "CommandGroup.h"

/** 	\class CommandGroup 
 *	\brief A class to return a group of Command objects as one history object to the historystack
 *	
 */


CommandGroup::~ CommandGroup()
{
	foreach(OOMCommand* cmd, m_commands) {
		delete cmd;
	}
}

int CommandGroup::do_action()
{
	foreach(OOMCommand* cmd, m_commands) {
		cmd->do_action();
	}
	
	return 1;
}

int CommandGroup::undo_action()
{
	foreach(OOMCommand* cmd, m_commands) {
		cmd->undo_action();
	}
	
	return 1;
}

// eof

