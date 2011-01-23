//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================
//

#include "song.h"
#include "tviewdock.h"
#include "trackview.h"
#include "tvieweditor.h"

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
	tableView->setObjectName("tblTrackView");
	btnUp->setIcon(*upPCIcon);
	btnDown->setIcon(*downPCIcon);
	btnDelete->setIcon(*garbagePCIcon);
	btnUp->setIconSize(upPCIcon->size());
	btnDown->setIconSize(downPCIcon->size());
	btnTV->setIcon(*addTVIcon);
	btnTV->setIconSize(addTVIcon->size());
	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(btnUpClicked(bool)));
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(btnDownClicked(bool)));
	connect(btnTV, SIGNAL(clicked(bool)), SLOT(btnTVClicked(bool)));
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
		tableView->setRowHeight(_tableModel->rowCount(), 25);
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

void TrackViewDock::btnTVClicked(bool)
{
	TrackViewEditor* tve = new TrackViewEditor(this);
	tve->show();
}

void TrackViewDock::btnUpClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows();
	if (!rows.isEmpty())
	{
		int id = rows.at(0);
		if ((id - 1) < 0)
			return;
		int row = (id - 1);
		QList<QStandardItem*> item = _tableModel->takeRow(id);
		QStandardItem* txt = item.at(1);
		if(txt)
		{
			QString tname = txt->text();
			TrackView* tv = song->findTrackView(tname);
			if(tv)
			{
				TrackViewList* tvl = song->trackviews();
				tvl->erase(tv);
				song->insertTrackView(tv, row);
			}
		}
		//_selModel->blockSignals(true);
		tableView->selectRow(row);
		//_selModel->blockSignals(false);
	}
}/*}}}*/

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
		QStandardItem* txt = item.at(1);
		if(txt)
		{
			QString tname = txt->text();
			TrackView* tv = song->findTrackView(tname);
			if(tv)
			{
				TrackViewList* tvl = song->trackviews();
				tvl->erase(tv);
				song->insertTrackView(tv, row);
			}
		}
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
					dlist.append(tv);
				}
			}
		}
		if (!dlist.isEmpty())
		{
			for (int d = 0; d < dlist.size(); ++d)
			{
				song->removeTrackView(dlist.at(d));
			}
		}
	}
}/*}}}*/

QList<int> TrackViewDock::getSelectedRows()/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = tableView->selectionModel();
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
