//========================================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//
//    Add Track commands
//
//  (C) Copyright 2012 The Open Octave Project http://www.openoctave.org
//========================================================================

#ifndef ADD_TRACK_H
#define ADD_TRACK_H

#include "traverso_shared/OOMCommand.h"

#include "track.h"
class VirtualTrack;

class AddRemoveTrackCommand : public OOMCommand
{
        Q_OBJECT

public:
	AddRemoveTrackCommand(
		Track* t,
		int type,
		int index = -1);

	AddRemoveTrackCommand(
		VirtualTrack* t,
		int type,
		int index = -1);

        int do_action();
        int undo_action();

private:
	VirtualTrack* m_vtrack;
	Track* m_track;
	int m_type;
	int m_index;
};

#endif // ADD_TRACK_H
