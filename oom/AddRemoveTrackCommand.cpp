//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//    add ctrl value command class
//
//  (C) Copyright 2011 Remon Sijrier
//=========================================================


#include "AddRemoveTrackCommand.h"
#include "TrackManager.h"

AddRemoveTrackCommand::AddRemoveTrackCommand(Track *t, int type, int index)
	: OOMCommand(tr("Add Track"))
	, m_vtrack(0)
	, m_track(t)
	, m_type(type)
	, m_index(index)
{
}

AddRemoveTrackCommand::AddRemoveTrackCommand(VirtualTrack *vt, int type, int index)
	: OOMCommand(tr("Add Track"))
	, m_vtrack(vt)
	, m_track(0)
	, m_type(type)
	, m_index(index)
{
}

int AddRemoveTrackCommand::do_action()
{
	if (m_type == ADD)
	{
		//Perform add track actions
	}
	else
	{
		//Perform delete track actions
	}


	return 1;
}

int AddRemoveTrackCommand::undo_action()
{
	if (m_type == ADD)
	{
		//Perform delete track action
	}
	else
	{
		//Perform add track actions
	}

	return 1;
}
