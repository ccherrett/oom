//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "mixerview.h"
#include "globals.h"
#include "song.h"


MixerView::MixerView(QWidget* parent)
: TrackViewDock(parent)
{
	//m_tracklist = song->visibletracks();
	toggleButtons(false);
	populateTable(-1, true);
	//connect(song, SIGNAL(songChanged(int)), this, SLOT(populateTable(int)));
}

MixerView::~MixerView()
{
}

void MixerView::addButton(QWidget* btn)
{
	horizontalLayout->insertWidget(0, btn);
}

void MixerView::populateTable(int flag, bool startup)/*{{{*/
{
	if(flag & (SC_VIEW_DELETED | SC_VIEW_ADDED) || flag == -1)
	{
		//m_selectList.clear();
		TrackViewList* tviews = song->trackviews();
		_tableModel->clear();
		for(iTrackView it = tviews->begin(); it != tviews->end(); ++it)
		{
			QList<QStandardItem*> rowData;
			QStandardItem *chk = new QStandardItem(true);
			chk->setCheckable(true);
			if(startup)
			{
				chk->setCheckState((*it)->selected() ? Qt::Checked : Qt::Unchecked);
				if((*it)->selected())
					m_selectList.append((*it)->viewName());
			}
			else
			{
				chk->setCheckState(m_selectList.contains((*it)->viewName()) ? Qt::Checked : Qt::Unchecked);
			}
			QStandardItem *tname = new QStandardItem((*it)->viewName());
			rowData.append(chk);
			rowData.append(tname);
			_tableModel->blockSignals(true);
			_tableModel->insertRow(_tableModel->rowCount(), rowData);
			_tableModel->blockSignals(false);
			tableView->setRowHeight(_tableModel->rowCount(), 25);
		}
		_autoTableModel->clear();
		int index = 0;
		QList<int> list;
		list << Track::MIDI << Track::AUDIO_INPUT << Track::AUDIO_OUTPUT << Track::AUDIO_BUSS << Track::AUDIO_AUX << Track::WAVE;
		for(iTrackView ait = song->autoviews()->begin(); ait != song->autoviews()->end(); ++index,++ait)
		{
			QList<QStandardItem*> rowData2;
			QStandardItem *chk = new QStandardItem(true);
			chk->setCheckable(true);
			if(startup)
			{
				chk->setCheckState((*ait)->selected() ? Qt::Checked : Qt::Unchecked);
				if((*ait)->selected())
					m_selectList.append((*ait)->viewName());
			}
			else
			{
				chk->setCheckState(m_selectList.contains((*ait)->viewName()) ? Qt::Checked : Qt::Unchecked);
			}
			//chk->setCheckState((*ait)->selected() ? Qt::Checked : Qt::Unchecked);
			QStandardItem *tname = new QStandardItem((*ait)->viewName());
			if((*ait)->viewName() != "Working View" && (*ait)->viewName() != "Comment View")
			{
				chk->setForeground(QBrush(QColor(g_trackColorListSelected.value(list.at(index)))));
				tname->setForeground(QBrush(QColor(g_trackColorListSelected.value(list.at(index)))));
			}
			rowData2.append(chk);
			rowData2.append(tname);
			_autoTableModel->blockSignals(true);
			_autoTableModel->insertRow(_autoTableModel->rowCount(), rowData2);
			_autoTableModel->blockSignals(false);
			autoTable->setRowHeight(_autoTableModel->rowCount(), 25);
		}
		updateTrackList();
		updateTableHeader();
		tableView->resizeRowsToContents();
		autoTable->resizeRowsToContents();
	}
}/*}}}*/

void MixerView::trackviewChanged(QStandardItem *item)/*{{{*/
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
				if(chk->checkState() == Qt::Checked)
				{
					if(!m_selectList.contains(tv->viewName()))
						m_selectList.append(tv->viewName());
				}
				else
				{
					if(m_selectList.contains(tv->viewName()))
						m_selectList.removeAt(m_selectList.indexOf(tv->viewName()));
				}
				updateTrackList();
			}
		}
	}
}/*}}}*/

void MixerView::autoTrackviewChanged(QStandardItem *item)/*{{{*/
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
				//printf("MixerView::autoTrackviewChanged: %s\n", tname->text().toLatin1().constData());
				if(chk->checkState() == Qt::Checked)
				{
					if(!m_selectList.contains(tv->viewName()))
						m_selectList.append(tv->viewName());
				}
				else
				{
					if(m_selectList.contains(tv->viewName()))
						m_selectList.removeAt(m_selectList.indexOf(tv->viewName()));
				}
				updateTrackList();
			}
		}
	}
}/*}}}*/

