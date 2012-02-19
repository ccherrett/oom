//===========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  (C) Copyright 2011 Andrew Williams & Christopher Cherrett
//===========================================================

#include <QtGui>
#include "tracklistview.h"
#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "song.h"
#include "audio.h"
#include "track.h"
#include "part.h"
#include "AbstractMidiEditor.h"
#include "citem.h"
#include "Composer.h"
#include "event.h"
#include "traverso_shared/TConfig.h"
#include "icons.h"

TrackListView::TrackListView(AbstractMidiEditor* editor, QWidget* parent)
: QFrame(parent)
{
	m_editor = editor;
	m_displayRole = PartRole;
	m_selectedIndex = -1;
	m_editing = false;
	scrollPos = QPoint(1, 1);
	m_headers << "Track List";
	m_layout = new QVBoxLayout(this);
	m_layout->setContentsMargins(8, 2, 8, 2);
	m_model = new QStandardItemModel(this);
	m_selmodel = new QItemSelectionModel(m_model);
	m_table = new QTableView(this);
	m_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_table->setObjectName("TrackListView");
	m_table->setModel(m_model);
	m_table->setSelectionModel(m_selmodel);
	m_table->setAlternatingRowColors(false);
	m_table->setShowGrid(true);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::DoubleClicked);
	//m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setCornerButtonEnabled(false);
	m_table->horizontalHeader()->setStretchLastSection(true);
	m_table->verticalHeader()->hide();
	m_layout->addWidget(m_table);

	m_buttonBox = new QHBoxLayout;
	m_chkWorkingView = new QCheckBox(tr("Working View"), this);
	m_chkWorkingView->setToolTip(tr("Toggle Working View. Show only tracks with parts in them"));
	m_chkWorkingView->setChecked(true);

	m_chkSnapToPart = new QCheckBox(tr("Snap To Part"), this);
	m_chkSnapToPart->setToolTip(tr("Move Playback cursor to the first note in a part when changing parts."));
	bool snap = tconfig().get_property("PerformerEdit", "snaptopart", true).toBool();
	m_chkSnapToPart->setChecked(snap);

	m_btnRefresh = new QToolButton(this);
	m_btnRefresh->setAutoRaise(true);
	m_btnRefresh->setIcon(*refreshIconSet3);
	m_btnRefresh->setIconSize(QSize(25, 25));
	m_btnRefresh->setFixedSize(QSize(25, 25));

	m_buttonBox->addWidget(m_chkWorkingView);
	QSpacerItem* hSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	m_buttonBox->addItem(hSpacer);
	m_buttonBox->addWidget(m_chkSnapToPart);
	m_buttonBox->addWidget(m_btnRefresh);

	m_layout->addLayout(m_buttonBox);

	//populateTable();
	connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(toggleTrackPart(QStandardItem*)));
	connect(m_selmodel, SIGNAL(currentRowChanged(const QModelIndex, const QModelIndex)), this, SLOT(selectionChanged(const QModelIndex, const QModelIndex)));
	connect(m_chkWorkingView, SIGNAL(stateChanged(int)), this, SLOT(displayRoleChanged(int)));
	connect(m_chkSnapToPart, SIGNAL(stateChanged(int)), this, SLOT(snapToPartChanged(int)));
	connect(m_table, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextPopupMenu(QPoint)));
	connect(m_btnRefresh, SIGNAL(clicked()), this, SLOT(populateTable()));
}

TrackListView::~TrackListView()
{
	tconfig().set_property("PerformerEdit", "snaptopart", m_chkSnapToPart->isChecked());
	tconfig().save();
}

void TrackListView::showEvent(QShowEvent*)
{
	populateTable();
}

void TrackListView::snapToPartChanged(int state)/*{{{*/
{
	tconfig().set_property("PerformerEdit", "snaptopart", state);
	tconfig().save();
}/*}}}*/

void TrackListView::displayRoleChanged(int role)/*{{{*/
{
	switch(role)
	{
		case Qt::Checked:
			m_displayRole = PartRole;
		break;
		case Qt::Unchecked:
			m_displayRole = TrackRole;
		break;
	}
	populateTable();
}/*}}}*/

