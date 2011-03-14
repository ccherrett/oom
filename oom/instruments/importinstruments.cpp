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
#include "xml.h"
#include "importinstruments.h"
#include <errno.h>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QList>
#include <QCloseEvent>
#include <QMessageBox>

LSCPImport::LSCPImport(QWidget* parent) :QDialog(parent)
{
	setupUi(this);
	_mapModel = new QStandardItemModel(mapTable);
	mapTable->setModel(_mapModel);
	_client = 0;
	connect(btnConnect, SIGNAL(clicked(bool)), SLOT(btnConnectClicked(bool)));
	connect(btnRefresh, SIGNAL(clicked(bool)), SLOT(btnRefreshClicked(bool)));
	connect(btnImport, SIGNAL(clicked(bool)), SLOT(btnImportClicked(bool)));
	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked(bool)));
	updateTableHeader();
}

void LSCPImport::btnConnectClicked(bool)
{
	QString host("localhost");
	int port = 8888;
	if(!txtHost->text().isEmpty())
	{
		host = txtHost->text();
		port = txtPort->value();
		if(!_client)
			_client = new LSClient(host.toUtf8().constData(), port);
	}
	else
	{
		if(!_client)
			_client = new LSClient();
	}
	if(!_client->startClient())
	{
		QString str = QString("Linuxsample LSCP server connection failed while connecting to: %1 port %1").arg(host).arg(port);
		QMessageBox::critical(this, tr("OOMidi: Server connection failed"), str);
		delete _client;
		_client = 0;
	}
	else
	{
		btnRefreshClicked(true);
	}
}

void LSCPImport::btnRefreshClicked(bool)
{
	if(_client)
	{
		_mapModel->clear();
		MidiInstrumentList* instr = _client->getInstruments();
		if(instr)
		{
			for(iMidiInstrument i = instr->begin(); i != instr->end(); ++i)
			{
				if ((*i)->filePath().isEmpty())
					continue;
				QList<QStandardItem*> rowData;
				QStandardItem* chk = new QStandardItem(true);
				chk->setCheckable(true);
				chk->setCheckState(Qt::Checked);
				rowData.append(chk);
				QStandardItem* ins = new QStandardItem((*i)->iname());
				QVariant v = qVariantFromValue((void*) (*i));
				ins->setData(v, Qt::UserRole);
				ins->setEditable(false);
				rowData.append(ins);
				QStandardItem* fname = new QStandardItem((*i)->filePath());
				fname->setEditable(true);
				rowData.append(fname);
				_mapModel->appendRow(rowData);
			}
		}
		updateTableHeader();
	}
}

void LSCPImport::btnImportClicked(bool)
{
	for(int i = 0; i < _mapModel->rowCount(); ++i)
	{
		QStandardItem* chk = _mapModel->item(i, 0);
		if(chk->checkState() == Qt::Unchecked)
			continue;
		QStandardItem* ins = _mapModel->item(i, 1);
		QStandardItem* fpath = _mapModel->item(i, 2);
		MidiInstrument* mi = (MidiInstrument*) ins->data(Qt::UserRole).value<void*>();
		mi->setFilePath(fpath->text());
		FILE* f = fopen(fpath->text().toAscii().constData(), "w");
		if (f == 0)
		{
			//if(debugMsg)
			//  printf("READ IDF %s\n", fi->filePath().toLatin1().constData());
			QString s("Creating file failed: ");
			s += QString(strerror(errno));
			QMessageBox::critical(this, tr("OOMidi: Create file failed"), s);
		}

		Xml xml(f);
		mi->write(0, xml);
		if (fclose(f) != 0)
		{
			QString s = QString("Write File\n") + fpath->text() + QString("\nfailed: ") + QString(strerror(errno));
			QMessageBox::critical(this, tr("OOMidi: Write File failed"), s);
		}
	}
}

void LSCPImport::btnCloseClicked(bool)
{
	//force disconnect on the client and quit
	if(_client)
	{
		_client->stopClient();
		delete _client;
		_client = 0;
	}
	close();
}
void LSCPImport::updateTableHeader()/*{{{*/
{
	QStandardItem* chk = new QStandardItem(tr("I"));
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

void LSCPImport::closeEvent(QCloseEvent* e)
{
	//force disconnect on the client and quit
	if(_client)
	{
		_client->stopClient();
		delete _client;
		_client = 0;
	}
	e->accept();
}
#endif