void MixerView::updateTrackList()/*{{{*/
{
	//printf("Song::updateTrackViews1()\n");
	m_tracklist.clear();
	//m_tracklist = new TrackList();
	bool viewselected = false;
	bool customview = false;
	bool workview = false;
	bool commentview = false;
	
	QStandardItem *witem = _autoTableModel->item(0);
	QStandardItem *citem = _autoTableModel->item(5);
	//TrackView* wv = song->findAutoTrackView("Working View");
	//TrackView* cv = song->findAutoTrackView("Comment View");
	//if(wv && wv->selected())
	if(witem && witem->checkState() == Qt::Checked)
	{
		workview = true;
		viewselected = true;
	}
	//if(cv && cv->selected())
	if(citem && citem->checkState() == Qt::Checked)
	{
		commentview = true;
		viewselected = true;
	}
	for(iTrackView it = song->trackviews()->begin(); it != song->trackviews()->end(); ++it)
	{
		if(m_selectList.contains((*it)->viewName()))
		{
			TrackList* tl = (*it)->tracks();
			for(ciTrack t = tl->begin(); t != tl->end(); ++t)
			{
				bool found = false;
				if(workview && (*t)->parts()->empty()) {
					continue;
				}
				for (ciTrack i = m_tracklist.begin(); i != m_tracklist.end(); ++i)
				{
					if ((*i)->name() == (*t)->name())
					{
						found = true;
						break;
					}
				}
				if(!found && (*t)->name() != "Master")
				{
					m_tracklist.push_back((*t));
					customview = true;
					viewselected = true;
				}
			}
		}
	}
	for(iTrackView ait = song->autoviews()->begin(); ait != song->autoviews()->end(); ++ait)
	{
		if(customview && (*ait)->viewName() == "Working View")
			continue;
		if(m_selectList.contains((*ait)->viewName()))/*{{{*/
		{
			TrackList* tl = song->tracks();
			for(ciTrack t = tl->begin(); t != tl->end(); ++t)
			{
				bool found = false;
				for (ciTrack i = m_tracklist.begin(); i != m_tracklist.end(); ++i)
				{
					if ((*i)->name() == (*t)->name())
					{
						found = true;
						break;
					}
				}
				if(!found)
				{
					viewselected = true;
					switch((*t)->type())/*{{{*/
					{
						case Track::MIDI:
						case Track::DRUM:
						case Track::AUDIO_SOFTSYNTH:
						case Track::WAVE:
							if((*ait)->viewName() == "Working View")
							{
								if((*t)->parts()->empty())
									break;
								m_tracklist.push_back((*t));
							}
							else if((*ait)->viewName() == "Comment View")
							{
								if((*t)->comment().isEmpty())
									break;
								m_tracklist.push_back((*t));
							}
							break;
						case Track::AUDIO_OUTPUT:
							if((*ait)->viewName() == "Outputs View" && (*t)->name() != "Master")
							{
								m_tracklist.push_back((*t));
							}
							else if((*ait)->viewName() == "Comment View" && (*t)->name() != "Master")
							{
								if((*t)->comment().isEmpty())
									break;
								m_tracklist.push_back((*t));
							}
							break;
						case Track::AUDIO_BUSS:
							if((*ait)->viewName() == "Buss View")
							{
								m_tracklist.push_back((*t));
							}
							else if((*ait)->viewName() == "Comment View")
							{
								if((*t)->comment().isEmpty())
									break;
								m_tracklist.push_back((*t));
							}
							break;
						case Track::AUDIO_AUX:
							if((*ait)->viewName() == "Aux View")
							{
								m_tracklist.push_back((*t));
							}
							else if((*ait)->viewName() == "Comment View")
							{
								if((*t)->comment().isEmpty())
									break;
								m_tracklist.push_back((*t));
							}
							break;
						case Track::AUDIO_INPUT:
							if((*ait)->viewName() == "Inputs  View")
							{
								m_tracklist.push_back((*t));
							}
							else if((*ait)->viewName() == "Comment View")
							{
								if((*t)->comment().isEmpty())
									break;
								m_tracklist.push_back((*t));
							}
							break;
						default:
							fprintf(stderr, "unknown track type %d\n", (*t)->type());
							return;
					}/*}}}*/
				}
			}
		}/*}}}*/
	}
	if(!viewselected)
	{
		//Make the viewtracks the artracks
		for(ciTrack it = song->artracks()->begin(); it != song->artracks()->end(); ++it)
		{
			if((*it)->name() != "Master")
			m_tracklist.push_back((*it));
		}
	}
	emit trackListChanged(&m_tracklist);
}/*}}}*/
