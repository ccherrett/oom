//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================


#include <QMessageBox>
#include <QDialog>
#include <QStringListModel>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QModelIndexList>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QMap>
#include <QtGui>

#include <math.h>
#include <string.h>
#include "tvieweditor.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "midiport.h"
#include "instruments/minstrument.h"
#include "icons.h"
#include "app.h"
#include "popupmenu.h"
#include "track.h"
#include "trackview.h"
#include "instrumentdelegate.h"
#include "tablespinner.h"
#include "CreateTrackDialog.h"

TrackViewEditor::TrackViewEditor(QWidget* parent, bool templateMode) : QDialog(parent)
{
	setupUi(this);
	_selected = 0;
	_editing = false;
	_addmode = false;
	m_templateMode = templateMode;

	cmbMode->addItem(tr("Song"));
	cmbMode->addItem(tr("Global"));
	cmbMode->setCurrentIndex((int)m_templateMode);

	InstrumentDelegate* pdelegate = new InstrumentDelegate(this);
	TableSpinnerDelegate* tdelegate = new TableSpinnerDelegate(this);
	m_model = new QStandardItemModel(0, 3, this);
	m_selmodel = new QItemSelectionModel(m_model);
	optionsTable->setModel(m_model);
	optionsTable->setItemDelegateForColumn(1, tdelegate);
	optionsTable->setItemDelegateForColumn(2, pdelegate);
	m_allmodel = new QStandardItemModel(this);
	m_allselmodel = new QItemSelectionModel(m_allmodel);
	updateTableHeader();
	
	buildViewList();
	populateTypes();
	m_filterModel = new QSortFilterProxyModel(this);
	m_filterModel->setSourceModel(m_allmodel);
	m_filterModel->setFilterRole(TrackTypeRole);
	m_filterModel->setFilterKeyColumn(0);
	listAllTracks->setModel(m_filterModel);
	
	btnOk = buttonBox->button(QDialogButtonBox::Ok);
	btnCancel = buttonBox->button(QDialogButtonBox::Cancel);
	btnApply = buttonBox->button(QDialogButtonBox::Apply);
	btnApply->setEnabled(false);
	btnCopy->setEnabled(false);

	btnUp->setIcon(*up_arrowIconSet3);
	btnDown->setIcon(*down_arrowIconSet3);
	btnRemove->setIcon(*previousIconSet3);
	btnAdd->setIcon(*nextIconSet3);
	btnAddVirtual->setIcon(*plusIconSet3);

	populateTrackList();
	cmbTypeSelected(0);

	connect(btnAdd, SIGNAL(clicked()), SLOT(btnAddTrack()));
	connect(btnAddVirtual, SIGNAL(clicked()), this, SLOT(btnAddVirtualTrack()));
	
	txtName->setReadOnly(true);
	connect(btnRemove, SIGNAL(clicked()), SLOT(btnRemoveTrack()));
	connect(btnNew, SIGNAL(clicked(bool)), SLOT(btnNewClicked(bool)));

	connect(cmbMode, SIGNAL(currentIndexChanged(int)), SLOT(setMode(int)));

	connect(cmbViews, SIGNAL(currentIndexChanged(int)), SLOT(cmbViewSelected(int)));
	
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
	
	connect(btnOk, SIGNAL(clicked(bool)), SLOT(btnOkClicked(bool)));
	
	connect(btnApply, SIGNAL(clicked(bool)), SLOT(btnApplyClicked(bool)));
	
	connect(btnCancel, SIGNAL(clicked(bool)), SLOT(btnCancelClicked(bool)));

	connect(btnDelete, SIGNAL(clicked(bool)), SLOT(btnDeleteClicked(bool)));

	connect(btnCopy, SIGNAL(clicked()), SLOT(btnCopyClicked()));
	
	connect(txtName, SIGNAL(textEdited(QString)), SLOT(txtNameEdited(QString)));
	
	connect(btnUp, SIGNAL(clicked(bool)), SLOT(btnUpClicked(bool)));
	
	connect(btnDown, SIGNAL(clicked(bool)), SLOT(btnDownClicked(bool)));
	
	connect(chkRecord, SIGNAL(toggled(bool)), SLOT(chkRecordChecked(bool)));
	
	connect(m_model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(settingsChanged(QStandardItem*)));
	
	connect(txtComment, SIGNAL(textChanged()), SLOT(txtCommentChanged()));
}


