//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2011 Andrew Williams
//=========================================================

#include "config.h"
#include "globals.h"
#include "gconfig.h"
#ifdef LSCP_SUPPORT
#include "audio.h"
#include "instruments/minstrument.h"
#include "midictrl.h"
#include "network/lsclient.h"
#include "importinstruments.h"
#include <QStandardItem>
#include <QStandardItemModel>

LSCPImport::LSCPImport(QWidget* parent) :QDialog(parent)
{
	setupUi(this);
	_mapModel = new QStandardItemModel();
	connect(btnConnect, SIGNAL(clicked(bool)), SLOT(btnConnectClicked(bool)));
	connect(btnRefresh, SIGNAL(clicked(bool)), SLOT(btnRefreshClicked(bool)));
	connect(btnImport, SIGNAL(clicked(bool)), SLOT(btnImportClicked(bool)));
	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked(bool)));
}

void LSCPImport::btnConnectClicked(bool)
{
}

void LSCPImport::btnRefreshClicked(bool)
{
}

void LSCPImport::btnImportClicked(bool)
{
}

void LSCPImport::btnCloseClicked(bool)
{
}
#endif
