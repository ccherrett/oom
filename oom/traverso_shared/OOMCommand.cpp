//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//
//
//  (C) Copyright 2011 Remon Sijrier
//=========================================================

#include "OOMCommand.h"

OOMCommand::OOMCommand( const QString& des )
	: QUndoCommand(des)
{
}
OOMCommand::~OOMCommand()
{}


/**
 * 	Virtual function, needs to be reimplemented for all
	type of Commands
 */
int OOMCommand::do_action( )
{
	return -1;
}

/**
 * 	Virtual function, needs to be reimplemented for all
	type of Commands

 */
int OOMCommand::undo_action( )
{
	return -1;
}

void OOMCommand::process_command(OOMCommand * cmd)
{
	Q_ASSERT(cmd);

	qDebug("processing %s\n", cmd->text().toAscii().data());
	cmd->redo();
}
