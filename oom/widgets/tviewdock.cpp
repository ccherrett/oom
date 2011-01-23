//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================
//

#include "song.h"
#include "tviewdock.h"
#include "trackview.h"

#include "icons.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QModelIndex>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QModelIndexList>
#include <QList>

TrackViewDock::TrackViewDock(QWidget* parent) : QFrame(parent)
{
	setupUi(this);
	_tableModel = new QStandardItemModel(tableView);
	tableView->setModel(_tableModel);
	btnUp->setIcon(*upPCIcon);
	btnDown->setIcon(*downPCIcon);
	btnDelete->setIcon(*garbagePCIcon);
	btnUp->setIconSize(upPCIcon->size());
	btnDown->setIconSize(downPCIcon->size());
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(btnUpClicked(bool)));
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(btnDownClicked(bool)));
	connect(_tableModel, SIGNAL(rowsInserted(QModelIndex, int, int)), SLOT(trackviewInserted(QModelIndex, int, int)));
	connect(_tableModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), SLOT(trackviewRemoved(QModelIndex, int, int)));
	connect(_tableModel, SIGNAL(itemChanged(QStandardItem*)), SLOT(trackviewChanged(QStandardItem*)));
	connect(song, SIGNAL(songChanged(int)), SLOT(populateTable(int)));
	populateTable(SC_VIEW_CHANGED);
	//updateTableHeader();
}

TrackViewDock::~TrackViewDock()
{
	//Write preshutdown hooks
}

void TrackViewDock::populateTable(int /*flag*/)
{
	printf("TrackViewDock::populateTable(int flag) fired\n");
	TrackViewList* tviews = song->trackviews();
	_tableModel->clear();
	for(iTrackView it = tviews->begin(); it != tviews->end(); ++it)
	{
		QList<QStandardItem*> rowData;
		QStandardItem *chk = new QStandardItem(true);
		chk->setCheckable(true);
		chk->setCheckState((*it)->selected() ? Qt::Checked : Qt::Unchecked);
		QStandardItem *tname = new QStandardItem((*it)->viewName());
		rowData.append(chk);
		rowData.append(tname);
		_tableModel->blockSignals(true);
		_tableModel->insertRow(_tableModel->rowCount(), rowData);
		_tableModel->blockSignals(false);
		tableView->setRowHeight(_tableModel->rowCount(), 50);
	}
	updateTableHeader();
	tableView->resizeRowsToContents();
}

void TrackViewDock::trackviewChanged(QStandardItem *item)
{
	if(item)
	{
		int row = item->row();
		QStandardItem *tname = _tableModel->item(row, 1);
		if(tname)
		{
			TrackView* tv = song->findTrackView(tname->text());
			if(tv)
			{
				tv->setSelected((item->checkState() == Qt::Checked) ? true : false);
				song->updateTrackViews1();
			}
		}
	}
}

void TrackViewDock::trackviewInserted(QModelIndex, int, int)
{
}

void TrackViewDock::trackviewRemoved(QModelIndex, int, int)
{
}

void TrackViewDock::btnUpClicked(bool)
{
	QList<int> rows;// = tableView->getSelectedRows();/*{{{*/
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id - 1) < 0)
			return;
		int row = (id - 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		QStandardItem* txt = item.at(1);
		txt->setEditable(false);
		//_selectedIndex = row;
		_tableModel->insertRow(row, item);
		tableView->setRowHeight(row, 50);
		tableView->resizeRowsToContents();
		tableView->setColumnWidth(0, 20);
		//_selModel->blockSignals(true);
		tableView->selectRow(row);
		//_selModel->blockSignals(false);
	}/*}}}*/
}

void TrackViewDock::btnDownClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id + 1) >= _tableModel->rowCount())
			return;
		int row = (id + 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		QStandardItem* txt = item.at(2);
		txt->setEditable(false);
		//_selectedIndex = row;
		_tableModel->insertRow(row, item);
		tableView->setRowHeight(row, 50);
		tableView->resizeRowsToContents();
		tableView->setColumnWidth(1, 20);
		tableView->setColumnWidth(0, 1);
		//_selModel->blockSignals(true);
		tableView->selectRow(row);
		//_selModel->blockSignals(false);
	}
}/*}}}*/

void TrackViewDock::btnDeleteClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		printf("Found rows to delete\n");
		QList<TrackView*> dlist;
		int id = 0;
		TrackViewList* tviews = song->trackviews();
		for (int i = 0; i < rows.size(); ++i)
		{
			int r = rows.at(i);
			QStandardItem *item = _tableModel->item(r, 1);
			if(item)
			{
				TrackView* tv = song->findTrackView(item->text());
				if(tv)
				{
					printf("Found tv to delete\n");
					dlist.append(tv);
				}
			}
		}
		if (!dlist.isEmpty())
		{
			for (int d = 0; d < dlist.size(); ++d)
			{
				printf("Deleting item\n");
				song->removeTrackView(dlist.at(d));
			}
		}
		//updateTableHeader();
	}
}/*}}}*/

QList<int> TrackViewDock::getSelectedRows()/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = tableView->selectionModel();
	if (smodel->hasSelection())
	{
		printf("TrackViewDock::getSelectedRows() found selected rows\n");
		QModelIndexList indexes = smodel->selectedRows();
		printf("TrackViewDock::getSelectedRows()\n");
		QList<QModelIndex>::const_iterator id;
		for (id = indexes.constBegin(); id != indexes.constEnd(); ++id)
		{
			printf("Appending selected row\n");
			int row = (*id).row();
			rv.append(row);
		}
	}
	return rv;
}/*}}}*/

void TrackViewDock::updateTableHeader()/*{{{*/
{
	QStandardItem* hstat = new QStandardItem(true);
	hstat->setCheckable(true);
	hstat->setCheckState(Qt::Unchecked);
	QStandardItem* hpatch = new QStandardItem(tr("TrackView"));
	_tableModel->setHorizontalHeaderItem(0, hstat);
	_tableModel->setHorizontalHeaderItem(1, hpatch);
	tableView->setColumnWidth(0, 20);
	tableView->horizontalHeader()->setStretchLastSection(true);
	tableView->verticalHeader()->hide();
}/*}}}*/
