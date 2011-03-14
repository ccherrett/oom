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
	_mapModel = new QStandardItemModel(mapTable);
	connect(btnConnect, SIGNAL(clicked(bool)), SLOT(btnConnectClicked(bool)));
	connect(btnRefresh, SIGNAL(clicked(bool)), SLOT(btnRefreshClicked(bool)));
	connect(btnImport, SIGNAL(clicked(bool)), SLOT(btnImportClicked(bool)));
	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked(bool)));
	updateTableHeader();
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
	//force disconnect on the client and quit
	close();
}
void LSCPImport::updateTableHeader()/*{{{*/
{
	QStandardItem* chk = new QStandardItem(true);
	chk->setCheckable(true);
	chk->setCheckState(Qt::Unchecked);
	QStandardItem* instr = new QStandardItem(tr("Instruments"));
	QStandardItem* fname = new QStandardItem(tr("File Name"));
	_mapModel->setHorizontalHeaderItem(0, chk);
	_mapModel->setHorizontalHeaderItem(1, instr);
	_mapModel->setHorizontalHeaderItem(2, fname);
	mapTable->setColumnWidth(0, 20);
	mapTable->setColumnWidth(1, 200);
	mapTable->horizontalHeader()->setStretchLastSection(true);
	mapTable->verticalHeader()->hide();
}/*}}}*/
#endif