void TrackListView::songChanged(int flags)/*{{{*/
{
	if(flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_PART_INSERTED | SC_PART_REMOVED | SC_PART_MODIFIED/*| SC_PART_COLOR_MODIFIED*/))
	{
		if(debugMsg)
			printf("TrackListView::songChanged\n");
		if(!m_editing)
			populateTable();
	}
}/*}}}*/

void TrackListView::populateTable()/*{{{*/
{
	if(debugMsg)
		printf("TrackListView::populateTable\n");
	QScrollBar *bar = m_table->verticalScrollBar();
	int barPos = 0;
	if(bar)
		barPos = bar->sliderPosition();

	m_model->clear();
	for(iTrack i = song->artracks()->begin(); i != song->artracks()->end(); ++i)
	{
		if(!(*i)->isMidiTrack())
			continue;
		MidiTrack* track = (MidiTrack*)(*i);
		PartList* pl = track->parts();
		if(m_displayRole == PartRole && pl->empty())
		{
			continue;
		}

		QStandardItem* trackName = new QStandardItem();
		trackName->setForeground(QBrush(QColor(205,209,205)));
		trackName->setBackground(QBrush(QColor(20,20,20)));
		trackName->setFont(QFont("fixed-width", 10, QFont::Bold));
		trackName->setText(track->name());
		trackName->setCheckable(true);
		trackName->setCheckState(m_selectedTracks.contains(track->id()) ? Qt::Checked : Qt::Unchecked);
		
		trackName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		trackName->setData(1, TrackRole);
		trackName->setData(track->name(), TrackNameRole);
		trackName->setData(track->id(), TrackIdRole);
		trackName->setEditable(true);
		m_model->appendRow(trackName);

		for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			Part* part = ip->second;
			QStandardItem* partName = new QStandardItem();
			partName->setFont(QFont("fixed-width", 9, QFont::Bold));
			partName->setText(part->name());
			partName->setData(part->sn(), PartRole);
			partName->setData(2, TrackRole);
			partName->setData(track->name(), TrackNameRole);
			partName->setData(part->name(), PartNameRole);
			partName->setData(part->tick(), TickRole);
			partName->setData(track->id(), TrackIdRole);
			partName->setEditable(true);
			partName->setCheckable(true);
			partName->setCheckState(m_editor->hasPart(part->sn()) ? Qt::Checked : Qt::Unchecked);

			if(!partColorIcons.isEmpty() && part->colorIndex() < partColorIcons.size())
				partName->setIcon(partColorIcons.at(part->colorIndex()));
			
			m_model->appendRow(partName);
		}
	}
	m_model->setHorizontalHeaderLabels(m_headers);
	if(m_selectedIndex < m_model->rowCount())
	{
		m_table->selectRow(m_selectedIndex);
		m_table->scrollTo(m_model->index(m_selectedIndex, 0));
	}
	if(bar)
		bar->setSliderPosition(barPos);
}/*}}}*/

void TrackListView::updateColor()
{
	while(!m_colorRows.isEmpty())
	{
		int row = m_colorRows.takeFirst();
		QStandardItem *item = m_model->item(row);
		if(item)
		{
			//qDebug("TrackListView::updateColor: row: %d", row);
			m_model->blockSignals(true);
			item->setForeground(QColor(187, 191, 187));
			m_model->blockSignals(false);
		}
	}
	update();
}

