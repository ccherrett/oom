//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================
//

#include "app.h"
#include "globals.h"
#include "gconfig.h"
#include "song.h"
#include "audio.h"
#include "tviewdock.h"
#include "tviewmenu.h"
#include "trackview.h"
#include "tvieweditor.h"
#include "arranger.h"

#include "icons.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QModelIndex>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QModelIndexList>
#include <QList>
#include <QtGui>

TrackViewDock::TrackViewDock(QWidget* parent) : QFrame(parent)
{
	setupUi(this);
	tableView->installEventFilter(oom);
	autoTable->installEventFilter(oom);
	_tableModel = new QStandardItemModel(tableView);
	_autoTableModel = new QStandardItemModel(autoTable);
	tableView->setModel(_tableModel);
	tableView->setObjectName("tblTrackView");
	autoTable->setModel(_autoTableModel);
	btnUp->setIcon(*upPCIcon);
	btnDown->setIcon(*downPCIcon);
	btnDelete->setIcon(*garbagePCIcon);
	btnDelete->setIconSize(garbagePCIcon->size());
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
	connect(_autoTableModel, SIGNAL(itemChanged(QStandardItem*)), SLOT(autoTrackviewChanged(QStandardItem*)));
	connect(tableView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextPopupMenu(QPoint)));
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
	//printf("TrackViewDock::populateTable(int flag) fired\n");
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
	_autoTableModel->clear();
	for(iTrackView ait = song->autoviews()->begin(); ait != song->autoviews()->end(); ++ait)
	{
		QList<QStandardItem*> rowData2;
		QStandardItem *chk = new QStandardItem(true);
		chk->setCheckable(true);
		chk->setCheckState((*ait)->selected() ? Qt::Checked : Qt::Unchecked);
		QStandardItem *tname = new QStandardItem((*ait)->viewName());
		rowData2.append(chk);
		rowData2.append(tname);
		_autoTableModel->blockSignals(true);
		_autoTableModel->insertRow(_autoTableModel->rowCount(), rowData2);
		_autoTableModel->blockSignals(false);
		autoTable->setRowHeight(_autoTableModel->rowCount(), 25);
	}
	updateTableHeader();
	tableView->resizeRowsToContents();
	autoTable->resizeRowsToContents();
}

void TrackViewDock::trackviewChanged(QStandardItem *item)
{
	if(item)
	{
		int row = item->row();
		QStandardItem *tname = _tableModel->item(row, 1);
		QStandardItem *chk = _tableModel->item(row, 0);
		if(tname)
		{
			TrackView* tv = song->findTrackView(tname->text());
			if(tv)
			{
				tv->setSelected((chk->checkState() == Qt::Checked) ? true : false);
				song->updateTrackViews1();
			}
		}
	}
}

void TrackViewDock::autoTrackviewChanged(QStandardItem *item)
{
	if(item)
	{
		int row = item->row();
		QStandardItem *tname = _autoTableModel->item(row, 1);
		QStandardItem *chk = _autoTableModel->item(row, 0);
		if(tname)
		{
			TrackView* tv = song->findAutoTrackView(tname->text());
			if(tv)
			{
				tv->setSelected((chk->checkState() == Qt::Checked) ? true : false);
				song->updateTrackViews1();
			}
		}
	}
}

void TrackViewDock::updateTrackView(int table, QStandardItem *item)/*{{{*/
{
	if(item)
	{
		int row = item->row();
		QStandardItem *tname;
		if(table)
			tname = _autoTableModel->item(row, 1);
		else
			tname = _tableModel->item(row, 1);
		if(tname)
		{
			TrackView* tv;
			if(table)
				tv = song->findAutoTrackView(tname->text());
			else
				tv = song->findTrackView(tname->text());
			if(tv)
			{
				tv->setSelected((item->checkState() == Qt::Checked) ? true : false);
				song->updateTrackViews1();
			}
		}
	}
}/*}}}*/

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

void TrackViewDock::contextPopupMenu(QPoint pos)/*{{{*/
{
	QModelIndex index = tableView->indexAt(pos);
	if(!index.isValid())
		return;
	QStandardItem* item = _tableModel->itemFromIndex(index);
	if(item)
	{
		//Make it works even if you rightclick on the checkbox
		QStandardItem* tvcol = _tableModel->item(item->row(), 1);
		if(tvcol)
		{
			TrackView *tv = song->findTrackView(tvcol->text());
			if(tv)
			{
				QMenu* p = new QMenu(this);/*{{{*/
				TrackViewMenu *item = new TrackViewMenu(p, tv);
				p->addAction(item);

				QAction* act = p->exec(mapToGlobal(pos));
				if (act)
				{
					QString tname = act->data().toString();
					Track* track = song->findTrack(tname);
					if(track)
					{
						oom->arranger->addCanvasPart(track);
					}
				}
				delete p;/*}}}*/
			}
		}
	}
}/*}}}*/

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
	QStandardItem* hpatch = new QStandardItem(tr("Custom Views"));
	_tableModel->setHorizontalHeaderItem(0, hstat);
	_tableModel->setHorizontalHeaderItem(1, hpatch);
	tableView->setColumnWidth(0, 20);
	tableView->horizontalHeader()->setStretchLastSection(true);
	tableView->verticalHeader()->hide();

	QStandardItem* ahstat = new QStandardItem(true);
	ahstat->setCheckable(true);
	ahstat->setCheckState(Qt::Unchecked);
	QStandardItem* ahpatch = new QStandardItem(tr("Views"));
	_autoTableModel->setHorizontalHeaderItem(0, ahstat);
	_autoTableModel->setHorizontalHeaderItem(1, ahpatch);
	autoTable->setColumnWidth(0, 20);
	autoTable->horizontalHeader()->setStretchLastSection(true);
	autoTable->verticalHeader()->hide();
}/*}}}*/

void TrackViewDock::selectStaticView(int n)
{
	Track::TrackType type = (Track::TrackType)n;
	QStandardItem *item = 0;
	switch(type)
	{
		case Track::AUDIO_INPUT:
			item = _autoTableModel->item(1, 0);
		    break;
		case Track::AUDIO_OUTPUT:
			item = _autoTableModel->item(2, 0);
		    break;
		case Track::AUDIO_BUSS:
			item = _autoTableModel->item(3, 0);
		    break;
		case Track::AUDIO_AUX:
			item = _autoTableModel->item(4, 0);
		    break;
		default:
		break;
	}
	if(item && item->checkState() == Qt::Unchecked)
	{
		item->setCheckState(Qt::Checked);
	}
}
