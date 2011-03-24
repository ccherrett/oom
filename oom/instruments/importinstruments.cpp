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
#include "song.h"
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
#include <QShowEvent>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QVector>
#include <QMap>
#include <QtGui>
//#include <boost/bind.hpp>

using namespace QtConcurrent;

LSCPImport::LSCPImport(QWidget* parent) :QDialog(parent)
{
	setupUi(this);
	_mapModel = new QStandardItemModel(mapTable);
	m_futureWatcher = 0;
	mapTable->setModel(_mapModel);
	m_import_client = 0;
	txtHost->setText(config.lsClientHost);
	txtPort->setValue(config.lsClientPort);
	txtRetry->setValue(config.lsClientRetry);
	txtTimeout->setValue(config.lsClientTimeout);
	//connect(btnConnect, SIGNAL(clicked(bool)), SLOT(btnConnectClicked(bool)));
	connect(btnList, SIGNAL(clicked(bool)), SLOT(btnListClicked(bool)));
	connect(btnSelectAll, SIGNAL(clicked(bool)), SLOT(btnSelectAllClicked(bool)));
	connect(btnImport, SIGNAL(clicked(bool)), SLOT(btnImportClicked(bool)));
	connect(btnClose, SIGNAL(clicked(bool)), SLOT(btnCloseClicked(bool)));
	connect(btnSave, SIGNAL(clicked(bool)), SLOT(btnSaveClicked(bool)));
	connect(txtPort, SIGNAL(valueChanged(int)), SLOT(portValueChanged(int)));
	connect(txtRetry, SIGNAL(valueChanged(int)), SLOT(retryValueChanged(int)));
	connect(txtTimeout, SIGNAL(valueChanged(int)), SLOT(timeoutValueChanged(int)));
	connect(txtHost, SIGNAL(editingFinished()), SLOT(hostValueChanged()));
	updateTableHeader();
}

void LSCPImport::portValueChanged(int val)
{
	config.lsClientPort = val;
}

void LSCPImport::retryValueChanged(int val)
{
	config.lsClientRetry = val;
}

void LSCPImport::timeoutValueChanged(int val)
{
	config.lsClientTimeout = val;
}

void LSCPImport::hostValueChanged()
{
	config.lsClientHost = txtHost->text();
}

void LSCPImport::btnListClicked(bool)
{
	_mapModel->clear();
	btnSelectAll->blockSignals(true);
	btnSelectAll->setChecked(false);
	btnSelectAll->blockSignals(false);
	QString host = config.lsClientHost; //("localhost");
	int port = config.lsClientPort; //8888;
	//qDebug() << host;
	if(!m_import_client)
		m_import_client = new LSClient(host.toUtf8().constData(), port);
	m_import_client->setRetry(config.lsClientRetry);
	m_import_client->setTimeout(config.lsClientTimeout);
	if(!m_import_client->startClient())
	{
		QString str = QString("Linuxsampler LSCP server connection failed while connecting to: %1 on port %2").arg(host).arg(port);
		QMessageBox::critical(this, tr("OOMidi: Server connection failed"), str);
		delete m_import_client;
		m_import_client = 0;
	}
	else
	{
		QMap<int, QString> instr = m_import_client->listInstruments();
		if(!instr.isEmpty())
		{
			QList<int> keys = instr.keys();
			for(int i = 0; i < keys.size(); ++i)
			{
				QList<QStandardItem*> rowData;
				QStandardItem* chk = new QStandardItem(true);
				chk->setCheckable(true);
				chk->setCheckState(Qt::Unchecked);
				rowData.append(chk);
				QStandardItem* ins = new QStandardItem(QString::number(keys.at(i)));
				ins->setEditable(false);
				rowData.append(ins);
				QStandardItem* fname = new QStandardItem(instr.take(keys.at(i)));
				fname->setEditable(false);
				rowData.append(fname);
				_mapModel->appendRow(rowData);
			}
			updateTableHeader(true);
		}//Need to notify user here that nothing happened
		else
		{
			QMessageBox::information(this, tr("OOMidi: LSCP Client"), tr("No Instrument Maps found."));
		}
		m_import_client->stopClient();
		delete m_import_client;
		m_import_client = 0;
	}
}

MidiInstrument* redirLookup(int i)/*{{{*/
{
	LSClient m_lsclient(config.lsClientHost.toUtf8().constData(), config.lsClientPort);
	m_lsclient.setRetry(config.lsClientRetry);
	m_lsclient.setTimeout(config.lsClientTimeout);
	m_lsclient.startClient();
	MidiInstrument* rv = m_lsclient.getInstrument(i);
	m_lsclient.stopClient();
//	delete m_lsclient;
	
	return rv;
}/*}}}*/