void TrackListView::contextPopupMenu(QPoint pos)/*{{{*/
{
	QModelIndex mindex = m_table->indexAt(pos);
	if(!mindex.isValid())
		return;
	//int row = mindex.row();
	QStandardItem* item = m_model->itemFromIndex(mindex);
	if(item)
	{
		m_selectedIndex = item->row();
		//Make it works even if you rightclick on the checkbox
		QString trackName = item->data(TrackNameRole).toString();
		int type = item->data(TrackRole).toInt();
		Track* track = song->findTrack(trackName);
		if(!track || !m_editor)
			return;
		QMenu* p = new QMenu(this);
		QString title(tr("Part Color"));
		int index = track->getDefaultPartColor();
		Part* npart = 0;
		if(type == 1)
			title = QString(tr("Default Part Color"));
		else
		{
			PartList* list = track->parts();
			int sn = item->data(PartRole).toInt();
			unsigned tick = item->data(TickRole).toInt();
			npart = list->find(tick, sn);
			if(npart)
				index = npart->colorIndex();
		}
		QMenu* colorPopup = p->addMenu(title);
		
		QMenu* colorSub; 
		for (int i = 0; i < NUM_PARTCOLORS; ++i)
		{
			QString colorname(config.partColorNames[i]);
			if(colorname.contains("menu:", Qt::CaseSensitive))
			{
				colorSub = colorPopup->addMenu(colorname.replace("menu:", ""));
			}
			else
			{
				if(index == i)
				{
					colorname = QString(config.partColorNames[i]);
					colorPopup->setIcon(partColorIconsSelected.at(i));
					colorPopup->setTitle(colorSub->title()+": "+colorname);
		
					colorname = QString("* "+config.partColorNames[i]);
					QAction *act_color = colorSub->addAction(partColorIconsSelected.at(i), colorname);
					act_color->setData(20 + i);
				}
				else
				{
					colorname = QString("     "+config.partColorNames[i]);
					QAction *act_color = colorSub->addAction(partColorIcons.at(i), colorname);
					act_color->setData(20 + i);
				}
			}	
		}
		p->addAction(tr("Add Part"))->setData(1);
		p->addAction(tr("Add Part and Select"))->setData(2);
		if(type == 2)
			p->addAction(tr("Delete Part"))->setData(3);

		QAction* act = p->exec(QCursor::pos());
		if (act)
		{
			int selection = act->data().toInt();
			switch(selection)
			{
				case 1:
				{
					CItem *citem = oom->composer->addCanvasPart(track);
					if(citem)
					{
						Part* mp = citem->part();
						populateTable();//update check state
						//Select and scroll to the added part
						if(mp)
						{
							int psn = mp->sn();
							for(int i = 0; i < m_model->rowCount(); ++i)
							{
								QStandardItem* item = m_model->item(i, 0);
								if(item)
								{
									int type = item->data(TrackRole).toInt();
									if(type == 1)
									{//TrackMode
										continue;
									}
									else
									{//PartMode
										int sn = item->data(PartRole).toInt();
										if(psn == sn)
										{
											//m_tempColor = item->foreground();
											m_model->blockSignals(true);
											item->setForeground(QColor(99, 36, 36));
											m_model->blockSignals(false);
											update();
											m_selectedIndex = item->row();
											m_table->selectRow(m_selectedIndex);
											m_table->scrollTo(m_model->index(m_selectedIndex, 0));
											m_colorRows.append(m_selectedIndex);
											QTimer::singleShot(350, this, SLOT(updateColor()));
											break;
										}
									}
								}
							}
						}
					}
				}
				break;
				case 2:
				{
					CItem* citem = oom->composer->addCanvasPart(track);
					if(citem)
					{
						Part* mp = citem->part();
						if(mp)
						{
							m_editor->addPart(mp);
							populateTable();//update check state
							updatePartSelection(mp);
						}
					}
					break;
				}
				case 3:
				{
					if(npart)
					{
						audio->msgRemovePart(npart);
						populateTable();//update check state
						scrollPos = pos;
						/*if(row < m_model->rowCount())
						{
							QModelIndex rowIndex = m_table->indexAt(pos);
							m_table->scrollTo(rowIndex, QAbstractItemView::PositionAtTop);
						}*/
					}
					break;
				}
				case 20 ... NUM_PARTCOLORS + 20:
				{
					int curColorIndex = selection - 20;
					if(npart)
					{
						npart->setColorIndex(curColorIndex);
						song->update(SC_PART_COLOR_MODIFIED);
					}
					else
					{
						track->setDefaultPartColor(curColorIndex);
					}
					populateTable();//update check state
					break;
				}
			}
		}
		delete p;
	}
}/*}}}*/

