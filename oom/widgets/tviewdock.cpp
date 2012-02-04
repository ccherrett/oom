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
#include "Composer.h"

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
	_templateModel = new QStandardItemModel(templateView);
	m_icons << QIcon(":/images/icons/views_inputs.png") << QIcon(":/images/icons/views_outputs.png") << QIcon(":/images/icons/views_busses.png") << QIcon(":/images/icons/views_auxs.png");
	tableView->setModel(_tableModel);
	tableView->setObjectName("tblTrackView");
	autoTable->setObjectName("tblAutoTable");
	autoTable->setModel(_autoTableModel);
	templateView->setModel(_templateModel);
	
	btnUp->setIcon(QIcon(*up_arrowIconSet3));
	btnDown->setIcon(*down_arrowIconSet3);
	btnDelete->setIcon(*garbageIconSet3);
	btnTV->setIcon(*plusIconSet3);

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
	populateTable(-1);
	//updateTableHeader();
}

TrackViewDock::~TrackViewDock()
{
	//Write preshutdown hooks
}

void TrackViewDock::toggleButtons(bool show)
{
	btnUp->setVisible(show);
	btnDown->setVisible(show);
	btnDelete->setVisible(show);
	btnTV->setVisible(show);
	chkViewAll->setVisible(show);
}

void TrackViewDock::populateTable(int flag, bool)/*{{{*/
{
	if(flag & (SC_VIEW_CHANGED | SC_VIEW_DELETED | SC_VIEW_ADDED) || flag == -1)
	{
		//printf("TrackViewDock::populateTable(int flag) fired\n");
		_tableModel->clear();
		QList<qint64> *idlist = song->trackViewIndexList();
		TrackViewList* tvlist = song->trackviews();
		for(int i = 0; i < idlist->size(); ++i)
		{
			qint64 tvid = idlist->at(i);
			TrackView* tv = tvlist->value(tvid);
			if(tv)
			{
				//QList<QStandardItem*> rowData;
				QStandardItem *chk = new QStandardItem(tv->viewName());
				chk->setCheckable(true);
				chk->setCheckState(tv->selected() ? Qt::Checked : Qt::Unchecked);
				chk->setData(tv->id());
				_tableModel->blockSignals(true);
				_tableModel->insertRow(_tableModel->rowCount(), chk);
				_tableModel->blockSignals(false);
				tableView->setRowHeight(_tableModel->rowCount(), 25);
			}
		}
		_autoTableModel->clear();
		int icon_index = 0;
		QList<int> list;
		list << Track::MIDI << Track::AUDIO_INPUT << Track::AUDIO_OUTPUT << Track::AUDIO_BUSS << Track::AUDIO_AUX << Track::WAVE;
		idlist = song->autoTrackViewIndexList();
		tvlist = song->autoviews();
		for(int i = 0; i < idlist->size(); ++i)
		{
			TrackView* v = tvlist->value(idlist->at(i));;
			if(v)
			{
				//QList<QStandardItem*> rowData2;
				QStandardItem *chk = new QStandardItem(v->viewName());
				chk->setCheckable(true);
				chk->setCheckState(v->selected() ? Qt::Checked : Qt::Unchecked);
				//QStandardItem *tname = new QStandardItem(v->viewName());
				chk->setData(v->id());
				if(v->id() != song->workingViewId() && v->id() != song->commentViewId())
				{
					//chk->setForeground(QBrush(QColor(g_trackColorListSelected.value(list.at(i)))));
					//tname->setForeground(QBrush(QColor(g_trackColorListSelected.value(list.at(index)))));
					chk->setIcon(m_icons.at(icon_index));
					++icon_index;
				}
				//rowData2.append(chk);
				//rowData2.append(tname);
				_autoTableModel->blockSignals(true);
				_autoTableModel->insertRow(_autoTableModel->rowCount(), chk);
				_autoTableModel->blockSignals(false);
				autoTable->setRowHeight(_autoTableModel->rowCount(), 25);
			}
		}
		updateTableHeader();
		tableView->resizeRowsToContents();
		autoTable->resizeRowsToContents();
		//if(bar)
		//{
		//	qDebug() << "Setting ScrollBar position to: " << barPos;
		//	bar->setSliderPosition(barPos);
		//}
	}
}/*}}}*/

