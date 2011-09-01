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

TrackListView::TrackListView(AbstractMidiEditor* editor, QWidget* parent)
: QFrame(parent)
{
	m_editor = editor;
	m_displayRole = PartRole;
	m_selectedIndex = -1;
	scrollPos = QPoint(1, 1);
	m_headers << "V" << "Track List";
	m_layout = new QVBoxLayout(this);
	m_layout->setContentsMargins(8, 2, 8, 2);
	m_model = new QStandardItemModel(0, 2, this);
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

	m_buttonBox->addWidget(m_chkWorkingView);
	QSpacerItem* hSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	m_buttonBox->addItem(hSpacer);
	m_buttonBox->addWidget(m_chkSnapToPart);

	m_layout->addLayout(m_buttonBox);

	populateTable();
	connect(song, SIGNAL(songChanged(int)), this, SLOT(songChanged(int)));
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(toggleTrackPart(QStandardItem*)));
	connect(m_selmodel, SIGNAL(currentRowChanged(const QModelIndex, const QModelIndex)), this, SLOT(selectionChanged(const QModelIndex, const QModelIndex)));
	connect(m_chkWorkingView, SIGNAL(stateChanged(int)), this, SLOT(displayRoleChanged(int)));
	connect(m_chkSnapToPart, SIGNAL(stateChanged(int)), this, SLOT(snapToPartChanged(int)));
	connect(m_table, SIGNAL(customContextMenuRequested(QPoint)), SLOT(contextPopupMenu(QPoint)));
}

TrackListView::~TrackListView()
{
	tconfig().set_property("PerformerEdit", "snaptopart", m_chkSnapToPart->isChecked());
	tconfig().save();
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
	if(flags & (SC_TRACK_INSERTED | SC_TRACK_REMOVED | SC_TRACK_MODIFIED | SC_PART_INSERTED | SC_PART_REMOVED /*| SC_PART_COLOR_MODIFIED*/))
	{
		//if(debugMsg)
			printf("TrackListView::songChanged\n");
		populateTable();
	}
}/*}}}*/

void TrackListView::populateTable()/*{{{*/
{
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
		QList<QStandardItem*> trackRow;
		QStandardItem* chkTrack = new QStandardItem(true);
		chkTrack->setCheckable(true);
		chkTrack->setBackground(QBrush(QColor(20,20,20)));
		chkTrack->setData(1, TrackRole);
		chkTrack->setData(track->name(), TrackNameRole);
		if(m_selected.contains(track->name()))
			chkTrack->setCheckState(Qt::Checked);
		chkTrack->setEditable(false);
		trackRow.append(chkTrack);
		QStandardItem* trackName = new QStandardItem();
		trackName->setForeground(QBrush(QColor(205,209,205)));
		trackName->setBackground(QBrush(QColor(20,20,20)));
		trackName->setFont(QFont("fixed-width", 10, QFont::Bold));
		trackName->setText(track->name());
		//QFont font = trackName->font();
		trackName->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		trackName->setData(1, TrackRole);
		trackName->setData(track->name(), TrackNameRole);
		trackName->setEditable(true);
		trackRow.append(trackName);
		m_model->appendRow(trackRow);

		for(iPart ip = pl->begin(); ip != pl->end(); ++ip)
		{
			QList<QStandardItem*> partsRow;
			Part* part = ip->second;
			QStandardItem* chkPart = new QStandardItem(true);
			chkPart->setCheckable(true);
			chkPart->setData(part->sn(), PartRole);
			chkPart->setData(2, TrackRole);
			chkPart->setData(track->name(), TrackNameRole);
			chkPart->setData(part->tick(), TickRole);
			chkPart->setEditable(false);
			if(m_editor->hasPart(part->sn()))
			{
				chkPart->setCheckState(Qt::Checked);
			}
			QStandardItem* partName = new QStandardItem();

			partName->setFont(QFont("fixed-width", 9, QFont::Bold));
			//if(m_displayRole == TrackRole)
			partName->setText(part->name());
			partName->setData(part->sn(), PartRole);
			partName->setData(2, TrackRole);
			partName->setData(track->name(), TrackNameRole);
			partName->setData(part->tick(), TickRole);
			partName->setEditable(true);
			//else
			//	partName->setText(track->name()+" : "+part->name());

			if(!partColorIcons.isEmpty() && part->colorIndex() < partColorIcons.size())
				partName->setIcon(partColorIcons.at(part->colorIndex()));
			partsRow.append(chkPart);
			partsRow.append(partName);
			m_model->appendRow(partsRow);
		}
	}
	m_model->setHorizontalHeaderLabels(m_headers);
	m_table->setColumnWidth(0, 20);
	if(m_selectedIndex < m_model->rowCount())
	{
		m_table->selectRow(m_selectedIndex);
		QStandardItem* item = m_model->item(m_selectedIndex, 1);
		if(item)
		{
			//m_table->scrollTo(item->index(), QAbstractItemView::EnsureVisible);
			//m_table->scrollTo(item->index(), QAbstractItemView::PositionAtTop);
			//m_table->scrollTo(item->index(), QAbstractItemView::PositionAtBottom);
			m_table->scrollTo(item->index(), QAbstractItemView::PositionAtCenter);
		}
	}
	else
	{
		QModelIndex rowIndex = m_table->indexAt(scrollPos);
		m_table->scrollTo(rowIndex, QAbstractItemView::PositionAtTop);
	}
}/*}}}*/

