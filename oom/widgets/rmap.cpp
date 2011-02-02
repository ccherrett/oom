//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QStandardItemModel>
#include <QStandardItem>
#include <QModelIndex>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QModelIndexList>
#include <QList>
//#include <QLineEdit>
#include <QMessageBox>
#include <QDir>
#include <QFileInfoList>
#include <QFileInfo>

#include "rmap.h"
#include "rmapdialog.h"
#include "icons.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "app.h"

RouteMapDock::RouteMapDock(QWidget* parent) : QFrame(parent)
{
	setupUi(this);
	_listModel = new QStandardItemModel(routeList);
	routeList->setModel(_listModel);
	//routeList->setObjectName("rmDockRouteList");

	btnDelete->setIcon(*garbagePCIcon);
	btnDelete->setIconSize(garbagePCIcon->size());
	btnAdd->setIcon(*addTVIcon);
	btnAdd->setIconSize(addTVIcon->size());
	btnEdit->setIcon(*midi_edit_instrumentIcon);
	btnEdit->setIconSize(midi_edit_instrumentIcon->size());
	btnLoad->setIcon(*midi_reset_instrIcon);
	btnLoad->setIconSize(midi_reset_instrIcon->size());
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));
	connect(btnAdd, SIGNAL(clicked(bool)), SLOT(btnAddClicked(bool)));
	connect(btnEdit, SIGNAL(clicked(bool)), SLOT(btnEditClicked(bool)));
	connect(btnLoad, SIGNAL(clicked(bool)), SLOT(btnLoadClicked(bool)));
	connect(song, SIGNAL(songChanged(int)), SLOT(populateTable(int)));
	populateTable(-1);
}

void RouteMapDock::btnDeleteClicked(bool)
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		QStandardItem* item = _listModel->item(id, 1);
		if(item)
		{
			QString tname = item->text();
			tname = routePath + "/" + tname + ".orm";
			printf("RouteMapDock::btnDeleteClicked() Deleting: %s\n", tname.toLatin1().constData());
			QFileInfo f(tname);
			if(f.exists() && f.isFile() && f.isWritable())
			{
				QFile file(f.filePath());
				if(QMessageBox::question(this, QString("OOMidi: Delete?"),
										tr("Are you sure you want to delete route map from disk?\n\n%1")
										.arg(file.fileName()), QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok)
				{
					file.remove();
					populateTable(-1);
				}
			}
			//oom->loadRouteMapping(tname);
		}
	}
}

void RouteMapDock::btnAddClicked(bool)
{
	RouteMapDialog* rmd = new RouteMapDialog(false, this);
	connect(rmd, SIGNAL(saveRouteMap(QString, QString)), SLOT(saveRouteMap(QString, QString)));
	rmd->exec();
	if(rmd)
		delete rmd;
}

void RouteMapDock::btnEditClicked(bool)
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		QStandardItem* item = _listModel->item(id, 1);
		QStandardItem* note = _listModel->item(id, 0);
		if(item && note)
		{
			RouteMapDialog* rmd = new RouteMapDialog(true, this);
			connect(rmd, SIGNAL(saveRouteMap(QString, QString)), SLOT(updateRouteMap(QString, QString)));
			QString tname = item->text();
			rmd->setFileName(item->text());
			rmd->setNotes(note->text());
			rmd->exec();
			if(rmd)
				delete rmd;
		}
	}
}

void RouteMapDock::saveRouteMap(QString _name, QString note)
{
	if(_name.isEmpty())
		return;
	oom->saveRouteMapping(routePath + "/" + _name + ".orm", note);
	populateTable(-1);
}

void RouteMapDock::btnLoadClicked(bool)
{
	printf("RouteMapDock::btnLoadClicked(bool)\n");
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		QStandardItem* item = _listModel->item(id, 1);
		if(item)
		{
			QString tname = item->text();
			tname = routePath + "/" + tname + ".orm";
			printf("RouteMapDock::btnLoadClicked() Loading: %s\n", tname.toLatin1().constData());
			oom->loadRouteMapping(tname);
		}
	}
}

void RouteMapDock::updateRouteMap(QString _name, QString note)
{
	if(_name.isEmpty())
		return;
	oom->updateRouteMapping(routePath + "/" + _name + ".orm", note);
	populateTable(-1);
}

void RouteMapDock::renameRouteMap(QString, QString)
{
}

void RouteMapDock::populateTable(int /*flag*/)/*{{{*/
{
	//printf("TrackViewDock::populateTable(int flag) fired\n");
	_listModel->clear();
	QDir routes;
	routes.setFilter(QDir::Files | QDir::NoSymLinks);
	if(!routes.cd(routePath))
		return;
	QFileInfoList files = routes.entryInfoList();
	for(int it = 0; it < files.size(); ++it)
	{
		QFileInfo f = files.at(it);
		QString note = oom->noteForRouteMapping(f.filePath());
		QList<QStandardItem*> rowData;
		QStandardItem *chk = new QStandardItem(note);
		QStandardItem *tname = new QStandardItem(f.baseName());
		tname->setToolTip(note);
		chk->setToolTip(note);
		printf("TrackViewDock::populateTable() Note: %s\n", note.toLatin1().constData());
		rowData.append(chk);
		rowData.append(tname);
		_listModel->blockSignals(true);
		_listModel->insertRow(_listModel->rowCount(), rowData);
		_listModel->blockSignals(false);
		routeList->setRowHeight(_listModel->rowCount(), 25);
	}
	updateTableHeader();
	routeList->resizeRowsToContents();
}/*}}}*/

void RouteMapDock::updateTableHeader()/*{{{*/
{
	QStandardItem* hstat = new QStandardItem("");
	QStandardItem* hpatch = new QStandardItem(tr("Route Maps"));
	_listModel->setHorizontalHeaderItem(0, hstat);
	_listModel->setHorizontalHeaderItem(1, hpatch);
	routeList->setColumnWidth(0, 1);
	routeList->horizontalHeader()->setStretchLastSection(true);
	routeList->verticalHeader()->hide();
}/*}}}*/

QList<int> RouteMapDock::getSelectedRows()/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = routeList->selectionModel();
	if (smodel->hasSelection())
	{
		printf("Has Selected\n");
		QModelIndexList indexes = smodel->selectedRows();
		QList<QModelIndex>::const_iterator id;
		for (id = indexes.constBegin(); id != indexes.constEnd(); ++id)
		{
			printf("Selected \n");
			int row = (*id).row();
			rv.append(row);
		}
	}
	return rv;
}/*}}}*/
