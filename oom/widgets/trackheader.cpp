//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "gconfig.h"
#include "globals.h"
#include "config.h"
#include "track.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "popupmenu.h"
#include "globals.h"
#include "icons.h"
#include "scrollscale.h"
#include "xml.h"
#include "mididev.h"
#include "midiport.h"
#include "midiseq.h"
#include "comment.h"
#include "header.h"
#include "node.h"
#include "instruments/minstrument.h"
#include "arranger.h"
#include "event.h"
#include "midiedit/drummap.h"
#include "synth.h"
#include "menulist.h"
#include "midimonitor.h"
#include "pcanvas.h"
#include "trackheader.h"

TrackHeader::TrackHeader(Track* t, QWidget* parent)
: QFrame(parent)
{
	m_track = t;
}

void TrackHeader::songChanged(int)
{
}