void TrackViewDock::trackviewChanged(QStandardItem *item)/*{{{*/
{
	if(item)
	{
		/*int row = item->row();
		QStandardItem *tname = _tableModel->item(row, 1);
		QStandardItem *chk = _tableModel->item(row, 0);
		if(tname)
		{*/
			TrackView* tv = song->findTrackViewById(item->data().toLongLong());
			if(tv)
			{
				tv->setSelected(item->checkState() == Qt::Checked);
				song->updateTrackViews();
			}
		//}
	}
}/*}}}*/

void TrackViewDock::autoTrackviewChanged(QStandardItem *item)/*{{{*/
{
	if(item)
	{
		//int row = item->row();
		//QStandardItem *tname = _autoTableModel->item(row, 1);
		//QStandardItem *chk = _autoTableModel->item(row, 0);
		//if(tname)
		//{
			TrackViewList* tvlist = song->autoviews();
			TrackView* tv = tvlist->value(item->data().toLongLong());//song->findAutoTrackView(tname->text());
			if(tv)
			{
				qDebug("TrackViewDock::autoTrackviewChanged found track view: %s", item->text().toUtf8().constData());
				tv->setSelected(item->checkState() == Qt::Checked);
				song->updateTrackViews();
			}
		//}
	}
}/*}}}*/

void TrackViewDock::updateTrackView(int table, QStandardItem *item)/*{{{*/
{
	if(item)
	{
		/*int row = item->row();
		QStandardItem *tname;
		if(table)
			tname = _autoTableModel->item(row, 1);
		else
			tname = _tableModel->item(row, 1);
		if(tname)
		{*/
			TrackView* tv;
			if(table)
				tv = song->findAutoTrackView(item->text());
			else
				tv = song->findTrackViewById(item->data().toLongLong());
			if(tv)
			{
				tv->setSelected((item->checkState() == Qt::Checked) ? true : false);
				song->updateTrackViews();
			}
		//}
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
		//QStandardItem* tvcol = _tableModel->item(item->row(), 1);
		//if(tvcol)
		//{
			TrackView *tv = song->findTrackViewById(item->data().toLongLong());
			if(tv)
			{
				QMenu* p = new QMenu(this);
				TrackViewMenu *itemMenu = new TrackViewMenu(p, tv, (chkViewAll->checkState() == Qt::Checked));
				p->addAction(itemMenu);

				QAction* act = p->exec(QCursor::pos());
				if (act)
				{
					QString tname = act->data().toString();
					Track* track = song->findTrack(tname);
					if(track)
					{
						if(track->type() == Track::WAVE)
							oom->importWave(track);
						else
							oom->composer->addCanvasPart(track);
						song->updateTrackViews();
					}
				}
				delete p;
			}
		//}
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
		QStandardItem* txt = item.at(0);
		if(txt)
		{
			qint64 tid = txt->data().toLongLong();
			TrackView* tv = song->findTrackViewById(tid);
			if(tv)
			{
				TrackViewList* tvl = song->trackviews();
				tvl->remove(tv->id());
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
		QStandardItem* txt = item.at(0);
		if(txt)
		{
			//QString tname = txt->text();
			TrackView* tv = song->findTrackViewById(txt->data().toLongLong());
			if(tv)
			{
				song->removeTrackView(tv->id());
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
		foreach (int r, rows)
		{
			QStandardItem *item = _tableModel->item(r);
			if(item)
			{
				TrackView* tv = song->findTrackViewById(item->data().toLongLong());
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
				song->removeTrackView(dlist.at(d)->id());
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
	QStandardItem* hpatch = new QStandardItem(tr("Custom Views"));
	_tableModel->setHorizontalHeaderItem(0, hpatch);
	tableView->horizontalHeader()->setStretchLastSection(true);
	tableView->verticalHeader()->hide();

	QStandardItem* ahpatch = new QStandardItem(tr("Views"));
	_autoTableModel->setHorizontalHeaderItem(0, ahpatch);
	autoTable->horizontalHeader()->setStretchLastSection(true);
	autoTable->verticalHeader()->hide();

	_templateModel->setHorizontalHeaderItem(0, new QStandardItem(tr("Templates")));
	templateView->verticalHeader()->hide();
	templateView->horizontalHeader()->setStretchLastSection(true);
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