void TrackListView::selectionChanged(const QModelIndex current, const QModelIndex)/*{{{*/
{
	if(!current.isValid())
		return;
	int row = current.row();
	m_selectedIndex = row;
	QStandardItem* item = m_model->item(row);
	if(item)
	{
		int type = item->data(TrackRole).toInt();
		qint64 tid = item->data(TrackIdRole).toLongLong();
		bool checked = (item->checkState() == Qt::Checked);
		//QString trackName = item->data(TrackNameRole).toString();
		Track* track = song->findTrackByIdAndType(tid, Track::MIDI);
		if(!track || !m_editor || type == 1 || !checked)
			return;

		PartList* list = track->parts();
		int sn = item->data(PartRole).toInt();
		unsigned tick = item->data(TickRole).toInt();
		Part* part = list->find(tick, sn);
		updatePartSelection(part);
	}
}/*}}}*/

void TrackListView::updatePartSelection(Part* part)/*{{{*/
{
	if(part)
	{
		Track* track = part->track();
		Part* curPart = m_editor->curCanvasPart();
		if (curPart)
		{
			song->setRecordFlag(curPart->track(), false);
		}
		m_editor->setCurCanvasPart(part);
		Song::movePlaybackToPart(part);
		// and turn it on for the new parts track
		song->setRecordFlag(track, true);
		song->deselectTracks();
		song->deselectAllParts();
		track->setSelected(true);
		part->setSelected(true);
		song->update(SC_SELECTION);

		int psn = part->sn();
		for(int i = 0; i < m_model->rowCount(); ++i)
		{
			QStandardItem* item = m_model->item(i, 0);
			if(item)
			{
				int type = item->data(TrackRole).toInt();
				if(type == 1)
				{//TrackMode
					continue;
				}
				else
				{//PartMode
					int sn = item->data(PartRole).toInt();
					if(psn == sn)
					{
						m_selectedIndex = item->row();

						m_tempColor = item->foreground();
						
						m_model->blockSignals(true);
						item->setForeground(QColor(99, 36, 36));
						m_model->blockSignals(false);
						update();
						m_table->selectRow(m_selectedIndex);
						m_table->scrollTo(m_model->index(m_selectedIndex, 0));
						
						m_colorRows.append(m_selectedIndex);
						
						QTimer::singleShot(350, this, SLOT(updateColor()));
						break;
					}
				}
			}
		}
	}
}/*}}}*/

void TrackListView::updateCheck(PartList* list, bool on)/*{{{*/
{
	for(iPart i = list->begin(); i != list->end(); ++i)
	{
		Part* part = i->second;
		//QList<QStandardItem*> found = m_model->findItems(part->name(), Qt::MatchExactly, 1);
		for(int r = 0; r < m_model->rowCount(); ++r)
		{
			QStandardItem* item = m_model->item(r);
			if(item)
			{
				int type = item->data(TrackRole).toInt();
				if(type == 1)
					continue;
				int sn = item->data(PartRole).toInt();
				int psn = part->sn();
				//printf("Serial no. item: %d, part: %d\n", sn, psn);
				if(sn == psn)
				{
					//printf("Updating item\n");
					m_model->blockSignals(true);
					item->setCheckState(on ? Qt::Checked : Qt::Unchecked);
					m_model->blockSignals(false);
					break;
				}
			}
		}
	}
	m_table->update();
}/*}}}*/

void TrackListView::updateCheck()/*{{{*/
{
	for(int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem* item = m_model->item(i, 0);
		if(item)
		{
			int type = item->data(TrackRole).toInt();
			qint64 tid = item->data(TrackIdRole).toLongLong();
			QString trackName = item->data(TrackNameRole).toString();
			if(type == 1)
			{//TrackMode
				m_model->blockSignals(true);
				if(m_selectedTracks.contains(tid))
					item->setCheckState(Qt::Checked);
				else
					item->setCheckState(Qt::Unchecked);
				m_model->blockSignals(false);
			}
			else
			{//PartMode
				int sn = item->data(PartRole).toInt();
				//printf("Serial no. item: %d, part: %d\n", sn, psn);
				m_model->blockSignals(true);
				if(m_editor->hasPart(sn))
					item->setCheckState(Qt::Checked);
				else
					item->setCheckState(Qt::Unchecked);
				m_model->blockSignals(false);
			}
		}
	}
}/*}}}*/