void TrackViewEditor::buildViewList()
{
	while(cmbViews->count())
		cmbViews->removeItem(0);

	TrackViewList *tvl = 0;
	if(m_templateMode)
	{
		cmbViews->addItem(tr("Select Instrument Template"), 0);
		tvl = oom->instrumentTemplates();
	}
	else
	{
		cmbViews->addItem(tr("Select Track View"), 0);
		tvl = song->trackviews();
	}
	if(tvl && tvl->size())
	{
		ciTrackView iter = tvl->constBegin();
		while (iter != tvl->constEnd())
		{
			cmbViews->addItem(iter.value()->viewName(), iter.value()->id());
			++iter;
		}
	}
}

void TrackViewEditor::populateTypes()
{
	while(cmbType->count())
		cmbType->removeItem(0);
	
//MIDI=0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_BUSS,AUDIO_AUX
	//Populate trackTypes and pass it to cmbTypes 
	cmbType->addItem(tr("All Types"), -1);
	cmbType->addItem(tr("Midi Tracks"), Track::MIDI);
	cmbType->addItem(tr("Audio Tracks"), Track::WAVE);
	cmbType->addItem(tr("Inputs"), Track::AUDIO_INPUT);
	cmbType->addItem(tr("Outputs"), Track::AUDIO_OUTPUT);
	cmbType->addItem(tr("Busses"), Track::AUDIO_BUSS);
	cmbType->addItem(tr("Auxs"), Track::AUDIO_AUX);
}

void TrackViewEditor::setType(int val)
{//Translate tracktype to combo index
	Track::TrackType type = (Track::TrackType)val;
	switch(type)
	{
		case Track::MIDI:
			cmbType->setCurrentIndex(1);
		break;
		case Track::WAVE:
			cmbType->setCurrentIndex(2);
		break;
		case Track::AUDIO_INPUT:
			cmbType->setCurrentIndex(3);
		break;
		case Track::AUDIO_OUTPUT:
			cmbType->setCurrentIndex(4);
		break;
		case Track::AUDIO_BUSS:
			cmbType->setCurrentIndex(5);
		break;
		case Track::AUDIO_AUX:
			cmbType->setCurrentIndex(6);
		break;
		default:
			cmbType->setCurrentIndex(0);
		break;
	}
}

void TrackViewEditor::populateTrackList()/*{{{*/
{
	m_allmodel->clear();
	if(!m_templateMode)
	{
		for (ciTrack t = song->tracks()->begin(); t != song->tracks()->end(); ++t)
		{
			QStandardItem *item = new QStandardItem((*t)->name());
			item->setData((*t)->id(), TrackIdRole);
			item->setData((*t)->type(), TrackTypeRole);
			item->setData(0, TrackSourceRole);
			item->setEditable(false);
			m_allmodel->appendRow(item);
		}
	}
	{
		QMap<qint64, VirtualTrack*> vthash = trackManager->virtualTracks();
		QMap<qint64, VirtualTrack*>::const_iterator iter = vthash.constBegin();
		QList<qint64> keys;
		while(iter != vthash.constEnd())
		{
			if(!keys.contains(iter.value()->id))
			{
				QString s(iter.value()->name);
				QStandardItem *item = new QStandardItem(s.append(tr(" (Virtual)")));
				item->setData(iter.value()->id, TrackIdRole);
				item->setData(iter.value()->type, TrackTypeRole);
				item->setData(1, TrackSourceRole);
				item->setEditable(false);
				m_allmodel->appendRow(item);
				keys.append(iter.value()->id);
			}
			iter++;
		}
	}
}/*}}}*/

//----------------------------------------------
// Slots
//----------------------------------------------

