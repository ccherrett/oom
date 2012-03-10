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
#include "TrackManager.h"

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
	connect(templateView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(templateContextPopupMenu(QPoint)));
	connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(song, SIGNAL(songChanged(int)), SLOT(populateTable(int)));
	connect(oom, SIGNAL(instrumentTemplateAdded(qint64)), this, SLOT(populateInstrumentTemplates()));
	connect(oom, SIGNAL(instrumentTemplateRemoved(qint64)), this, SLOT(populateInstrumentTemplates()));
	populateTable(-1);
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
				QStandardItem *chk = new QStandardItem(v->viewName());
				chk->setCheckable(true);
				chk->setCheckState(v->selected() ? Qt::Checked : Qt::Unchecked);
				chk->setData(v->id());
				if(v->id() != song->workingViewId() && v->id() != song->commentViewId())
				{
					chk->setIcon(m_icons.at(icon_index));
					++icon_index;
				}
				_autoTableModel->blockSignals(true);
				_autoTableModel->insertRow(_autoTableModel->rowCount(), chk);
				_autoTableModel->blockSignals(false);
				autoTable->setRowHeight(_autoTableModel->rowCount(), 25);
			}
		}
		updateTableHeader();
		tableView->resizeRowsToContents();
		autoTable->resizeRowsToContents();
	}
	if(flag & (SC_CONFIG)|| flag == -1)
		populateInstrumentTemplates();
}/*}}}*/

void TrackViewDock::populateInstrumentTemplates()
{
	//qDebug("TrackViewDock::populateInstrumentTemplates------------");
	_templateModel->clear();/*{{{*/
	ciTrackView iter = oom->instrumentTemplates()->constBegin();
	while(iter != oom->instrumentTemplates()->constEnd())
	{
		TrackView* tv = iter.value();
		if(tv)
		{
			QStandardItem *item = new QStandardItem(tv->viewName());
			item->setData(tv->id());
			item->setEditable(false);
			_templateModel->blockSignals(true);
			_templateModel->appendRow(item);
			_templateModel->blockSignals(false);
			templateView->setRowHeight(_templateModel->rowCount(), 25);
		}
		iter++;
	}/*}}}*/
	updateTableHeader();
	templateView->resizeRowsToContents();
}