void TrackListView::contextPopupMenu(QPoint pos)/*{{{*/
{
	QModelIndex mindex = m_table->indexAt(pos);
	if(!mindex.isValid())
		return;
	//int row = mindex.row();
	QStandardItem* item = m_model->itemFromIndex(mindex);
	if(item)
	{
		//Make it works even if you rightclick on the checkbox
		QStandardItem* chkcol = m_model->item(item->row(), 0);
		if(chkcol)
		{
			QString trackName = chkcol->data(TrackNameRole).toString();
			int type = chkcol->data(TrackRole).toInt();
			Track* track = song->findTrack(trackName);
			if(!track || !m_editor || chkcol->column() != 0)
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
						oom->composer->addCanvasPart(track);
					break;
					case 2:
					{
						CItem* citem = oom->composer->addCanvasPart(track);
						Part* mp = citem->part();
						if(mp)
						{
							m_editor->addPart(mp);
							updatePartSelection(mp);
							populateTable();//update check state
						}
						break;
					}
					case 3:
					{
						if(npart)
						{
							audio->msgRemovePart(npart);
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
						break;
					}
				}
			}
			delete p;
		}
	}
}/*}}}*/

void TrackListView::selectionChanged(const QModelIndex current, const QModelIndex)/*{{{*/
{
	if(!current.isValid())
		return;
	QStandardItem* test = m_model->itemFromIndex(current);
	if(test && test->column() == 0)
		return;
	int row = current.row();
	m_selectedIndex = row;
	QStandardItem* item = m_model->item(row, 0);
	int type = item->data(TrackRole).toInt();
	bool checked = (item->checkState() == Qt::Checked);
	QString trackName = item->data(TrackNameRole).toString();
	Track* track = song->findTrack(trackName);
	if(!track || !m_editor || type == 1 || !checked)
		return;

	PartList* list = track->parts();
	int sn = item->data(PartRole).toInt();
	unsigned tick = item->data(TickRole).toInt();
	Part* part = list->find(tick, sn);
	updatePartSelection(part);
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
		movePlaybackToPart(part);
		// and turn it on for the new parts track
		song->setRecordFlag(track, true);
		song->deselectTracks();
		song->deselectAllParts();
		track->setSelected(true);
		part->setSelected(true);
		song->update(SC_SELECTION);
	}
}/*}}}*/

void TrackListView::movePlaybackToPart(Part* part)/*{{{*/
{
	bool snap = tconfig().get_property("PerformerEdit", "snaptopart", true).toBool();
	if(audio->isPlaying() || !snap)
		return;
	if(part)
	{
		unsigned tick = part->tick();
		EventList* el = part->events();
		if(el->empty())
		{//move pb to part start
			Pos p(tick, true);
			song->setPos(0, p, true, true, true);
		}
		else
		{
			for(iEvent i = el->begin(); i != el->end(); ++i)
			{
				Event ev = i->second;
				if(ev.isNote())
				{
					Pos p(tick+ev.tick(), true);
					song->setPos(0, p, true, true, true);
					break;
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
		QList<QStandardItem*> found = m_model->findItems(part->name(), Qt::MatchExactly, 1);
		foreach(QStandardItem* item, found)
		{
			int type = item->data(TrackRole).toInt();
			if(type == 1)
				continue;
			int sn = item->data(PartRole).toInt();
			int psn = part->sn();
			//printf("Serial no. item: %d, part: %d\n", sn, psn);
			if(sn == psn)
			{
				QStandardItem* pitem = m_model->item(item->row(), 0);
				//printf("Updating item\n");
				m_model->blockSignals(true);
				if(on)
					pitem->setCheckState(Qt::Checked);
				else
					pitem->setCheckState(Qt::Unchecked);
				m_model->blockSignals(false);
			}
		}
	}
}/*}}}*/

void TrackListView::updateCheck()/*{{{*/
{
	for(int i = 0; i < m_model->rowCount(); ++i)
	{
		QStandardItem* item = m_model->item(i, 0);
		if(item)
		{
			int type = item->data(TrackRole).toInt();
			QString trackName = item->data(TrackNameRole).toString();
			if(type == 1)
			{//TrackMode
				m_model->blockSignals(true);
				if(m_selected.contains(trackName))
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
	int type = item->data(TrackRole).toInt();
	int column = item->column();
	QStandardItem* chkItem = m_model->item(item->row(), 0);
	bool checked = (chkItem->checkState() == Qt::Checked);
	QString trackName = item->data(TrackNameRole).toString();
	Track* track = song->findTrack(trackName);
	if(!track || !m_editor)
		return;

	PartList* list = track->parts();
	if(list->empty())
	{
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
					m_selected.append(trackName);
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
					m_selected.removeAll(trackName);
					updateCheck();
					song->update(SC_SELECTION);
				}
			}
			else
			{
				QString newName = item->text();
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
						m_selected.removeAll(trackName);
						updateCheck();
						song->update(SC_SELECTION);
					}
				}
				else
				{
					QString name = item->text();
					if(name.isEmpty())
					{
						QMessageBox::critical(this, tr("OOMidi: Invalid part name"),
								tr("Please choose a name with at least one charactor"),
								QMessageBox::Ok, Qt::NoButton, Qt::NoButton);
						
						m_model->blockSignals(true);
						item->setText(item->data(PartRole).toString());
						m_model->blockSignals(false);
						update();
						return;
					}

					m_model->blockSignals(true);
					item->setData(name, PartRole);
					m_model->blockSignals(false);

					Part* newPart = part->clone();
					newPart->setName(name);
					// Indicate do undo, and do port controller values but not clone parts.
					audio->msgChangePart(part, newPart, true, true, false);
				}
			}
		}
		break;
	}
	update();
	if(m_selectedIndex < m_model->rowCount())
		m_table->selectRow(m_selectedIndex);
}/*}}}*/

