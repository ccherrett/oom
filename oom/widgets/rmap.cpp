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
	routeList->installEventFilter(oom);
	_listModel = new QStandardItemModel(routeList);
	routeList->setModel(_listModel);
	//routeList->setObjectName("rmDockRouteList");

	btnDelete->setIcon(*garbageIconSet3);
	//btnDelete->setIconSize(garbagePCIcon->size());
	btnAdd->setIcon(*plusIconSet3);
	//btnAdd->setIconSize(addTVIcon->size());
	btnEdit->setIcon(*route_editIconSet3);
	//btnEdit->setIconSize(midi_edit_instrumentIcon->size());
	btnLoad->setIcon(*loadIconSet3);
	//btnLoad->setIconSize(midi_reset_instrIcon->size());
	btnCopy->setIcon(*duplicateIconSet3);
	//btnCopy->setIconSize(duplicatePCIcon->size());
	btnLink->setIcon(*connectIconSet3);
	//btnLink->setIconSize(midi_inputpluginsIcon->size());
	btnClear->setIcon(*garbageIconSet3);
	//btnClear->setIconSize(midi_inputpluginsIcon->size());

	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));
	connect(btnAdd, SIGNAL(clicked(bool)), SLOT(btnAddClicked(bool)));
	connect(btnEdit, SIGNAL(clicked(bool)), SLOT(btnEditClicked(bool)));
	connect(btnLoad, SIGNAL(clicked(bool)), SLOT(btnLoadClicked(bool)));
	connect(btnCopy, SIGNAL(clicked(bool)), SLOT(btnCopyClicked(bool)));
	connect(btnLink, SIGNAL(clicked(bool)), SLOT(btnLinkClicked(bool)));
	connect(btnClear, SIGNAL(clicked(bool)), SLOT(btnClearClicked(bool)));
	connect(_listModel, SIGNAL(itemChanged(QStandardItem*)), SLOT(renameRouteMap(QStandardItem*)));
	connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));
	populateTable(-1);
}

void RouteMapDock::btnDeleteClicked(bool)/*{{{*/
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
			//printf("RouteMapDock::btnDeleteClicked() Deleting: %s\n", tname.toLatin1().constData());
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
}/*}}}*/

void RouteMapDock::btnAddClicked(bool)/*{{{*/
{
	RouteMapDialog* rmd = new RouteMapDialog(false, this);
	connect(rmd, SIGNAL(saveRouteMap(QString, QString)), SLOT(saveRouteMap(QString, QString)));
	rmd->exec();
	if(rmd)
		delete rmd;
	populateTable(-1);
}/*}}}*/

void RouteMapDock::btnEditClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		QStandardItem* item = _listModel->item(id, 1);
		//QStandardItem* note = _listModel->item(id, 0);
		QString note = oom->noteForRouteMapping(routePath + "/" + item->text() + ".orm");
		if(item)
		{
			RouteMapDialog* rmd = new RouteMapDialog(true, this);
			connect(rmd, SIGNAL(saveRouteMap(QString, QString)), SLOT(updateRouteMap(QString, QString)));
			rmd->setFileName(item->text());
			rmd->setNotes(note);
			rmd->exec();
			if(rmd)
				delete rmd;
		}
	}
	populateTable(-1);
}/*}}}*/

void RouteMapDock::btnCopyClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		QStandardItem* item = _listModel->item(id, 1);
		if(item)
		{
			QString tname = item->text();
			QString origname = routePath + "/" + tname + ".orm";
			QString part = " - Copy";
			QFileInfo f(origname);
			if(f.exists() && f.isFile() && f.isWritable())
			{
				QFile file(f.filePath());
				part += " ";
				for(int i = 1; true; ++i)
				{
					QString n;
					n.setNum(i);
					QString s = part + n;
					if(file.copy(routePath + "/" + tname + s + ".orm"))
						break;
				}
				populateTable(-1);
			}
		}
	}
}/*}}}*/

void RouteMapDock::btnLoadClicked(bool)/*{{{*/
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
			oom->loadRouteMapping(tname);
		}
	}
	populateTable(-1);
}/*}}}*/

void RouteMapDock::btnLinkClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		QStandardItem* path = _listModel->item(id, 0);
		if(path)
		{
			song->associatedRoute = path->text();
			song->dirty = true;
			songChanged(-1);
		}
	}
}/*}}}*/

void RouteMapDock::btnClearClicked(bool)/*{{{*/
{
	song->associatedRoute = "";
	song->dirty = true;
	songChanged(-1);
}/*}}}*/

void RouteMapDock::songChanged(int)
{
	//Update the label with the current link
	if(song->associatedRoute.isEmpty())
	{
		lblLinked->setText("[ Unlinked ]");
	}
	else
	{
		QFileInfo f(song->associatedRoute);
		lblLinked->setText(f.baseName());
	}
}

void RouteMapDock::saveRouteMap(QString _name, QString note)/*{{{*/
{
	if(_name.isEmpty())
		return;
	oom->saveRouteMapping(routePath + "/" + _name + ".orm", note);
	populateTable(-1);
}/*}}}*/

void RouteMapDock::updateRouteMap(QString _name, QString note)/*{{{*/
{
	if(_name.isEmpty())
		return;
	oom->updateRouteMapping(routePath + "/" + _name + ".orm", note);
	populateTable(-1);
}/*}}}*/

void RouteMapDock::renameRouteMap(QStandardItem* changed)/*{{{*/
{
	int row = changed->row();
	QStandardItem* item = _listModel->item(row, 0);
	if(item)
	{
		QString tname = changed->text();
		tname = routePath + "/" + tname + ".orm";
		QFileInfo f(item->text());
		if(f.exists() && f.isFile() && f.isWritable())
		{
			QFile file(f.filePath());
			if(!file.rename(tname))
			{
				QMessageBox::critical(this, QString("OOMidi: Error"),
							tr("Failed to rename map.\nPosible causes and map by that name already exists.\n\n%1").arg(file.fileName()));
			}
		}
	}
	populateTable(-1);
}/*}}}*/

void RouteMapDock::populateTable(int /*flag*/)/*{{{*/
{
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
		QStandardItem *chk = new QStandardItem(f.filePath());
		QStandardItem *tname = new QStandardItem(f.baseName());
		tname->setToolTip(note);
		chk->setToolTip(note);
		rowData.append(chk);
		rowData.append(tname);
		_listModel->blockSignals(true);
		_listModel->insertRow(_listModel->rowCount(), rowData);
		_listModel->blockSignals(false);
		routeList->setRowHeight(_listModel->rowCount(), 25);
	}
	updateTableHeader();
	//routeList->resizeRowsToContents();
}/*}}}*/

void RouteMapDock::updateTableHeader()/*{{{*/
{
	QStandardItem* hstat = new QStandardItem("");
	QStandardItem* hpatch = new QStandardItem(tr("Route Maps"));
	_listModel->setHorizontalHeaderItem(0, hstat);
	_listModel->setHorizontalHeaderItem(1, hpatch);
	routeList->setColumnWidth(0, 1);
	routeList->setColumnHidden(0, true);
	routeList->horizontalHeader()->setStretchLastSection(true);
	routeList->verticalHeader()->hide();
}/*}}}*/

QList<int> RouteMapDock::getSelectedRows()/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = routeList->selectionModel();
	if (smodel->hasSelection())
	{
		QModelIndexList indexes = smodel->selectedRows();
		QList<QModelIndex>::const_iterator id;
		for (id = indexes.constBegin(); id != indexes.constEnd(); ++id)
		{
			int row = (*id).row();
			rv.append(row);
		}
	}
	return rv;
}/*}}}*/
