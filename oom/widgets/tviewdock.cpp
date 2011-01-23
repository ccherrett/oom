//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================
//

#include "tviewdock.h"

#include "icons.h"

TrackViewDock::TrackViewDock(QWidget* parent) : QFrame(parent)
{
	setupUi(this);

	btnUp->setIcon(*upPCIcon);
	btnDown->setIcon(*downPCIcon);
	btnDelete->setIcon(*garbagePCIcon);
	btnUp->setIconSize(upPCIcon->size());
	btnDown->setIconSize(downPCIcon->size());
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(btnUpClicked(bool)));
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(btnDownClicked(bool)));
}

TrackViewDock::~TrackViewDock()
{
	//save anything you need here
}

void TrackViewDock::btnUpClicked(bool)
{
}

void TrackViewDock::btnDownClicked(bool)
{
}

void TrackViewDock::btnDeleteClicked(bool)
{
}