void TrackListView::toggleTrackPart(QStandardItem* item)/*{{{*/
{
	if(!item)
		return;
	m_editing = true;
	int type = item->data(TrackRole).toInt();
	int column = item->column();
	bool checked = (item->checkState() == Qt::Checked);
	qint64 tid = item->data(TrackIdRole).toLongLong();
	QString trackName = item->data(TrackNameRole).toString();
	QString partName;
	m_selectedIndex = item->row();
	QString newName = item->text();
	if(type == 1)
	{
		if(trackName == newName)
			column = 0;
		else
			column = 1;
	}
	else
	{
		partName = item->data(PartNameRole).toString();
		if(partName == newName)
			column = 0;
		else
			column = 1;
	}

	Track* track = song->findTrackByIdAndType(tid, Track::MIDI);
	if(!track || !m_editor)
	{
		m_editing = false;
		return;
	}

	PartList* list = track->parts();
	if(list->empty())
	{
		m_editing = false;
		updateCheck();
		return;
	}
	switch(type)
	{
		case 1: //Track
		{
			if(!column)
			{
				if(checked)
				{
					m_editor->addParts(list);
					m_selectedTracks.append(track->id());
					if(!list->empty())
					{
						updatePartSelection(list->begin()->second);

						updateCheck(list, checked);
					}
				}
				else
				{
					m_editor->removeParts(list);
					m_editor->updateCanvas();
					m_selectedTracks.removeAll(track->id());
					updateCheck();
					song->update(SC_SELECTION);
				}
			}
			else
			{
				bool valid = true;
				if(newName.isEmpty())
				{
					valid = false;
				}
				if(valid)
				{
					for (iTrack i = song->tracks()->begin(); i != song->tracks()->end(); ++i)
					{
						if ((*i)->name() == newName)
						{
							valid = false;
							break;
						}
					}
				}
				if(!valid)
				{
					QMessageBox::critical(this, tr("OOMidi: bad trackname"),
							tr("please choose a unique track name"),
							QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
					m_model->blockSignals(true);
					item->setText(item->data(TrackNameRole).toString());
					m_model->blockSignals(false);
					update();
					m_editing = false;
					return;
				}
				m_model->blockSignals(true);
				item->setData(newName, TrackNameRole);
				m_model->blockSignals(false);

				Track* newTrack = track->clone(false);
				newTrack->setName(newName);
				track->setName(newName);
				audio->msgChangeTrack(newTrack, track);
			}
		}
		break;
		case 2: //Part
		{
			int sn = item->data(PartRole).toInt();
			unsigned tick = item->data(TickRole).toInt();
			Part* part = list->find(tick, sn);
			if(part)
			{
				if(!column)
				{
					if(checked)
					{		
						m_editor->addPart(part);
						updatePartSelection(part);
					}
					else
					{
						m_editor->removePart(sn);
						m_editor->updateCanvas();
						updateCheck();
						song->update(SC_SELECTION);
					}
				}
				else
				{
					if(partName.isEmpty())
					{
						QMessageBox::critical(this, tr("OOMidi: Invalid part name"),
								tr("Please choose a name with at least one charactor"),
								QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
						
						m_model->blockSignals(true);
						item->setText(item->data(PartNameRole).toString());
						m_model->blockSignals(false);
						update();
						m_editing = false;
						return;
					}

					m_model->blockSignals(true);
					item->setData(newName, PartNameRole);
					m_model->blockSignals(false);

					Part* newPart = part->clone();
					newPart->setName(newName);
					// Indicate do undo, and do port controller values but not clone parts.
					audio->msgChangePart(part, newPart, true, true, false);
					song->update(SC_PART_MODIFIED);
				}
			}
		}
		break;
	}
	update();
	if(m_selectedIndex < m_model->rowCount())
	{
		m_table->selectRow(m_selectedIndex);
		m_table->scrollTo(m_model->index(m_selectedIndex, 0));
	}
	m_editing = false;
}/*}}}*/