void LSCPImport::btnImportClicked(bool)/*{{{*/
{
	QVector<int> maps;
	for(int i = 0; i < _mapModel->rowCount(); ++i)
	{
		QStandardItem* chk = _mapModel->item(i, 0);
		if(chk->checkState() == Qt::Unchecked)
			continue;
		QStandardItem* id = _mapModel->item(i, 1);
		maps.append(id->text().toInt());
	}
	if(!maps.isEmpty())
	{
		btnSelectAll->blockSignals(true);
		btnSelectAll->setChecked(false);
		btnSelectAll->blockSignals(false);
		_mapModel->clear();
		QProgressDialog dialog(this);
		dialog.setLabelText(QString("Progressing instrument %1 map(s)...").arg(maps.size()));

		m_futureWatcher = new QFutureWatcher<MidiInstrument*>(this);
		connect(m_futureWatcher, SIGNAL(finished()), &dialog, SLOT(reset()));
		connect(&dialog, SIGNAL(canceled()), m_futureWatcher, SLOT(cancel()));
		connect(m_futureWatcher, SIGNAL(progressRangeChanged(int,int)), &dialog, SLOT(setRange(int,int)));
		connect(m_futureWatcher, SIGNAL(progressValueChanged(int)), &dialog, SLOT(setValue(int)));
		connect(m_futureWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(appendInstrument(int)));
		
		m_futureWatcher->setFuture(QtConcurrent::mapped(maps, redirLookup));

		dialog.exec();

		m_futureWatcher->waitForFinished();

		/*MidiInstrumentList* instr = m_import_client->getInstruments(maps, txtRetry->value(), txtTimeout->value());
		if(instr)
		{
			_mapModel->clear();
			for(iMidiInstrument i = instr->begin(); i != instr->end(); ++i)
			{
				if ((*i)->filePath().isEmpty())
					continue;
				QList<QStandardItem*> rowData;
				QStandardItem* chk = new QStandardItem(true);
				chk->setCheckable(true);
				chk->setCheckState(Qt::Unchecked);
				rowData.append(chk);
				QStandardItem* ins = new QStandardItem((*i)->iname());
				ins->setEditable(true);
				QVariant v = qVariantFromValue((void*) (*i));
				ins->setData(v, Qt::UserRole);
				ins->setEditable(false);
				rowData.append(ins);
				QStandardItem* fname = new QStandardItem((*i)->filePath());
				fname->setEditable(true);
				rowData.append(fname);
				_mapModel->appendRow(rowData);
			}
			updateTableHeader();
		}*/
	}
}/*}}}*/

void LSCPImport::appendInstrument(int val)/*{{{*/
{
	if(m_futureWatcher)
	{
		MidiInstrument* in = m_futureWatcher->resultAt(val);

		if(in && !in->filePath().isEmpty())
		{
			QList<QStandardItem*> rowData;
			QStandardItem* chk = new QStandardItem(true);
			chk->setCheckable(true);
			chk->setCheckState(Qt::Unchecked);
			rowData.append(chk);
			QStandardItem* ins = new QStandardItem(in->iname());
			ins->setEditable(true);
			QVariant v = qVariantFromValue((void*) in);
			ins->setData(v, Qt::UserRole);
			ins->setEditable(false);
			rowData.append(ins);
			QStandardItem* fname = new QStandardItem(in->filePath());
			fname->setEditable(true);
			rowData.append(fname);
			_mapModel->appendRow(rowData);
			updateTableHeader();
		}
	}
}/*}}}*/


void LSCPImport::btnSaveClicked(bool)/*{{{*/
{
	for(int i = 0; i < _mapModel->rowCount(); ++i)
	{
		QStandardItem* chk = _mapModel->item(i, 0);
		if(chk->checkState() == Qt::Unchecked)
			continue;
		QStandardItem* ins = _mapModel->item(i, 1);
		QStandardItem* fpath = _mapModel->item(i, 2);
		MidiInstrument* mi = (MidiInstrument*) ins->data(Qt::UserRole).value<void*>();
		QFileInfo finfo(fpath->text());
		QDir dpath = finfo.dir();
		if(!dpath.exists())
		{
			dpath.mkpath(dpath.absolutePath());
		}
		if(dpath.exists() && !finfo.exists())//Dont overwrite
		{
			mi->setFilePath(fpath->text());
			FILE* f = fopen(fpath->text().toAscii().constData(), "w");
			if (f == 0)
			{
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
			else
			{
				midiInstruments.push_front(mi);
			}
		}
	}
	emit instrumentsImported();
	song->update(SC_CONFIG | SC_MIDI_CONTROLLER);
}/*}}}*/

void LSCPImport::btnSelectAllClicked(bool sel)/*{{{*/
{
	for(int i = 0; i < _mapModel->rowCount(); ++i)
	{
		QStandardItem* item = _mapModel->item(i, 0);
		if(item)
		{
			if(sel)
				item->setCheckState(Qt::Checked);
			else
				item->setCheckState(Qt::Unchecked);
		}
	}
}/*}}}*/

void LSCPImport::updateTableHeader(bool list)/*{{{*/
{
	QStandardItem* chk = new QStandardItem(tr("I"));
	QStandardItem* instr = new QStandardItem(list ? tr("Num") : tr("Instruments") );
	QStandardItem* fname = new QStandardItem(list ? tr("Instruments") : tr("File Name"));
	_mapModel->setHorizontalHeaderItem(0, chk);
	_mapModel->setHorizontalHeaderItem(1, instr);
	_mapModel->setHorizontalHeaderItem(2, fname);
	mapTable->setColumnWidth(0, 20);
	mapTable->setColumnWidth(1, list ? 50 : 250);
	mapTable->horizontalHeader()->setStretchLastSection(true);
	mapTable->verticalHeader()->hide();
	btnSave->setEnabled(!list);
	btnImport->setEnabled(list);
}/*}}}*/

void LSCPImport::showEvent(QShowEvent*)
{
	_mapModel->clear();
	btnSelectAll->blockSignals(true);
	btnSelectAll->setChecked(false);
	btnSelectAll->blockSignals(false);
}
void LSCPImport::btnCloseClicked(bool)
{
	//force disconnect on the client and quit
	close();
}

void LSCPImport::closeEvent(QCloseEvent* e)
{
	//force disconnect on the client and quit
	if(m_import_client)
	{
		m_import_client->stopClient();
		delete m_import_client;
		m_import_client = 0;
	}
	e->accept();
}
#endif
