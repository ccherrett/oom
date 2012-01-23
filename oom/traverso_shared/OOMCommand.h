//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//
//
//  (C) Copyright 2011 Remon Sijrier
//=========================================================

#ifndef OOMCOMMAND_H
#define OOMCOMMAND_H

#include <QObject>
#include <QUndoCommand>
#include <QUndoStack>

class QUndoStack;

class OOMCommand : public QObject, public QUndoCommand
{
        Q_OBJECT

public :
	enum
	{
		ADD, REMOVE, MODIFY
	};

	OOMCommand(const QString& des = "No description set!");
	virtual ~OOMCommand();

	virtual int do_action();
        virtual int undo_action();
        
	void undo() {undo_action();}
	void redo() {do_action();}

        QString		m_description;

	static void process_command(OOMCommand* cmd);
};


#endif


