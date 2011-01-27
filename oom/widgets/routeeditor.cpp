//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include "routeeditor.h"

#include "track.h"

RouteEditor::RouteEditor(QWidget* parent) : QDialog(parent)
{
	setupUi(this);
	_selected = 0;
}

RouteEditor::~RouteEditor()
{
}

void RouteEditor::populateInternalPorts()
{
}

void RouteEditor::populateSystemPorts()
{
}

void RouteEditor::populateJackPorts()
{
}

void RouteEditor::setTab(int ind)
{
	tabMain->setCurrentIndex(ind);
}

void RouteEditor::setSelectedTrack(QString /*name*/)
{
}

void RouteEditor::btnOkPressed(bool)
{
}

void RouteEditor::btnCancelPressed(bool)
{
}

void RouteEditor::setTrack(Track* trk)
{
	_selected = trk;
}