void TrackViewEditor::setMode(int mode)
{
	//Just in case it was called externally
	cmbMode->blockSignals(true);
	cmbMode->setCurrentIndex(mode);
	cmbMode->blockSignals(false);

	m_templateMode = mode;

	reset();
	populateTrackList();
	buildViewList();
	setType(-1);
}

void TrackViewEditor::txtNameEdited(QString text)
{
	if(_selected)
	{
		_editing = true;
		int cursorPos = txtName->cursorPosition();
		txtName->setText(TrackView::getValidName(text, m_templateMode));
		txtName->setCursorPosition(cursorPos);
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::chkRecordChecked(bool)
{
	if(_selected)
	{
		_editing = true;
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::settingsChanged(QStandardItem*)
{
	if(_selected)
	{
		_editing = true;
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::txtCommentChanged()
{
	if(_selected)
	{
		_editing = true;
		btnApply->setEnabled(true);
	}
}

void TrackViewEditor::btnNewClicked(bool)/*{{{*/
{
	reset();
	TrackView *v = 0;
	if(m_templateMode)
	{
		v = oom->addInstrumentTemplate();
		if(v)
			_selected = v->id();
	}
	else
	{
		v = song->addNewTrackView();
		if(v)
			_selected = v->id();
	}
	if(v)
	{
		_addmode = true;
		_editing = true;
		btnDelete->setEnabled(false);
		//Clear txtName, set value to Untitled 
		txtName->setText(v->viewName());
		txtName->setFocus(Qt::OtherFocusReason);
		txtName->setReadOnly(false);
		btnApply->setEnabled(true);
	}
}/*}}}*/

void TrackViewEditor::cmbViewSelected(int ind)/*{{{*/
{//TODO: Update for virtual tracks
	if(ind == 0)
	{
		m_model->clear();
		//Clear txtName
		txtName->setText("");
		//Disable btnCopy
		btnCopy->setEnabled(false);
		btnDelete->setEnabled(false);
		chkRecord->setChecked(false);
		return;
	}
	m_model->clear();
	btnCopy->setEnabled(true);
	btnDelete->setEnabled(true);
	//Perform actions to populate list below based on selected view
	QString sl = cmbViews->itemText(ind);
	qint64 tvid = cmbViews->itemData(ind).toLongLong();
	txtName->setText(sl);
	TrackView* v = 0;
	if(m_templateMode)
		v = oom->findInstrumentTemplateById(tvid);
	else
		v = song->findTrackViewById(tvid);
	_editing = true;
	if(v)
	{
		_selected = v->id();
		QMap<qint64, VirtualTrack*> vtracks = trackManager->virtualTracks();
		txtComment->blockSignals(true);
		txtComment->setText(v->comment());
		txtComment->blockSignals(false);
		txtComment->moveCursor(QTextCursor::End);
		QList<qint64> *list = v->trackIndexList();
		for(int p = 0; p < list->size(); ++p)
		{	
			qint64 i = list->at(p);
			TrackView::TrackViewTrack *tvt = v->tracks()->value(i);
			if(tvt)
			{
				if(tvt->is_virtual)
				{
					VirtualTrack* t = vtracks.value(i);
					if(t)
					{
						QString trackname = t->name;
						QStandardItem* tname = new QStandardItem(trackname);
						tname->setData(t->id, TrackIdRole);
						tname->setData(1, TrackSourceRole);
						tname->setEditable(false);
						QString tp("0");
						QString pname(t->type == Track::MIDI ? tr("Select Patch") : "-");
						int prog = 0;
						//Check if there are settings
						if(tvt->hasSettings())
						{
							TrackView::TrackSettings *tset = tvt->settings;
							if(tset)
							{
								tp = QString::number(tset->transpose);
								pname = tset->pname;
								prog = tset->program;
							}
						}

						QStandardItem* transp = new QStandardItem(tp);
						transp->setEditable(false);
						if(t->type == Track::MIDI)
							transp->setEditable(true);
						QStandardItem* patch = new QStandardItem(pname);
						patch->setData(prog, ProgramRole);
						patch->setEditable(false);
						if(t->type == Track::MIDI)
						{
							patch->setData(t->instrumentName, InstrumentNameRole);
							patch->setEditable(true);
						}
						QList<QStandardItem*> rowData;
						rowData.append(tname);
						rowData.append(transp);
						rowData.append(patch);
						m_model->appendRow(rowData);
					}
				}
				else
				{
					Track* t = song->findTrackById(i);
					if(t)
					{
						QString trackname = t->name();
						QStandardItem* tname = new QStandardItem(trackname);
						tname->setData(t->id(), TrackIdRole);
						tname->setData(0, TrackSourceRole);
						tname->setEditable(false);
						QString tp("0");
						QString pname(t->isMidiTrack() ? tr("Select Patch") : "-");
						int prog = 0;
						//Check if there are settings
						if(tvt->hasSettings())
						{
							TrackView::TrackSettings *tset = tvt->settings;
							if(tset)
							{
								tp = QString::number(tset->transpose);
								pname = tset->pname;
								prog = tset->program;
							}
						}

						QStandardItem* transp = new QStandardItem(tp);
						transp->setEditable(false);
						if(t->isMidiTrack())
							transp->setEditable(true);
						QStandardItem* patch = new QStandardItem(pname);
						patch->setData(prog, ProgramRole);
						patch->setEditable(false);
						if(t->isMidiTrack())
						{
							int outPort = ((MidiTrack*)t)->outPort();
							MidiInstrument* instr = midiPorts[outPort].instrument();
							patch->setData(instr->iname(), InstrumentNameRole);
							patch->setEditable(true);
						}
						QList<QStandardItem*> rowData;
						rowData.append(tname);
						rowData.append(transp);
						rowData.append(patch);
						m_model->appendRow(rowData);
					}
				}
			}
		}
		chkRecord->setChecked(v->record());
	}
	txtName->setReadOnly(false);
	btnApply->setEnabled(false);
	updateTableHeader();
	_editing = false;
}/*}}}*/

void TrackViewEditor::cmbTypeSelected(int index)/*{{{*/
{
	//Perform actions to filter the list below based on selected type
	//"All" "Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Wave"
	int type = cmbType->itemData(index).toInt();
	if(type == -1)
	{
		m_filterModel->setFilterRegExp(QRegExp("[0-9]", Qt::CaseInsensitive, QRegExp::Wildcard));
		return;
	}

	Track::TrackType t = (Track::TrackType)type;
	switch (t)
	{
		case Track::AUDIO_OUTPUT:
			m_filterModel->setFilterRegExp(QRegExp(QString::number(Track::AUDIO_OUTPUT), Qt::CaseInsensitive, QRegExp::FixedString));
			break;
		case Track::AUDIO_INPUT:
			m_filterModel->setFilterRegExp(QRegExp(QString::number(Track::AUDIO_INPUT), Qt::CaseInsensitive, QRegExp::FixedString));
			break;
		case Track::AUDIO_AUX:
			m_filterModel->setFilterRegExp(QRegExp(QString::number(Track::AUDIO_AUX), Qt::CaseInsensitive, QRegExp::FixedString));
			break;
		case Track::AUDIO_BUSS:
			m_filterModel->setFilterRegExp(QRegExp(QString::number(Track::AUDIO_BUSS), Qt::CaseInsensitive, QRegExp::FixedString));
			break;
		case Track::MIDI...Track::DRUM:
			m_filterModel->setFilterRegExp(QRegExp(QString::number(Track::MIDI), Qt::CaseInsensitive, QRegExp::FixedString));
			break;
		case Track::WAVE:
			m_filterModel->setFilterRegExp(QRegExp(QString::number(Track::WAVE), Qt::CaseInsensitive, QRegExp::FixedString));
			break;
		default:
		break;
	}
}/*}}}*/

void TrackViewEditor::btnApplyClicked(bool/* state*/)/*{{{*/
{
	//printf("TrackViewEditor::btnApplyClicked()\n");
	if(_editing && _selected)
	{
		TrackView *view = 0;
		if(m_templateMode)
		{
			view = oom->findInstrumentTemplateById(_selected);
		}
		else
		{
			view = song->findTrackViewById(_selected);
		}
		if(!view)
		{//TODO Fire error and reset()
			reset();
			return;
		}
		view->setViewName(txtName->text());
		//printf("after setviewname\n");
		view->clear();
		//Process all the tracks in the listSelectedTracks and add them to the TrackList
		for(int row = 0; row < m_model->rowCount(); ++row)
		{
			QStandardItem* titem = m_model->item(row, 0);
			if(titem)
			{
				int srcType = titem->data(TrackSourceRole).toInt();
				qint64 tid = titem->data(TrackIdRole).toLongLong();
				if(srcType)
				{
					VirtualTrack *vt = trackManager->virtualTracks().value(tid);
					if(vt)
					{
						TrackView::TrackViewTrack *tvt = new TrackView::TrackViewTrack(vt->id);
						tvt->is_virtual = true;
						if(vt->type  == Track::MIDI)
						{
							QStandardItem* trans = m_model->item(row, 1);
							int transpose = trans->data(Qt::DisplayRole).toInt();
							QStandardItem* patch = m_model->item(row, 2);
							QString pname = patch->text();
							int prog = patch->data(ProgramRole).toInt();
							if((transpose != 0) || prog >= 0)
							{
								//printf("Added TrackSettings for Track: %s\n", titem->text().toUtf8().constData());
								TrackView::TrackSettings *ts = new TrackView::TrackSettings;
								ts->pname = pname;
								ts->program = prog;
								ts->transpose = transpose;
								tvt->settings = ts;
							}
						}
						view->addTrack(tvt);
					}
				}
				else
				{
					Track *t = song->findTrackById(tid);
					if(t)
					{
						TrackView::TrackViewTrack *tvt = new TrackView::TrackViewTrack(t->id());
						if(t->isMidiTrack())
						{
							QStandardItem* trans = m_model->item(row, 1);
							int transpose = trans->data(Qt::DisplayRole).toInt();
							QStandardItem* patch = m_model->item(row, 2);
							QString pname = patch->text();
							int prog = patch->data(ProgramRole).toInt();
							if((transpose != 0) || prog >= 0)
							{
								//printf("Added TrackSettings for Track: %s\n", titem->text().toUtf8().constData());
								TrackView::TrackSettings *ts = new TrackView::TrackSettings;
								ts->pname = pname;
								ts->program = prog;
								ts->transpose = transpose;
								tvt->settings = ts;
							}
						}
						view->addTrack(tvt);
					}
				}
			}
		}
		view->setRecord(chkRecord->isChecked());
		view->setComment(txtComment->toPlainText());
		if(m_templateMode)
		{
			oom->changeConfig(true);
			song->update(SC_CONFIG);
			//Make the virtual tracks of the new template available to others
			populateTrackList();
		}
		else
		{
			//Song is not dirty from adding/removing templates, they are at config level
			song->dirty = true;
			song->updateTrackViews();
			if(_addmode)
				song->update(SC_VIEW_ADDED);
			else
				song->update(SC_VIEW_CHANGED);
		}
		btnApply->setEnabled(false);
		reset();
	}
}/*}}}*/

void TrackViewEditor::btnOkClicked(bool state)
{
	//printf("TrackViewEditor::btnOkClicked()\n");
	//Apply is the same as ok without the close so just call the other method
	if(_editing)
		btnApplyClicked(state);
	//Do other close cleanup;
}

void TrackViewEditor::reset()/*{{{*/
{
	//Reset cmbViews
	cmbViews->blockSignals(true);
	QAbstractItemModel *mod = cmbViews->model();
	if(mod)
		mod->removeRows(0, mod->rowCount());
	m_model->clear();
	buildViewList();
	populateTrackList();
	if(m_vtrackList.size())
	{
		//foreach(qint64 id, m_vtrackList.keys())
		//	trackManager->removeVirtualTrack(id);
		m_vtrackList.clear();
	}
	cmbViews->blockSignals(false);
	txtComment->blockSignals(true);
	txtComment->setText("");
	txtComment->blockSignals(false);
	cmbViews->setCurrentIndex(0);
	btnApply->setEnabled(false);
	btnDelete->setEnabled(false);
	btnCopy->setEnabled(false);
	txtName->setReadOnly(true);
	_editing = false;
	_addmode = false;
	_selected = 0;
	chkRecord->setChecked(false);
	txtName->setText("");
	updateTableHeader();
}/*}}}*/

void TrackViewEditor::btnCancelClicked(bool/* state*/)/*{{{*/
{
	//printf("TrackViewEditor::btnCancelClicked()\n");
	if(_addmode)
	{//remove the trackview that was added to the song
		if(m_templateMode)
		{
			TrackView* v = oom->findInstrumentTemplateById(_selected);
			oom->removeInstrumentTemplate(_selected);
			delete v;
			oom->changeConfig(true);
			song->update(SC_CONFIG);
		}
		else
		{
			TrackView* v = song->findTrackViewById(_selected);
			song->removeTrackView(_selected);
			delete v;
			song->update(SC_VIEW_CHANGED);
		}
		if(m_vtrackList.size())
		{
			//foreach(qint64 id, m_vtrackList.keys())
			//	trackManager->removeVirtualTrack(id);
			m_vtrackList.clear();
		}
	}
	_addmode = false;
	_selected = 0;
}/*}}}*/

void TrackViewEditor::btnCopyClicked()/*{{{*/
{
	if(_selected)
	{
		TrackView *tv = 0;
		TrackView *view = 0;
		qDebug("TrackViewEditor::btnCopyClicked:==========Selected ID: %lld", _selected);
		if(m_templateMode)
		{
			tv = oom->findInstrumentTemplateById(_selected);
		}
		else
		{
			tv = song->findTrackViewById(_selected);
		}

		if(tv)
		{
			view = new TrackView(*tv);
			if(view)
			{
				qDebug("TrackViewEditor::btnCopyClicked:==========NEW ID: %lld", view->id());
				view->setViewName(TrackView::getValidName(tv->viewName(), m_templateMode));
				if(m_templateMode)
				{
					oom->insertInstrumentTemplate(view, -1);
				}
				else
				{
					song->insertTrackView(view, -1);
				}
				txtName->setText(view->viewName());
				cmbViews->addItem(view->viewName(), view->id());
				cmbViews->blockSignals(true);
				cmbViews->setCurrentIndex(cmbViews->findText(view->viewName()));
				cmbViews->blockSignals(false);
				btnApply->setEnabled(true);
				_selected = view->id();
				_addmode = true;
				_editing = true;
			}
		}
	}
}/*}}}*/

void TrackViewEditor::btnDeleteClicked(bool)/*{{{*/
{
	if(_selected)
	{
		if(m_templateMode)
		{
			oom->removeInstrumentTemplate(_selected);
			oom->changeConfig(true);
			song->update(SC_CONFIG);
			populateTrackList();
		}
		else
		{
			song->removeTrackView(_selected);
			song->dirty = true;
			song->updateTrackViews();
			song->update(SC_VIEW_DELETED);
		}
		reset();
	}
}/*}}}*/


void TrackViewEditor::btnAddTrack()/*{{{*/
{
	//Perform actions to add action to right list and remove from left
	qDebug("TrackViewEditor::btnAddTrack:");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QItemSelectionModel* model = listAllTracks->selectionModel();
		if (model->hasSelection())
		{
			qDebug("TrackViewEditor::btnAddTrack: found selections");
			QModelIndexList sel = model->selectedRows(0);
			QList<QModelIndex>::const_iterator id;
			for (id = sel.constBegin(); id != sel.constEnd(); ++id)
			{
				//We have to index we will get the row.
				QModelIndex idx = *id;
				//QStandardItem *item = m_allmodel->item((*id).row());//FromIndex((*id));
				if(idx.isValid())
				{
					qDebug("TrackViewEditor::btnAddTrack: found item~~~~~~~~~~~~~~~~");
					qint64 tid = idx.data(TrackIdRole).toLongLong();
					int trackSource = idx.data(TrackSourceRole).toInt();
					QString text(idx.data().toString());
					if(trackSource)
					{
						VirtualTrack* trk = trackManager->virtualTracks().value(tid);
						if (trk)
						{
							qDebug("TrackViewEditor::btnAddTrack:~~~~~~~~~~~~~~Found Virtual Track");
						//	printf("Adding Track from row: %d\n", row);
							QList<QStandardItem *> items = m_model->findItems(trk->name);
							if(items.isEmpty())
							{
								QString trackname = trk->name;
								QStandardItem* tname = new QStandardItem(trackname);
								tname->setData(trk->id, TrackIdRole);
								tname->setData(1, TrackSourceRole);
								tname->setEditable(false);
								QString tp("0");
								QString pname(trk->type == Track::MIDI ? tr("Select Patch") : "-");
								int prog = 0;
								//Check if there are settings

								QStandardItem* transp = new QStandardItem(tp);
								transp->setEditable(false);
								if(trk->type == Track::MIDI)
									transp->setEditable(true);
								QStandardItem* patch = new QStandardItem(pname);
								patch->setData(prog, ProgramRole);
								patch->setEditable(false);
								if(trk->type == Track::MIDI)
								{
									patch->setData(trk->instrumentName, InstrumentNameRole);
									patch->setEditable(true);
								}
								QList<QStandardItem*> rowData;
								rowData.append(tname);
								rowData.append(transp);
								rowData.append(patch);
								m_model->appendRow(rowData);
								optionsTable->selectRow((m_model->rowCount()-1));
							}
						}
					}
					else
					{
						if(!m_templateMode)
						{
							Track* trk = song->findTrackById(tid);
							if (trk)
							{
							//	printf("Adding Track from row: %d\n", row);
								QList<QStandardItem *> items = m_model->findItems(trk->name());
								if(items.isEmpty())
								{
									QStandardItem* tname = new QStandardItem(trk->name());
									tname->setData(trk->id(), TrackIdRole);
									tname->setData(0, TrackSourceRole);
									tname->setEditable(false);
									QStandardItem* transp = new QStandardItem(QString::number(0));
									transp->setEditable(false);
									if(trk->isMidiTrack())
										transp->setEditable(true);
									QStandardItem* patch = new QStandardItem(trk->isMidiTrack() ? tr("Select Patch") : "-");
									patch->setData(0, ProgramRole);
									patch->setEditable(false);
									if(trk->isMidiTrack())
									{
										int outPort = ((MidiTrack*)trk)->outPort();
										MidiInstrument* instr = midiPorts[outPort].instrument();
										patch->setData(instr->iname(), InstrumentNameRole);
										patch->setEditable(true);
									}
									QList<QStandardItem*> rowData;
									rowData.append(tname);
									rowData.append(transp);
									rowData.append(patch);
									m_model->appendRow(rowData);
									optionsTable->selectRow((m_model->rowCount()-1));
								}
							}
						}
					}
				}
				//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
			}
		}
	}
	updateTableHeader();
}/*}}}*/

void TrackViewEditor::btnRemoveTrack()/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	//printf("Remove button clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QList<int> selrows = getSelectedRows();
		QList<int> del;
		if (!selrows.isEmpty())
		{
			for(int i = 0; i < selrows.size(); ++i)
			{
				//We have to index we will get the row.
				int row = selrows[i];
				QStandardItem* item = m_model->item(row);
				QString val = item->text();
				int srcType = item->data(TrackSourceRole).toInt();
				qint64 tid = item->data(TrackIdRole).toLongLong();
				if(srcType)
				{
					if(m_vtrackList.contains(tid))
					{
						m_vtrackList.erase(m_vtrackList.find(tid));
						//trackManager->removeVirtualTrack(tid);
					}
				}
				del.append(row);
				//qDebug("Found Track %s at index %d with type %d\n", val.toUtf8().constData(), row, trk->type());
			}
			//Delete the list in reverse order so the listview maintains the propper state
			if(!del.isEmpty())
			{
				QListIterator<int> it(del); 
				it.toBack();
				while(it.hasPrevious())
				{
					m_model->takeRow(it.previous());
				}

			}
			populateTrackList();
		}
	}
	updateTableHeader();
}/*}}}*/

void TrackViewEditor::btnAddVirtualTrack()
{
	if(!_selected)
		return;
	VirtualTrack* vt;
	CreateTrackDialog *ctdialog = new CreateTrackDialog(&vt, 0, -1, this);
	if(ctdialog->exec() && vt)
	{
		trackManager->addVirtualTrack(vt);
		//Make song save the virtual track
		song->update(SC_CONFIG);

		//Add to the full list of tracks and select the type
		QString s(vt->name);
		QStandardItem *item = new QStandardItem(s.append(tr(" (Virtual)")));
		item->setData(vt->id, TrackIdRole);
		item->setData(vt->type, TrackTypeRole);
		item->setData(1, TrackSourceRole);
		item->setEditable(false);
		m_allmodel->appendRow(item);
		
		//display types of created virtual
		setType(vt->type);
		
		if(QMessageBox::question(this, tr("Add track"), tr("Add new track to current view?"), QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok) == QMessageBox::Ok)
		{
			m_vtrackList[vt->id] = vt;
			QStandardItem* tname = new QStandardItem(vt->name);
			tname->setData(vt->id, TrackIdRole);
			tname->setData(1, TrackSourceRole);
			tname->setEditable(false);
			QStandardItem* transp = new QStandardItem(QString::number(0));
			transp->setEditable(false);
			if(vt->type == Track::MIDI)
				transp->setEditable(true);
			QStandardItem* patch = new QStandardItem(vt->type == Track::MIDI ? tr("Select Patch") : "-");
			patch->setData(0, ProgramRole);
			patch->setEditable(false);
			if(vt->type == Track::MIDI)
			{
				patch->setData(vt->instrumentName, InstrumentNameRole);
				patch->setData(vt->instrumentType,  InstrumentTypeRole);
				patch->setEditable(true);
			}
			QList<QStandardItem*> rowData;
			rowData.append(tname);
			rowData.append(transp);
			rowData.append(patch);
			m_model->appendRow(rowData);
			optionsTable->selectRow((m_model->rowCount()-1));
		}
	}
}

void TrackViewEditor::btnUpClicked(bool)/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	//printf("Remove up clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QList<int> rows = getSelectedRows();
		if (!rows.isEmpty())
		{
			int id = rows.at(0);
			if ((id - 1) < 0)
				return;
			int row = (id - 1);
			QList<QStandardItem*> item = m_model->takeRow(id);
			m_model->insertRow(row, item);
			optionsTable->selectRow(row);
		}
	}
}/*}}}*/

void TrackViewEditor::btnDownClicked(bool)/*{{{*/
{
	//Perform action to remove track from the selectedTracks list
	//printf("Remove up clicked\n");
	if(_selected)
	{
		btnApply->setEnabled(true);
		_editing = true;
		QList<int> rows = getSelectedRows();
		if (!rows.isEmpty())
		{
			int id = rows.at(0);
			if ((id + 1) >= m_model->rowCount())
				return;
			int row = (id + 1);
			QList<QStandardItem*> item = m_model->takeRow(id);
			m_model->insertRow(row, item);
			optionsTable->selectRow(row);
		}
	}
}/*}}}*/

QList<int> TrackViewEditor::getSelectedRows()/*{{{*/
{
	QList<int> rv;
	QItemSelectionModel* smodel = optionsTable->selectionModel();
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

void TrackViewEditor::updateTableHeader()/*{{{*/
{
	QStandardItem* tname = new QStandardItem(tr("Track"));
	QStandardItem* transp = new QStandardItem(tr("Transpose"));
	QStandardItem* hpatch = new QStandardItem(tr("Patch"));
	m_model->setHorizontalHeaderItem(0, tname);
	m_model->setHorizontalHeaderItem(1, transp);
	m_model->setHorizontalHeaderItem(2, hpatch);
	optionsTable->setColumnWidth(0, 150);
	optionsTable->setColumnWidth(1, 80);
	//optionsTable->setColumnWidth(2, 120);
	optionsTable->horizontalHeader()->setStretchLastSection(true);
}/*}}}*/
