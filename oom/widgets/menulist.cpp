//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: $
//  (C) Copyright 2011 Andrew Williams and the OOMidi team
//=========================================================

#include <QWidgetAction>
#include <QListWidget>
#include <QListWidgetItem>
#include <QKeyEvent>
#include <QCoreApplication>

#include "track.h"
#include "globals.h"
#include "icons.h"
#include "mididev.h"
#include "midiport.h"
#include "midiseq.h"
#include "song.h"
#include "node.h"
#include "audio.h"
#include "instruments/minstrument.h"
#include "app.h"
#include "gconfig.h"
#include "config.h"
#include "menulist.h"

MenuList::MenuList(QMenu* parent, Track *t) : QWidgetAction(parent)
{
	_track = t;
}

QWidget* MenuList::createWidget(QWidget* parent)
{
	if(!_track)
		return 0;
	//if(!_track->isMidiTrack() || _track->type() != Track::AUDIO_SOFTSYNTH)
	//	return 0;
	MidiTrack* track = (MidiTrack*) _track;

	MidiDevice* md = 0;
	int port = -1;
	if (track->type() == Track::AUDIO_SOFTSYNTH)
	{
		md = dynamic_cast<MidiDevice*> (track);
		if (md)
			port = md->midiPort();
	}
	else
		port = track->outPort();
	list = new QListWidget(parent);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setAlternatingRowColors(true);
	list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	list->setFixedHeight(300);
	for (int i = 0; i < MIDI_PORTS; ++i)
	{
		QString name;
		name.sprintf("%d:%s", i + 1, midiPorts[i].portname().toLatin1().constData());
		list->insertItem(i, name);
		if (i == port)
			list->setCurrentRow(i);
	}
	connect(list, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	//connect(list, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	//connect(list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(updateData(QListWidgetItem*)));
	return list;
}

void MenuList::updateData(QListWidgetItem *item)
{
	if(list && item)
	{
		//printf("Triggering Menu Action\n");
		setData(list->row(item));
		
		//FIXME: This is a seriously brutal HACK but its the only way it can get it done
		QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e);

		QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
		QCoreApplication::postEvent(this->parent(), e2);
	}
}