void TrackViewDock::trackviewChanged(QStandardItem *item)/*{{{*/
{
	if(item)
	{
		qint64 id = item->data().toLongLong();
		//qDebug("TrackViewDock::trackviewChanged: id %lld", id);
		TrackView* tv = song->findTrackViewById(id);
		if(tv)
		{
			//qDebug("TrackViewDock::trackviewChanged: Found trackview %s", tv->viewName().toUtf8().constData());
			bool selected = (item->checkState() == Qt::Checked);/*{{{*/
			if(selected && tv->hasVirtualTracks())
			{
				QStringList list;
				QList<qint64> vlist = tv->virtualTracks();
				QList<VirtualTrack*> vtList;
				for(int i = 0; i < vlist.size(); ++i)
				{
					VirtualTrack* vt = trackManager->virtualTracks().value(vlist[i]);
					if(vt)
					{
						list.append(vt->name);
						vtList.append(vt);
					}
				}
				QString msg(tr("The View you selected contains Virtual Tracks.\n%1 \nWould you like to create them as real tracks now?"));
				if(QMessageBox::question(this, 
					tr("Create Virtual Tracks"),
					msg.arg(list.join("\n")),
					QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
				{
					foreach(VirtualTrack* vt, vtList)
					{
						if(vt)
						{//Let create the real track and add a part to it
							qint64 id = trackManager->addTrack(vt, -1);
							if(id)
							{//Copy any settings for that track, delete the virtual track and add the real track to the view
								TrackView::TrackViewTrack *tvt = tv->tracks()->value(vt->id);
								TrackView::TrackViewTrack *ntvl = new TrackView::TrackViewTrack(id);
								if(tvt)
									ntvl->setSettingsCopy(tvt->settings);
								tv->addTrack(ntvl);
								Track* track = song->findTrackByIdAndType(id, vt->type);
								if(track)
								{
									if(tvt)
										tv->removeTrack(tvt);
								}
							}
						}
					}
				}
			}/*}}}*/
			tv->setSelected(selected);
			song->updateTrackViews();
		}
	}
}/*}}}*/

void TrackViewDock::autoTrackviewChanged(QStandardItem *item)/*{{{*/
{
	if(item)
	{
		TrackViewList* tvlist = song->autoviews();
		TrackView* tv = tvlist->value(item->data().toLongLong());//song->findAutoTrackView(tname->text());
		if(tv)
		{
			//qDebug("TrackViewDock::autoTrackviewChanged found track view: %s", item->text().toUtf8().constData());
			tv->setSelected(item->checkState() == Qt::Checked);
			song->updateTrackViews();
		}
	}
}/*}}}*/

void TrackViewDock::updateTrackView(int table, QStandardItem *item)/*{{{*/
{
	if(item)
	{
		TrackView* tv;
		if(table)
			tv = song->findAutoTrackView(item->text());
		else
			tv = song->findTrackViewById(item->data().toLongLong());
		if(tv)
		{
			tv->setSelected((item->checkState() == Qt::Checked) ? true : false);
			//TODO: If checked and it has virtual track prompt the user to create them before calling updateTrackViews
			song->updateTrackViews();
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
	TrackViewEditor* tve = new TrackViewEditor(this, m_tabWidget->currentIndex());
	tve->show();
}

void TrackViewDock::currentTabChanged(int index)/*{{{*/
{
	switch(index)
	{
		case 0:
		{
			btnUp->setEnabled(true);
			btnDown->setEnabled(true);
			chkViewAll->setEnabled(true);
			chkViewAll->setToolTip(tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\" font-family:'Luxi Serif'; font-size:10pt; font-weight:400; font-style:normal;\">\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">View <span style=\" font-weight:600;\">all</span> available tracks in TrackView context menu.</p>\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Leave unchecked to view only the tracks that contain <span style=\" font-weight:600;\">no parts</span></p></body></html>"));
			btnTV->setToolTip(tr("Create and Manage Views"));
			btnDelete->setToolTip(tr("Delete Views"));
		}
		break;
		case 1:
		{
			btnUp->setEnabled(false);
			btnDown->setEnabled(false);
			chkViewAll->setEnabled(false);
			chkViewAll->setToolTip("");
			btnTV->setToolTip(tr("Create and Manage Instrument Templates"));
			btnDelete->setToolTip(tr("Delete Instrument Templates"));
		}
		break;
	}
}/*}}}*/

void TrackViewDock::contextPopupMenu(QPoint pos)/*{{{*/
{
	QModelIndex index = tableView->indexAt(pos);
	if(!index.isValid())
		return;
	QStandardItem* item = _tableModel->itemFromIndex(index);
	if(item)
	{
		TrackView *tv = song->findTrackViewById(item->data().toLongLong());
		if(tv)
		{
			QMenu* p = new QMenu(this);
			TrackViewMenu *itemMenu = new TrackViewMenu(p, tv, (chkViewAll->checkState() == Qt::Checked));
			p->addAction(itemMenu);

			QAction* act = p->exec(QCursor::pos());
			if (act)
			{
				qint64 tid = act->data().toLongLong();
				TrackView::TrackViewTrack *tvt = tv->tracks()->value(tid);
				Track* track = song->findTrackById(tid);
				if(track)
				{
					if(track->type() == Track::WAVE)
						oom->importWave(track);
					else
						oom->composer->addCanvasPart(track);
					song->updateTrackViews();
				}
				else
				{//This must be a virtual track try and find it
					VirtualTrack* vt = trackManager->virtualTracks().value(tid);
					if(vt)
					{//Let create the real track and add a part to it
						qint64 id = trackManager->addTrack(vt, -1);
						if(id)
						{//Copy any settings for that track, delete the virtual track and add the real track to the view
							TrackView::TrackViewTrack *ntvl = new TrackView::TrackViewTrack(id);
							ntvl->setSettingsCopy(tvt->settings);
							tv->addTrack(ntvl);
							Track* track = song->findTrackByIdAndType(id, vt->type);
							if(track)
							{
								if(track->type() == Track::WAVE)
									oom->importWave(track);
								else
									oom->composer->addCanvasPart(track);
								song->updateTrackViews();
							}
							tv->removeTrack(tvt);
						}
					}
				}
			}
			delete p;
		}
	}
}/*}}}*/

void TrackViewDock::templateContextPopupMenu(QPoint pos)/*{{{*/
{
	//qDebug("TrackViewDock::templateContextPopupMenu");
	QModelIndex index = templateView->indexAt(pos);
	if(!index.isValid())
		return;
	QStandardItem* item = _templateModel->itemFromIndex(index);
	if(item)
	{
		TrackView *tview = oom->findInstrumentTemplateById(item->data().toLongLong());
		if(tview)
		{
			QMenu* p = new QMenu(this);
			QAction* add = p->addAction(tr("Add Template to Song"));
			add->setData(0);
			QAction* addEnable = p->addAction(tr("Add Template to Song selected"));
			addEnable->setData(1);

			QAction* act = p->exec(QCursor::pos());
			if (act)
			{
				int mid = act->data().toInt();
				switch(mid)
				{
					case 0:
					{
						//qDebug("TrackViewDock::templateContextPopupMenu~~~~~~~~~~~~~~~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
						TrackView* tv = new TrackView(false);
						QList<qint64> *list = tview->trackIndexList();
						for(int i = 0; i < list->size(); ++i)
						{
							qint64 id = list->at(i);
							TrackView::TrackViewTrack *tvt = tview->tracks()->value(id);
							TrackView::TrackViewTrack *newtvt = new TrackView::TrackViewTrack();
							if(tvt)
							{
								newtvt->setSettingsCopy(tvt->settings);
								newtvt->is_virtual = tvt->is_virtual;
								newtvt->id = tvt->id;
								tv->addTrack(newtvt);
							}
						}

						tv->setComment(tview->comment());
						tv->setViewName(tv->getValidName(tview->viewName()));
						tv->setRecord(tview->record());
						tv->setSelected(false);
						song->insertTrackView(tv, -1);
						song->updateTrackViews();
						populateTable(-1);
						m_tabWidget->setCurrentIndex(0);
					}
					break;
					case 1:
					{
						TrackView* tv = new TrackView(false);
						QList<qint64> *list = tview->trackIndexList();
						QStringList namelist;
						QList<VirtualTrack*> vtList;
						for(int i = 0; i < list->size(); ++i)
						{
							qint64 id = list->at(i);
							TrackView::TrackViewTrack *tvt = tview->tracks()->value(id);
							TrackView::TrackViewTrack *newtvt = new TrackView::TrackViewTrack();
							if(tvt)
							{
								VirtualTrack* vt = trackManager->virtualTracks().value(tvt->id);
								if(vt)
								{
									namelist.append(vt->name);
									vtList.append(vt);
								}
								newtvt->setSettingsCopy(tvt->settings);
								newtvt->is_virtual = tvt->is_virtual;
								newtvt->id = tvt->id;
								tv->addTrack(newtvt);
							}
						}

						tv->setComment(tview->comment());
						tv->setViewName(tv->getValidName(tview->viewName()));
						tv->setRecord(tview->record());
						QString msg(tr("The View you selected contains Virtual Tracks.\n%1 \nWould you like to create them as real tracks now?"));/*{{{*/
						if(QMessageBox::question(this, 
							tr("Create Virtual Tracks"),
							msg.arg(namelist.join("\n")),
							QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
						{
							foreach(VirtualTrack* vt, vtList)
							{
								if(vt)
								{//Let create the real track and add a part to it
									qint64 id = trackManager->addTrack(vt, -1);
									if(id)
									{//Copy any settings for that track, delete the virtual track and add the real track to the view
										TrackView::TrackViewTrack *tvt = tview->tracks()->value(vt->id);
										TrackView::TrackViewTrack *ntvl = new TrackView::TrackViewTrack(id);
										ntvl->setSettingsCopy(tvt->settings);
										tv->addTrack(ntvl);
										Track* track = song->findTrackByIdAndType(id, vt->type);
										if(track)
										{
											tv->removeTrack(tvt);
										}
									}
								}
							}
							song->updateTrackViews();
						}/*}}}*/
						tv->setSelected(true);
						song->insertTrackView(tv, -1);
						populateTable(-1);
						m_tabWidget->setCurrentIndex(0);
					}
					break;
				}
			}
			delete p;
		}
	}
}/*}}}*/

void TrackViewDock::btnUpClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows(0);
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
		tableView->selectRow(row);
	}
}/*}}}*/

void TrackViewDock::btnDownClicked(bool)/*{{{*/
{
	QList<int> rows = getSelectedRows(0);
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
			TrackView* tv = song->findTrackViewById(txt->data().toLongLong());
			if(tv)
			{
				song->removeTrackView(tv->id());
				song->insertTrackView(tv, row);
			}
		}
		tableView->selectRow(row);
	}
}/*}}}*/

void TrackViewDock::btnDeleteClicked(bool)/*{{{*/
{
	int tab = m_tabWidget->currentIndex();
	QList<int> rows = getSelectedRows(tab);
	QList<TrackView*> dlist;
	QStringList list;
	if (!rows.isEmpty())
	{
		switch(tab)
		{
			case 0:
			{
				foreach (int r, rows)
				{
					QStandardItem *item = _tableModel->item(r);
					if(item)
					{
						TrackView* tv = song->findTrackViewById(item->data().toLongLong());
						if(tv)
						{
							dlist.append(tv);
							list.append(tv->viewName());
						}
					}
				}
				if (!dlist.isEmpty())
				{
					QString msg(tr("You are about to delete the following View(s) \n%1 \nAre you sure this is what you want?"));
					if(QMessageBox::question(this, 
						tr("Delete View(s)"),
						msg.arg(list.join("\n")),
						QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
					{
						for (int d = 0; d < dlist.size(); ++d)
						{
							song->removeTrackView(dlist.at(d)->id());
						}
					}
				}
			}
			break;
			case 1:
			{
				foreach (int r, rows)
				{
					QStandardItem *item = _templateModel->item(r);
					if(item)
					{
						TrackView* tv = oom->findInstrumentTemplateById(item->data().toLongLong());
						if(tv)
						{
							dlist.append(tv);
							list.append(tv->viewName());
						}
					}
				}
				if (!dlist.isEmpty())
				{
					QString msg(tr("You are about to delete the follwing Instrument Template(s)\n%1 \nAre you sure this is what you want?"));
					if(QMessageBox::question(this, 
						tr("Delete Instrument Template(s)"),
						msg.arg(list.join("\n")),
						QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok)
					{
						for (int d = 0; d < dlist.size(); ++d)
						{
							oom->removeInstrumentTemplate(dlist.at(d)->id());
						}
					}
					oom->changeConfig(true);
				}
			}
			break;
		}
	}
}/*}}}*/

QList<int> TrackViewDock::getSelectedRows(int tab)/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = 0;
	if(tab)
	{
		 smodel = templateView->selectionModel();
	}
	else
	{
		 smodel = tableView->selectionModel();
	}
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

void TrackViewDock::selectStaticView(int n)/*{{{*/
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
}/*}}}*/
